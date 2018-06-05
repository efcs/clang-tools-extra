//===--- CommonUtils.cpp - clang-tidy----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include <string>

namespace clang {
namespace tidy {
namespace libcxx {

bool IsFromStdNamespace(const Decl *Dcl) {
  const DeclContext *D = Dcl->getDeclContext();
  while (D->isRecord())
    D = D->getParent();
  return D->isStdNamespace();
}

bool getMacroAndArgLocations(SourceManager &SM, ASTContext &Context,
                             SourceLocation Loc, SourceLocation &ArgLoc,
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
      Name = Lexer::getImmediateMacroName(OldArgLoc, SM, Context.getLangOpts());

      return true;
    }

    MacroLoc = SM.getExpansionRange(ArgLoc).getBegin();

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

bool isInLibcxxHeaderFile(const clang::SourceManager &SM,
                          const clang::Decl *D) {
  auto ExpansionLoc = SM.getFileLoc(D->getLocation()); //SM.getExpansionLoc(D->getLocStart());
  assert(D->getLocation() == SM.getExpansionLoc(D->getLocation()));
  if (ExpansionLoc.isInvalid())
    return false;
  const auto *FE = SM.getFileEntryForID(SM.getFileID(ExpansionLoc));
  if (!FE)
    return false;
  StringRef Name = FE->getName();
  if (Name.endswith_lower(".cpp"))
    return false;
  return true;
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
