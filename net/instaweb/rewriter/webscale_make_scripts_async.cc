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
#include "net/instaweb/rewriter/public/webscale_make_scripts_async.h"
#include "pagespeed/kernel/html/html_element.h"
#include "pagespeed/kernel/html/html_name.h"
#include "pagespeed/kernel/html/html_node.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/util/re2.h"

namespace net_instaweb {

WebscaleMakeScriptsAsync::WebscaleMakeScriptsAsync(RewriteDriver* rewrite_driver, MessageHandler* message_handler)
    : CommonFilter(rewrite_driver) {
  Statistics* statistics = rewrite_driver->statistics();
  message_handler = driver()->message_handler();
  escaped_urls = ConstructPatternFromCustomUrls(rewrite_driver->options());
}

WebscaleMakeScriptsAsync::~WebscaleMakeScriptsAsync(){
}

void WebscaleMakeScriptsAsync::InitStats(Statistics* statistics) {
}


void WebscaleMakeScriptsAsync::StartDocumentImpl() {
}


void WebscaleMakeScriptsAsync::StartElementImpl(HtmlElement* element) {
  message_handler = driver()->message_handler();

  // Script element found.
  if (element->keyword() == HtmlName::kScript) {
    const char* src_attribute = element->EscapedAttributeValue(HtmlName::kSrc);
    // Script element has a 'src' attribute.
    if (src_attribute != NULL) {
      // Convert the src element to a GoogleString format for compatibility.
      GoogleString* src_string = new GoogleString(src_attribute);

      // If there are no custom urls mentioned, print a message and do nothing.
      if (escaped_urls == "") {
        message_handler->Message(
          kInfo, "No custom urls provided.");
      }
      else {
        // A full match of the custom url needs to be found.
        bool match = RE2::FullMatch(src_string->c_str(), escaped_urls.c_str());
        if (match) {
          // If a match is found, add the async attribute.
          driver()->AddAttribute(element, HtmlName::kAsync, "true");
          // Set the debug comment so that it will be displayed when
          // ModPagespeedEnabledFilters=+debug is used.
          driver()->InsertDebugComment("Webscale added an async attribute successfully", element);
          message_handler->Message(
            kInfo, "Adding an async attribute to %s.", src_string->c_str());
        } else {
            message_handler->Message(
              kInfo, "Not adding an async attribute to %s.", src_string->c_str());
        }
      }
    }
  }
}

// Construct a regular expressions with the custom urls provied. Each custom
// url will be escaped and an OR of all the escaped urls will be constructed
// for pattern matching.  Pattern matching is preferred here instead of
// iterating the list of cusotm urls every time a src attribute is encoutered.
// This make comparison faster.
GoogleString WebscaleMakeScriptsAsync::ConstructPatternFromCustomUrls(const RewriteOptions* options) {
  const int number_of_custom_urls = options->num_custom_async_urls();
  GoogleString pattern = "";

  if (number_of_custom_urls == 0) {
    return pattern;
  }
  else {
    for(int i = 0; i < number_of_custom_urls - 1; i++) {
      pattern += RE2::QuoteMeta(options->custom_async_url(i)->c_str());
      pattern += '|';
    }
    pattern += RE2::QuoteMeta(options->custom_async_url(number_of_custom_urls - 1)->c_str());
    return pattern;
  }
}


void WebscaleMakeScriptsAsync::EndElementImpl(HtmlElement* element) {
}

void WebscaleMakeScriptsAsync::EndDocument() {
}

}
