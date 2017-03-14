/*
 *  COPYRIGHT:
 *      (C) Copyright Webscale Networks 2017.
 *       Licensed Materials - All Rights Reserved.
 *
*/

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_DEFER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_DEFER_H_

#include<vector>

#include "net/instaweb/rewriter/public/common_filter.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/string.h"

namespace net_instaweb {

class HtmlCharactersNode;
class HtmlElement;
class MessageHandler;
class RewriteDriver;

// Filter to defer custom third-party scripts.
class WebscaleMakeScriptsDefer : public CommonFilter {
 public:
  explicit WebscaleMakeScriptsDefer(RewriteDriver* rewrite_driver);
  virtual ~WebscaleMakeScriptsDefer();

  // Statistics is not really used. The comments in
  // net/instaweb/rewriter/rewrite_driver.cc mentions that it is good for all
  // new filters to export statistics. If it does, it should be added to
  // InitStats() else it breaks under Apache.
  static void InitStats(Statistics* statistics);

  // Constructs a string with all custom urls escaped and separated by bitwise OR operators.
  // Returns a string that is stored in escaped_urls.
  static GoogleString ConstructPatternFromCustomUrls(const RewriteOptions* options);

  // Overrides CommonFilter
  virtual void StartDocumentImpl();
  virtual void StartElementImpl(HtmlElement* element);
  virtual void EndElementImpl(HtmlElement* element);
  virtual void EndDocument();

  // Overrides HtmlFilter
  virtual const char* Name() const {
    return "WebscaleMakeScriptsDefer";
  }

  private:
    // A string which contains the custom urls escaped separated by bitwise OR operators.
    // Example: If the custom urls are ["js/a1.js", "js/a2.js"]
    // escaped_urls will finally be: "js\/a1\\.js|js\\/a2\\.js"
    GoogleString escaped_urls;

  DISALLOW_COPY_AND_ASSIGN(WebscaleMakeScriptsDefer);
};
} // namespace net_instaweb

#endif // NET_INSTAWEB_REWRITER_PUBLIC_WEBSCALE_MAKE_SCRIPTS_DEFER_H_
