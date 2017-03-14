/*
 *  COPYRIGHT:
 *     (C) Copyright Webscale Networks 2017.
 *      Licensed Materials - All Rights Reserved.
 *     
*/

#include <vector>

#include "base/logging.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_driver_factory.h"
#include "net/instaweb/rewriter/public/webscale_make_scripts_defer.h"
#include "pagespeed/kernel/html/html_element.h"
#include "pagespeed/kernel/html/html_name.h"
#include "pagespeed/kernel/html/html_node.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/util/re2.h"

namespace net_instaweb {

WebscaleMakeScriptsDefer::WebscaleMakeScriptsDefer(RewriteDriver* rewrite_driver)
    : CommonFilter(rewrite_driver) {
  escaped_urls = ConstructPatternFromCustomUrls(rewrite_driver->options());
}

WebscaleMakeScriptsDefer::~WebscaleMakeScriptsDefer(){
}

void WebscaleMakeScriptsDefer::InitStats(Statistics* statistics) {
}


void WebscaleMakeScriptsDefer::StartDocumentImpl() {
}


void WebscaleMakeScriptsDefer::StartElementImpl(HtmlElement* element) {
  MessageHandler* message_handler = driver()->message_handler();

  // Script element found.
  if (element->keyword() == HtmlName::kScript) {
    const char* src_attribute = element->EscapedAttributeValue(HtmlName::kSrc);
    // Script element has a 'src' attribute.
    if (src_attribute != NULL) {
      // If there are no custom urls mentioned, print a message and do nothing.
      if (escaped_urls == "") {
        message_handler->Message(kInfo, "No custom urls provided.");
      }
      else {
        // A full match of the custom url needs to be found.
        bool match = RE2::FullMatch(src_attribute, escaped_urls.c_str());
        if (match) {
          // If a match is found, add the defer attribute.
          driver()->AddAttribute(element, HtmlName::kDefer, "true");
          // Set the debug comment so that it will be displayed when
          // ModPagespeedFilters=+debug is used.
          driver()->InsertDebugComment("Webscale added a defer attribute successfully", element);
          message_handler->Message(kInfo, "Adding a defer attribute to %s.", src_attribute);
        } else {
            message_handler->Message(kInfo, "Not adding a defer attribute to %s.", src_attribute);
        }
      }
    }
  }
}

// Construct a regular expression with the custom urls provied. Each custom
// url will be escaped and an OR of all the escaped urls will be constructed
// for pattern matching.  Pattern matching is preferred here instead of
// iterating the list of custom urls every time a src attribute is encountered.
// This make comparison faster.
GoogleString WebscaleMakeScriptsDefer::ConstructPatternFromCustomUrls(const RewriteOptions* options) {
  const int number_of_custom_urls = options->num_custom_defer_urls();
  GoogleString pattern = "";
  GoogleString prefix = "";

  for(int i = 0; i < number_of_custom_urls; i++) {
    pattern += prefix + RE2::QuoteMeta(options->custom_defer_url(i)->c_str());
    prefix = '|';
  }
  return pattern;
}


void WebscaleMakeScriptsDefer::EndElementImpl(HtmlElement* element) {
}

void WebscaleMakeScriptsDefer::EndDocument() {
}

}
