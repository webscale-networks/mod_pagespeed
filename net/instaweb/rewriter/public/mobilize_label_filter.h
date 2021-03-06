/*
 * Copyright 2014 Google Inc.
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

// Author: jmaessen@google.com (Jan-Willem Maessen)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_MOBILIZE_LABEL_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_MOBILIZE_LABEL_FILTER_H_

#include <set>
#include <vector>

#include "net/instaweb/rewriter/mobilize_labeling.pb.h"
#include "net/instaweb/rewriter/public/mobilize_decision_trees.h"
#include "net/instaweb/rewriter/public/mobilize_filter_base.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/proto_util.h"
#include "pagespeed/kernel/base/scoped_ptr.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/html/html_element.h"
#include "pagespeed/kernel/html/html_node.h"
#include "pagespeed/kernel/html/html_parse.h"

namespace net_instaweb {

// Sample capturing the feature vector for a given DOM element.  We compute
// these up the DOM tree, aggregating into the parent when each child finishes.
// We also keep a global root sample so we can normalize statistics, and so that
// every actual DOM sample has a parent.
//
// Every feature is represented by a double entry in the feature vector f.
// Features ending in "Percent" have values between 0 and 100.0 and are computed
// at end of document by ComputeProportionalFeatures.  All other features are
// non-negative integers in practice.  We don't need the precision of doubles,
// but we do need the dynamic integer range or counters will peg.
struct ElementSample {
  ElementSample(int relevant_tag_depth, int tag_count,
                int content_bytes, int content_non_blank_bytes);

  // Here normalized represents 100 / global measurement, used
  // as a multiplier to compute percent features.
  void ComputeProportionalFeatures(ElementSample* normalized);
  GoogleString ToString(bool readable, HtmlParse* parser);

  HtmlElement* element;          // NULL for global count
  GoogleString id;               // id of *element, which might be flushed.
  ElementSample* parent;         // NULL for global count
  MobileRole::Level role;        // Mobile role (from parent where applicable)
  MobileRole::Level propagated_role;  // Mobile role from children during label
  bool explicitly_labeled;       // Was this DOM element explicitly labeled?
  bool explicitly_non_nav;       // Element or transitive ancestor NOT nav?
  std::vector<double> features;  // feature vector, always of size kNumFeatures.
};

// Classify DOM elements by adding data-mobile-role= attributes and / or adding
// them to a labeling protobuf so that the MoblizeRewriteFilter can rewrite them
// to be mobile-friendly.  The classes are:
//   Navigational: things like nav and menu bars, mostly in the header
//   Header: Page title, title image, logo associated with page, etc.
//   Content: The content we think the user wants to see.
//   Marginal: Other stuff on the page that typically resides in the margins,
//     header, or footer.
// We do this bottom-up, since we want to process children in a streaming
// fashion before their parent's close tag.  We take the presence of html5 tags
// as authoritative; note that we've assumed that they're authoritative in
// training our classifiers.
class MobilizeLabelFilter : public MobilizeFilterBase {
 public:
  typedef protobuf::RepeatedPtrField<GoogleString> MobilizationIds;
  // Monitoring variable names
  static const char kPagesLabeled[];  // Pages run through labeler.
  static const char kPagesRoleAdded[];
  static const char kNavigationalRoles[];
  static const char kHeaderRoles[];
  static const char kContentRoles[];
  static const char kMarginalRoles[];
  static const char kDivsUnlabeled[];
  static const char kAmbiguousRoleLabels[];
  // Property cache tag
  static const char kMobilizeLabeling[];

  MobilizeLabelFilter(bool is_menu_subfetch, RewriteDriver* driver);
  virtual ~MobilizeLabelFilter();

  static void InitStats(Statistics* statistics);
  static const MobilizationIds* IdsForRole(
      const MobilizeLabeling& labeling, MobileRole::Level role);

  virtual void DetermineEnabled(GoogleString* disabled_reason);
  // Get the computed labeling (which might have been fetched from the pcache).
  // NULL if no labeling has been computed or nothing can be labeled.
  const MobilizeLabeling* labeling() const { return labeling_.get(); }
  virtual const char* Name() const { return "MobilizeLabel"; }

 private:
  static MobilizationIds* MutableIdsForRole(
      MobilizeLabeling* labeling, MobileRole::Level role);
  void Init();
  virtual void StartDocumentImpl();
  virtual void StartNonSkipElement(
      MobileRole::Level role_attribute, HtmlElement* element);
  virtual void EndNonSkipElement(HtmlElement* element);
  virtual void Characters(HtmlCharactersNode* characters);
  virtual void EndDocumentImpl();
  void GetClassesFromOptions(const RewriteOptions* options);
  void HandleElementWithMetadata(
      MobileRole::Level role_attribute, HtmlElement* element);
  void HandleDivLikeElement(HtmlElement* element, MobileRole::Level role);
  void HandleExplicitlyConfiguredElement(HtmlElement* element);
  void ExplicitlyConfigureRole(MobileRole::Level role, HtmlElement* element);
  ElementSample* MakeNewSample(HtmlElement* element);
  void PopSampleStack();
  void ComputeContained(ElementSample* sample);
  void AggregateToTopOfStack(ElementSample* sample);
  void IncrementRelevantTagDepth();
  void SanityCheckEndOfDocumentState();
  void ComputeProportionalFeatures();
  void Label();
  void CreateLabeling();
  void DebugLabel();
  void UnlabelledDiv(ElementSample* sample);
  void InjectLabelJavascript();
  void NonMobileUnlabel();
  void DeletePagespeedId(HtmlElement* element);

  bool is_menu_subfetch_;
  bool compute_signals_;
  bool keep_label_ids_;

  int relevant_tag_depth_;
  int max_relevant_tag_depth_;
  int link_depth_;
  int tag_count_;
  int content_bytes_;
  int content_non_blank_bytes_;
  bool were_roles_added_;

  std::vector<ElementSample*> samples_;  // in document order
  std::vector<ElementSample*> sample_stack_;

  scoped_ptr<MobilizeLabeling> labeling_;
  std::set<StringPiece> label_ids_;  // refers to labeling_

  // The following two vectors are parsed from
  // RewriteOptions::mob_nav_elements(), which outlives them.
  std::set<StringPiece> nav_classes_;
  std::set<StringPiece> non_nav_classes_;

  Variable* pages_labeled_;
  Variable* pages_role_added_;
  Variable* role_variables_[MobileRole::kInvalid];
  Variable* divs_unlabeled_;
  Variable* ambiguous_role_labels_;

  DISALLOW_COPY_AND_ASSIGN(MobilizeLabelFilter);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_MOBILIZE_LABEL_FILTER_H_
