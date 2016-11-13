//===--- AttributeNotOnFirstDeclCheck.cpp - clang-tidy---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AttributeNotOnFirstDeclCheck.h"
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


static bool getMacroAndArgLocations(SourceManager &SM,  ASTContext &Context,
                                    SourceLocation Loc,
                                    SourceLocation &ArgLoc,
                                        SourceLocation &MacroLoc, StringRef &Name) {
    assert(Loc.isMacroID() && "Only reasonble to call this on macros");

    ArgLoc = Loc;

    // Find the location of the immediate macro expansion.
    while (true) {
      std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(ArgLoc);
      const SrcMgr::SLocEntry *E = &SM.getSLocEntry(LocInfo.first);
      const SrcMgr::ExpansionInfo &Expansion = E->getExpansion();

      SourceLocation OldArgLoc = ArgLoc;
      ArgLoc = Expansion.getExpansionLocStart();
      MacroLoc = Expansion.getExpansionLocEnd();
      if (!Expansion.isMacroArgExpansion()) {
         Name =
            Lexer::getImmediateMacroName(OldArgLoc, SM, Context.getLangOpts());

        return true;
      }

      MacroLoc = SM.getExpansionRange(ArgLoc).first;

      ArgLoc = Expansion.getSpellingLoc().getLocWithOffset(LocInfo.second);
      if (ArgLoc.isFileID())
        return true;

      // If spelling location resides in the same FileID as macro expansion
      // location, it means there is no inner macro.
      FileID MacroFID = SM.getFileID(MacroLoc);
      if (SM.isInFileID(ArgLoc, MacroFID)) {
        // Don't transform this case. If the characters that caused the
        // null-conversion come from within a macro, they can't be changed.
        return false;
      }
    }

    llvm_unreachable("getMacroAndArgLocations");
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
  SourceLocation ArgLoc, MacroLoc;
  StringRef Name;
  if (!getMacroAndArgLocations(SM, Context, VS->getLocation(), ArgLoc, MacroLoc, Name))
    return;
  std::string NamePlus = Name;
  NamePlus += ' ';
  Name = NamePlus;
  assert(Name.size() > 1);
  if (FixItLoc.isValid()) {
    if (!First->hasAttr<VisibilityAttr>()) {
      diag(FD->getLocation(), "function %0 was declared without attribute")
          << FD
          << FixItHint::CreateInsertion(FixItLoc, Name)
          << FixItHint::CreateRemoval(ArgLoc);
    } else {
      diag(FD->getLocation(), "function %0 was declared with duplicate attribute")
          << FD
          << FixItHint::CreateRemoval(ArgLoc);
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
