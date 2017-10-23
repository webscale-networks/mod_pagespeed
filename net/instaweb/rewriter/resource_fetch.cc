/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/resource_fetch.h"

#include "base/logging.h"
#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/fetch_race.h"
#include "net/instaweb/http/public/request_context.h"
#include "net/instaweb/http/public/sync_fetcher_adapter_callback.h"
#include "net/instaweb/public/global_constants.h"
#include "net/instaweb/rewriter/public/resource_namer.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_driver_pool.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/rewriter/public/rewrite_stats.h"
#include "net/instaweb/rewriter/public/server_context.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/timer.h"
#include "pagespeed/kernel/http/http_names.h"
#include "pagespeed/kernel/http/response_headers.h"

namespace net_instaweb {

void ResourceFetch::ApplyExperimentOptions(const GoogleUrl& url,
                                           const RequestContextPtr& request_ctx,
                                           ServerContext* server_context,
                                           RewriteOptions** custom_options) {
  const RewriteOptions* active_options;
  if (*custom_options == NULL) {
    RewriteDriverPool* driver_pool =
        server_context->standard_rewrite_driver_pool();
    active_options = driver_pool->TargetOptions();
  } else {
    active_options = *custom_options;
  }
  if (active_options->running_experiment()) {
    // If we're running an experiment and this resource url specifies a
    // experiment_spec, make sure the custom options have that experiment
    // selected.
    ResourceNamer namer;
    namer.DecodeIgnoreHashAndSignature(url.LeafSansQuery());
    if (namer.has_experiment()) {
      if (*custom_options == NULL) {
        *custom_options = active_options->Clone();
      }
      (*custom_options)->SetExperimentStateStr(namer.experiment());
      server_context->ComputeSignature(*custom_options);
    }
  }
}

RewriteDriver* ResourceFetch::GetDriver(
    const GoogleUrl& url, RewriteOptions* custom_options,
    ServerContext* server_context, const RequestContextPtr& request_ctx) {
  ApplyExperimentOptions(url, request_ctx, server_context, &custom_options);
  RewriteDriver* driver = (custom_options == NULL)
      ? server_context->NewRewriteDriver(request_ctx)
      : server_context->NewCustomRewriteDriver(custom_options, request_ctx);
  return driver;
}

void ResourceFetch::StartWithDriver(
    const GoogleUrl& url, CleanupMode cleanup_mode,
    ServerContext* server_context, RewriteDriver* driver,
    AsyncFetch* async_fetch) {

  ResourceFetch* resource_fetch = new ResourceFetch(
      url, cleanup_mode, driver, server_context->timer(),
      server_context->message_handler(), async_fetch);

  if (!driver->FetchResource(url.Spec(), resource_fetch)) {
    resource_fetch->Done(false);
  }
}

void ResourceFetch::Start(const GoogleUrl& url,
                          RewriteOptions* custom_options,
                          ServerContext* server_context,
                          AsyncFetch* async_fetch) {
  RewriteDriver* driver = GetDriver(
      url, custom_options, server_context, async_fetch->request_context());
  StartWithDriver(url, kAutoCleanupDriver,
                  server_context, driver, async_fetch);
}

bool ResourceFetch::BlockingFetch(const GoogleUrl& url,
                                  ServerContext* server_context,
                                  RewriteDriver* driver,
                                  SyncFetcherAdapterCallback* callback) {
  int64 start = server_context->timer()->NowMs();
  MessageHandler* message_handler = server_context->message_handler();
  FetchRace race(callback, server_context->thread_system(), message_handler);

  // Don't auto-cleanup the driver since we use driver->options and
  // driver->DecodeUrl below. In some cases, the driver will be done (and
  // cleaned) before this next call even returns.
  StartWithDriver(url, kDontAutoCleanupDriver, server_context, driver,
    race.NewRacer());

  const int64 deadline = start + driver->options()->blocking_fetch_timeout_ms();
  const int64 fallback_deadline =
      start + driver->options()->blocking_fetch_fallback_timeout_ms();
  if (fallback_deadline < deadline &&
      !race.WaitForWinner(server_context->timer(), fallback_deadline)) {
    StringVector decoded_urls;
    if (driver->DecodeUrl(url, &decoded_urls) && decoded_urls.size() == 1) {
      message_handler->Message(kInfo,
        "Slow primary fetch, issuing fallback request for %s to %s",
        url.spec_c_str(), decoded_urls[0].c_str());
      CacheUrlAsyncFetcher* fallback_fetcher = driver->CreateCacheFetcher();
      fallback_fetcher->set_own_fetcher(true);
      fallback_fetcher->Fetch(
        decoded_urls[0], server_context->message_handler(), race.NewRacer());
    } else {
      message_handler->Message(kInfo,
        "Cannot issue fallback request for %s: decoding resulted in %d urls",
        url.spec_c_str(), int(decoded_urls.size()));
    }
  }

  if (!race.WaitForWinner(server_context->timer(), deadline)) {
    message_handler->Message(kWarning, "Fetch timed out for %s",
        url.spec_c_str());
    driver->Cleanup();
    return false;
  }

  if (!race.Winner()->WaitForDone(server_context->timer(), deadline)) {
    message_handler->Message(kWarning,
        "Fetch timed out waiting for winner to finish: %s", url.spec_c_str());
    driver->Cleanup();
    return false;
  }

  if (!callback->success()) {
    message_handler->Message(kWarning, "Fetch failed for %s, status=%d",
        url.spec_c_str(), callback->response_headers()->status_code());
    driver->Cleanup();
    return false;
  }

  driver->Cleanup();
  return true;
}

ResourceFetch::ResourceFetch(const GoogleUrl& url,
                             CleanupMode cleanup_mode,
                             RewriteDriver* driver,
                             Timer* timer,
                             MessageHandler* handler,
                             AsyncFetch* async_fetch)
    : SharedAsyncFetch(async_fetch),
      driver_(driver),
      timer_(timer),
      message_handler_(handler),
      start_time_ms_(timer->NowMs()),
      redirect_count_(0),
      cleanup_mode_(cleanup_mode) {
  resource_url_.Reset(url);
  DCHECK(driver_->request_headers() == NULL);
}

ResourceFetch::~ResourceFetch() {
}

void ResourceFetch::HandleHeadersComplete() {
  // We do not want any cookies (or other person information) in pagespeed
  // resources. They shouldn't be here anyway, but we assure that.
  ConstStringStarVector v;
  DCHECK(!response_headers()->Lookup(HttpAttributes::kSetCookie, &v));
  DCHECK(!response_headers()->Lookup(HttpAttributes::kSetCookie2, &v));
  response_headers()->RemoveAll(HttpAttributes::kSetCookie);
  response_headers()->RemoveAll(HttpAttributes::kSetCookie2);

  for (int i = 0; i < driver_->options()->num_resource_headers(); ++i) {
    const RewriteOptions::NameValue* nv =
        driver_->options()->resource_header(i);
    response_headers()->Add(nv->name, nv->value);
  }

  // "Vary: Accept-Encoding" for all resources that are transmitted compressed.
  // Server ought to set these, I suppose.
  // response_headers()->Add(HttpAttributes::kVary, "Accept-Encoding");

  response_headers()->Add(kPageSpeedHeader,
                          driver_->options()->x_header_value());
  SharedAsyncFetch::HandleHeadersComplete();
}

void ResourceFetch::HandleDone(bool success) {
  if (success) {
    LOG(INFO) << "Resource " << resource_url_.Spec()
              << " : " << response_headers()->status_code();
  } else {
    // This is a fetcher failure, like connection refused, not just an error
    // status code.
    LOG(WARNING) << "Fetch failed for resource url " << resource_url_.Spec();
    if (!response_headers()->headers_complete()) {
      response_headers()->SetStatusAndReason(HttpStatus::kNotFound);
    }
  }
  RewriteStats* stats = driver_->server_context()->rewrite_stats();
  stats->fetch_latency_histogram()->Add(timer_->NowMs() - start_time_ms_);
  stats->total_fetch_count()->IncBy(1);
  if (cleanup_mode_ == kAutoCleanupDriver) {
    driver_->Cleanup();
  }
  SharedAsyncFetch::HandleDone(success);
  delete this;
}

}  // namespace net_instaweb
