//===--- ADLCallCheck.cpp - clang-tidy --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ADLCallCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Core/Lookup.h"
using namespace clang::ast_matchers;

namespace clang {

AST_MATCHER(FunctionDecl, hasNoADLAttr) {
  auto *AT = Node.getAttr<NoADLAttr>();
  return AT && !AT->isADLAllowed();
}

namespace tidy {
namespace google {

ADLCallCheck::ADLCallCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      MinimizeReplacementName(
          Options.getLocalOrGlobal("MinimizeReplacementName", true)),
      OmitLeadingQualifier(
          Options.getLocalOrGlobal("OmitLeadingQualifier", true)) {}

void ADLCallCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "MinimizeReplacementName", MinimizeReplacementName);
  Options.store(Opts, "OmitLeadingQualifier", OmitLeadingQualifier);
}

void ADLCallCheck::registerMatchers(ast_matchers::MatchFinder *Finder) {
  // Only register the matchers for C++; the functionality currently does not
  // provide any benefit to other languages, despite being benign.
  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      callExpr(allOf(hasAncestor(decl().bind("context")), usesADL(),
                     callee(functionDecl(hasNoADLAttr()).bind("callee"))))
          .bind("call"),
      this);
}

std::string
ADLCallCheck::getReplacementName(const DeclRefExpr *CalleeRef,
                                 const FunctionDecl *FD,
                                 const DeclContext *CallContext) const {
  if (!MinimizeReplacementName) {
    std::string NewName = FD->getQualifiedNameAsString();
    if (!OmitLeadingQualifier)
      NewName = "::" + NewName;
    return NewName;
  }

  tooling::RenameOptions Opts;
  Opts.OmitLeadingQualifiers = OmitLeadingQualifier;
  Opts.Minimize = MinimizeReplacementName;
  Opts.OmitInlineNamespaces = true;

  std::string NewName =
      clang::tooling::qualifyFunctionNameInContext(CallContext, FD, Opts);
  return NewName;
}

void ADLCallCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *TheCall = Result.Nodes.getNodeAs<CallExpr>("call");
  const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("callee");
  const auto *DC = cast<DeclContext>(Result.Nodes.getNodeAs<Decl>("context"));
  assert(DC);

  const auto *CalleeRef =
      llvm::cast<clang::DeclRefExpr>(TheCall->getCallee()->IgnoreImplicit());

  auto CalleeRange = clang::CharSourceRange::getTokenRange(
      TheCall->getCallee()->getSourceRange());
  if (CalleeRef->hasExplicitTemplateArgs())
    CalleeRange.setEnd(CalleeRef->getLAngleLoc().getLocWithOffset(-1));

  std::string NewName = getReplacementName(CalleeRef, FD, DC);

  diag(TheCall->getExprLoc(), "call to function should not use ADL")
      << FixItHint::CreateReplacement(CalleeRange, NewName);
}

} // namespace google
} // namespace tidy
} // namespace clang
