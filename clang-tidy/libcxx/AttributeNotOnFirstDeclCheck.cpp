//===--- AttributeNotOnFirstDeclCheck.cpp - clang-tidy---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AttributeNotOnFirstDeclCheck.h"
#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include <string>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {

static bool isInstantiationOf(const FunctionDecl *Pattern,
                              const FunctionDecl *Instance) {
  if (!Pattern)
    return false;
  Pattern = Pattern->getCanonicalDecl();

  do {
    Instance = Instance->getCanonicalDecl();
    if (Pattern == Instance) return true;
    Instance = Instance->getInstantiatedFromMemberFunction();
  } while (Instance);

  return false;
}

AST_MATCHER(FunctionDecl, isRedeclaration) {
  if (auto *FD = dyn_cast<FunctionDecl>(&Node)) {
    if (FD->isTemplateInstantiation())
      return false;

    auto TSK = FD->getTemplateSpecializationKind();
    if (TSK == TSK_ExplicitInstantiationDefinition
            || TSK == TSK_ExplicitInstantiationDeclaration)
      return false;
    auto *NewFD = FD->getCanonicalDecl();
    if (NewFD == FD)
      return false;
    TSK = NewFD->getTemplateSpecializationKind();
    if (TSK == TSK_ExplicitInstantiationDefinition
            || TSK == TSK_ExplicitInstantiationDeclaration)
      return false;
    return true;
  }
  return false;
}




void AttributeNotOnFirstDeclCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(functionDecl(allOf(
          hasAttr(attr::Visibility),
          isDefinition(),
          unless(isInstantiated()),
         isRedeclaration())).bind("decl"), this);
}

void AttributeNotOnFirstDeclCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("decl");
  auto *VS = FD->getAttr<VisibilityAttr>();
  assert(FD && VS);
  const FunctionDecl *First = FD->getCanonicalDecl();
  assert(First);
  SourceLocation FixItLoc = First->getInnerLocStart();
  if (!FixItLoc.isValid()) {
    if (!First->isReplaceableGlobalAllocationFunction())
      First->dumpColor();
    return;
  }
  if (isInstantiationOf(First->getTemplateInstantiationPattern(), FD))
    return;
  if (First->getLocation() == FD->getLocation())
    return;
  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;
  MacroInfo Info;
  if (!getMacroAndArgLocations(SM, Context, VS->getLocation(), Info))
    return;
  std::string NamePlus = Info.Name;
  NamePlus += ' ';
  Info.Name = NamePlus;
  assert(Info.Name.size() > 1);
  if (FixItLoc.isValid()) {
    if (!First->hasAttr<VisibilityAttr>()) {
      diag(FD->getLocation(), "function %0 was declared without attribute")
          << FD << FixItHint::CreateInsertion(FixItLoc, Info.Name)
          << FixItHint::CreateRemoval(Info.ArgLoc);
    } else {
      diag(FD->getLocation(),
           "function %0 was declared with duplicate attribute")
          << FD << FixItHint::CreateRemoval(Info.ArgLoc);
    }
    diag(First->getLocation(), "first declared here", DiagnosticIDs::Note)
          << First->getSourceRange();
  } else {
     diag(FD->getLocation(), "function %0 was declared without attribute")
        << FD;
     diag(First->getLocation(), "first declared here", DiagnosticIDs::Note)
          << First->getSourceRange();
  }
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
