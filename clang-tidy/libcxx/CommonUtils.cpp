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


bool getMacroAndArgLocationList(SourceManager &SM, ASTContext &Context,
                                SourceLocation Loc, std::vector<MacroInfo> &InfoList) {
  assert(Loc.isMacroID() && "Only reasonble to call this on macros");
  assert(InfoList.empty());
  MacroInfo Info;

  SourceLocation LastLoc = Loc;

  // Find the location of the immediate macro expansion.
  while (true) {
    std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(LastLoc);
    const SrcMgr::SLocEntry *E = &SM.getSLocEntry(LocInfo.first);
    if (!E->isExpansion())
      return !InfoList.empty();

    const SrcMgr::ExpansionInfo &Expansion = E->getExpansion();

    SourceLocation OldArgLoc = LastLoc;
    LastLoc = Expansion.getExpansionLocStart();

    Info.ArgLoc = Expansion.getExpansionLocStart();
    Info.MacroLoc = Expansion.getExpansionLocEnd();
    Info.ExpansionRange = Expansion.getExpansionLocRange();
    assert(Info.ExpansionRange.isTokenRange());
    SourceLocation EndLoc = Lexer::getLocForEndOfToken(
        Expansion.getExpansionLocStart(), 0, SM, Context.getLangOpts());
    Info.ExpansionRange.setEnd(EndLoc);
    Info.ExpansionRange.setTokenRange(false);

    if (!Expansion.isMacroArgExpansion()) {
      Info.Name =
          Lexer::getImmediateMacroName(OldArgLoc, SM, Context.getLangOpts());
    }

    Info.MacroLoc = SM.getExpansionRange(Info.ArgLoc).getBegin();
    Info.ArgLoc = Expansion.getSpellingLoc().getLocWithOffset(LocInfo.second);

    // If spelling location resides in the same FileID as macro expansion
    // location, it means there is no inner macro.
    FileID MacroFID = SM.getFileID(Info.MacroLoc);
    if (SM.isInFileID(Info.ArgLoc, MacroFID)) {
      // Don't transform this case. If the characters that caused the
      // null-conversion come from within a macro, they can't be changed.
      return false;
    }

    InfoList.push_back(Info);
    Info = MacroInfo{};
  }

  llvm_unreachable("getMacroAndArgLocations");
}

bool getMacroAndArgLocations(SourceManager &SM, ASTContext &Context,
                             SourceLocation Loc, MacroInfo &Info) {
  assert(Loc.isMacroID() && "Only reasonble to call this on macros");
  Info = MacroInfo{};
  Info.ArgLoc = Loc;

  // Find the location of the immediate macro expansion.
  while (true) {
    std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(Info.ArgLoc);
    const SrcMgr::SLocEntry *E = &SM.getSLocEntry(LocInfo.first);
    const SrcMgr::ExpansionInfo &Expansion = E->getExpansion();

    SourceLocation OldArgLoc = Info.ArgLoc;
    Info.ArgLoc = Expansion.getExpansionLocStart();
    Info.MacroLoc = Expansion.getExpansionLocEnd();
    Info.ExpansionRange = Expansion.getExpansionLocRange();
    assert(Info.ExpansionRange.isTokenRange());
    SourceLocation EndLoc = Lexer::getLocForEndOfToken(
        Expansion.getExpansionLocStart(), 0, SM, Context.getLangOpts());
    Info.ExpansionRange.setEnd(EndLoc);
    Info.ExpansionRange.setTokenRange(false);

    if (!Expansion.isMacroArgExpansion()) {
      Info.Name =
          Lexer::getImmediateMacroName(OldArgLoc, SM, Context.getLangOpts());
      return true;
    }

    Info.MacroLoc = SM.getExpansionRange(Info.ArgLoc).getBegin();
    Info.ArgLoc = Expansion.getSpellingLoc().getLocWithOffset(LocInfo.second);

    if (Info.ArgLoc.isFileID())
      return true;

    // If spelling location resides in the same FileID as macro expansion
    // location, it means there is no inner macro.
    FileID MacroFID = SM.getFileID(Info.MacroLoc);
    if (SM.isInFileID(Info.ArgLoc, MacroFID)) {
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


CharSourceRange getWhitespaceCorrectRange(SourceManager &SM,
                                          CharSourceRange Range) {
  // FIXME(EricWF): Remove leading whitespace when removing a token.
  bool Invalid = false;
  const char *TextAfter = SM.getCharacterData(Range.getEnd(), &Invalid);
  if (Invalid) {
    return Range;
  }
  std::string After(TextAfter, 25);
  unsigned Offset = std::strspn(TextAfter, " \t\r\n");
  CharSourceRange NewRange = Range;
  NewRange.setEnd(Range.getEnd().getLocWithOffset(Offset));
  return NewRange;
}


std::string nameWithTrailingSpace(SourceManager &SM, SourceLocation Loc,
                                  StringRef Str) {
  std::string Res = Str.data();

  bool Invalid;
  const char *TextAfter =
      SM.getCharacterData(Loc.getLocWithOffset(1), &Invalid);

  if (!Invalid && *TextAfter != '\n')
    Res += " ";
  return Res;
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
