// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: aroman@webscalenetworks.com (Augusto Roman)

#include "net/instaweb/http/public/fetch_race.h"

#include "base/logging.h"
#include "pagespeed/kernel/base/abstract_mutex.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/condvar.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/thread_system.h"
#include "pagespeed/kernel/base/timer.h"
#include "pagespeed/kernel/base/writer.h"

namespace net_instaweb {

FetchRace::FetchRace(
  AsyncFetch* target_fetch,
  ThreadSystem* thread_system,
  MessageHandler* message_handler)
    : thread_system_(thread_system),
      message_handler_(message_handler),
      target_fetch_(target_fetch),
      mutex_(thread_system->NewMutex()),
      winner_cond_(mutex_->NewCondvar()),
      winner_(NULL) {
}

FetchRace::~FetchRace() {
  // When FetchRace is destroyed, we disqualify all of the racers. Even if one
  // of them was previously selected as the winner, it is no longer part of the
  // race and any subsequent writes will fail.
  for (auto racer : racers_) {
    racer->Disqualify();
  }
}

FetchRace::Racer* FetchRace::NewRacer() {
  FetchRace::Racer* r = new FetchRace::Racer(this);
  racers_.push_back(r);
  return r;
}

bool FetchRace::WaitForWinner(Timer* timer, int64 deadline_ms) const {
  ScopedMutex hold_lock(mutex_.get());
  while (winner_ == NULL) {
    int64 remaining_ms = deadline_ms - timer->NowMs();
    if (remaining_ms <= 0) {
      return false; // Ooops!  Timed out.
    }
    winner_cond_->TimedWait(remaining_ms);
  }
  return true;
}

FetchRace::Racer* FetchRace::Winner() const {
  ScopedMutex hold_lock(mutex_.get());
  return winner_;
}

bool FetchRace::Finish(FetchRace::Racer* racer) {
  ScopedMutex hold_lock(mutex_.get());
  if (winner_ == NULL) {
    winner_ = racer;
    winner_cond_->Signal();
  }
  return winner_ == racer;
}

FetchRace::Racer::Racer(FetchRace* race)
    : AsyncFetch(race->target_fetch_->request_context()),
      message_handler_(race->message_handler_),
      mutex_(race->thread_system_->NewMutex()),
      race_(race),
      target_fetch_(race->target_fetch_),
      done_(false),
      done_cond_(mutex_->NewCondvar()) {
  request_headers()->CopyFrom(*target_fetch_->request_headers());
}

// HandleDone is a little complicated: We have to not only pass along the Done()
// call if we're the winner, but we also have to carefully delete ourself if the
// race has ended (usually because we're not the winner, but not always).
void FetchRace::Racer::HandleDone(bool success) {
  // If we're in the winner, pass the Done call on to the target fetch.
  if (ClaimWin()) {
    target_fetch_->Done(success);
  }

  // Now we check to see if we need to clean up after ourselves. If the parent
  // race has been destroyed (and disqualified us), then we are living on after
  // it and we're in charge of cleanup up after ourselves.
  mutex_->Lock();
  bool have_been_disqualified = (race_ == NULL);
  done_ = true;
  done_cond_->Signal();
  mutex_->Unlock();

  // Finally, once the mutex has been released, we can delete ourself if
  // necessary.
  if (have_been_disqualified) {
    delete this;
  }
}

bool FetchRace::Racer::WaitForDone(Timer* timer, int64 deadline_ms) const {
  ScopedMutex hold_lock(mutex_.get());
  // We loop in here in case the TimedWait returns early.
  while (!done_) {
    int64 remaining_ms = deadline_ms - timer->NowMs();
    if (remaining_ms <= 0) {
      return false;
    }
    done_cond_->TimedWait(remaining_ms);
  }
  return true;
}

bool FetchRace::Racer::HandleWrite(const StringPiece& content, MessageHandler* handler) {
  if (!ClaimWin()) {
    return true;
  }
  return target_fetch_->Write(content, handler);
}

bool FetchRace::Racer::HandleFlush(MessageHandler* handler) {
  if (!ClaimWin()) {
    return true;
  }
  return target_fetch_->Flush(handler);
}

void FetchRace::Racer::HandleHeadersComplete() {
  if (!ClaimWin()) {
    return;
  }
  target_fetch_->response_headers()->CopyFrom(*response_headers());
  target_fetch_->extra_response_headers()->CopyFrom(*extra_response_headers());
  if (content_length_known()) {
    target_fetch_->set_content_length(content_length());
  }
  target_fetch_->HeadersComplete();
}
bool FetchRace::Racer::IsCachedResultValid(const ResponseHeaders& headers) {
  if (Disqualified()) {
    // NOTE(aroman) Should this be false? Once disqualified, we should
    // discourage any further work for this fetch.
    return true;
  }
  return target_fetch_->IsCachedResultValid(headers);
}

bool FetchRace::Racer::IsBackgroundFetch() const {
  if (Disqualified()) {
    // If we're disqualified, allow this to be considered a low-priority fetch.
    return true;
  }
  return target_fetch_->IsBackgroundFetch();
}

bool FetchRace::Racer::ClaimWin() {
  ScopedMutex hold_lock(mutex_.get());
  if (race_ == NULL) {
    return false;
  }
  return race_->Finish(this);
}

bool FetchRace::Racer::Disqualified() const {
  ScopedMutex hold_lock(mutex_.get());
  return race_ == NULL;
}

void FetchRace::Racer::Disqualify() {
  // When we are disqualified, that means the parent FetchRace is shutting down
  // and we are now responsible for our lifetime. If Done() has already been
  // called, we should immediately delete ourselves, otherwise we need to delete
  // ourself when Done() gets called later.
  mutex_->Lock();
  race_ = NULL;
  target_fetch_ = NULL;
  bool wasDone = done_;
  mutex_->Unlock();

  if (wasDone) {
    delete this;
  }
}

}  // namespace net_instaweb

