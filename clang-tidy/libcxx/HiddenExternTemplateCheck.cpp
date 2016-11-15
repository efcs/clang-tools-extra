//===--- HiddenExternTemplateCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "HiddenExternTemplateCheck.h"
#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {

AST_POLYMORPHIC_MATCHER(isExplicitInstantiation,
                        AST_POLYMORPHIC_SUPPORTED_TYPES(FunctionDecl, VarDecl,
                                                        CXXRecordDecl, ClassTemplateSpecializationDecl)) {
  return (Node.getTemplateSpecializationKind() == TSK_ExplicitInstantiationDeclaration ||
          Node.getTemplateSpecializationKind() ==
          TSK_ExplicitInstantiationDefinition);
}

AST_POLYMORPHIC_MATCHER(hasHiddenVisibility,
                        AST_POLYMORPHIC_SUPPORTED_TYPES(FunctionDecl, CXXMethodDecl)) {
  auto *Fn = dyn_cast<FunctionDecl>(&Node);
  Fn = Fn->getTemplateInstantiationPattern();
  if (!Fn)
    return false;
  if (auto *VS = Fn->template getAttr<VisibilityAttr>())
    if (VS->getVisibility() == VisibilityAttr::Hidden)
      return true;

  for (auto *RD : Fn->redecls()) {
    if (auto *VS = RD->template getAttr<VisibilityAttr>()) {
      if (VS->getVisibility() == VisibilityAttr::Hidden)
        return true;
    }
  }
  return false;
}

void HiddenExternTemplateCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(functionDecl(
            isExplicitInstantiation(),
            hasHiddenVisibility()
            ).bind("func"), this);
}

void HiddenExternTemplateCheck::performFixIt(FunctionDecl *FD,
                                             SourceManager &SM, ASTContext &Context) {
    auto *VS = FD->getAttr<VisibilityAttr>();
    if (!VS)
      return;
    SourceLocation MacroLoc, ArgLoc;
    StringRef Name;
    if (!getMacroAndArgLocations(SM, Context, VS->getLocation(), ArgLoc, MacroLoc, Name))
      return;
    if (Name != "_LIBCPP_INLINE_VISIBILITY" && Name != "_LIBCPP_ALWAYS_INLINE" && Name != "HIDDEN")
      return;

    assert(ArgLoc.isValid());
    CharSourceRange Range(ArgLoc, true);
    diag(ArgLoc, "function %0 is explicitly instantiated and hidden")
        << FD
        << FixItHint::CreateReplacement(Range, "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY");
}

void HiddenExternTemplateCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("func");
  auto *Fn = MatchedDecl->getTemplateInstantiationPattern();


  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;

  performFixIt(Fn, SM, Context);
  for (FunctionDecl *RD : Fn->redecls()) {
    performFixIt(RD, SM, Context);

  }
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
