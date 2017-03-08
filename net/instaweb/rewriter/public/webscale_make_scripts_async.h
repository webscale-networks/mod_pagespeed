/*
 *  COPYRIGHT:
 *     (C) Copyright Webscale Networks 2017.
 *     Licensed Materials - All Rights Reserved.
 *
*/

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_ASYNC_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_ASYNC_H_

#include<vector>

#include "net/instaweb/rewriter/public/common_filter.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/string.h"

namespace net_instaweb {

class HtmlCharactersNode;
class HtmlElement;
class MessageHandler;
class RewriteDriver;

class WebscaleMakeScriptsAsync : public CommonFilter {
 public:
  explicit WebscaleMakeScriptsAsync(RewriteDriver* rewrite_driver, MessageHandler* handler);
  virtual ~WebscaleMakeScriptsAsync();

  static void InitStats(Statistics* statistics);

  static GoogleString ConstructPatternFromCustomUrls(const RewriteOptions* options);

  // Overrides CommonFilter
  virtual void StartDocumentImpl();
  virtual void StartElementImpl(HtmlElement* element);
  virtual void EndElementImpl(HtmlElement* element);
  virtual void EndDocument();

  // Overrides HtmlFilter
  virtual const char* Name() const {
    return "WebscaleMakeScriptsAsync";
  }

  MessageHandler* message_handler;
  GoogleString escaped_urls;

  DISALLOW_COPY_AND_ASSIGN(WebscaleMakeScriptsAsync);
};
} // namespace net_instaweb

#endif // NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_ASYNC_H_
