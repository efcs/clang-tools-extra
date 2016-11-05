//===--- TemplateMissingInlineCheck.cpp - clang-tidy-----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TemplateMissingInlineCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {


AST_MATCHER(FunctionDecl, isInlineFunc) {
  return Node.isInlined();
}
AST_MATCHER(FunctionDecl, isTemplatedDecl) {
  if (auto *M = dyn_cast<CXXMethodDecl>(&Node)) {
    if (M->getParent()->getDescribedTemplate())
      return true;
  }
  return Node.getTemplatedKind() != FunctionDecl::TK_NonTemplate;
}
void TemplateMissingInlineCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(functionDecl(isDefinition(), unless(isInlineFunc()),
            isTemplatedDecl()
  ).bind("func"), this);
}

void TemplateMissingInlineCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  const auto *First = FD->getCanonicalDecl();

  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;
  //SourceLocation StartLoc = FD->getInnerLocStart();
  //SourceLocation PreviousLocation = StartLoc.getLocWithOffset(-1);
  //bool NeedsSpace = isAlphanumeric(*SM.getCharacterData(PreviousLocation));
  diag(First->getLocation(), "function template %0 is not inline")
      << First
      << FixItHint::CreateInsertion(First->getInnerLocStart(), "inline ");
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
