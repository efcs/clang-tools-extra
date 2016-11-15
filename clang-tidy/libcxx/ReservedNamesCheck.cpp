//===--- ReservedNamesCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReservedNamesCheck.h"
#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {


static std::string getNewName(const IdentifierInfo *Info) {
  assert(Info && Info->getLength() != 0);
  const auto Len = Info->getLength();
  const char* Name = Info->getNameStart();
  bool HasLeadingUnderscore = Name[0] == '_';
  auto isLower = [](char ch) { return ch >= 'a' && ch <= 'z'; };
  auto isUpper = [](char ch) { return ch >= 'A' && ch <= 'Z'; };
  if (HasLeadingUnderscore) {
    if (Len == 1)
      return "";
    assert(Name[1] != '_');
    if (isUpper(Name[1])) {
      return "";
    }
    else if (isLower(Name[1])) {
      return "";
    } else {
      return "";
    }
  } else {
    if (isLower(Name[0])) {
      std::string NewName = "__";
      NewName += Name;
      return NewName;
    }
  }
  return "";
}
AST_MATCHER(Decl, isLocalOrParm) {
  if (auto *VD = dyn_cast<VarDecl>(&Node))
    return VD->isLocalVarDeclOrParm();
  return false;
}


void ReservedNamesCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(namedDecl(
          isFromStdNamespace(), unless(isReservedOrUnnamed()),
          anyOf(
                  isPrivate(),
                  nonTypeTemplateParmDecl(),
                  templateTypeParmDecl(),
                  isLocalOrParm())
  ).bind("decl"), this);
}

void ReservedNamesCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *MatchedDecl = Result.Nodes.getNodeAs<NamedDecl>("decl");
  const std::string NewName = getNewName(MatchedDecl->getIdentifier());
  auto *DC = MatchedDecl->getDeclContext();
  if (auto *VD = dyn_cast<ParmVarDecl>(MatchedDecl)) {
    auto *FD = cast<FunctionDecl>(DC);
    assert(FD);
    if (!FD->isThisDeclarationADefinition() && NewName != "") {
      SourceRange R(VD->getInnerLocStart(), VD->getLocEnd());
      diag(MatchedDecl->getLocation(), "use of non-reserved name %0")
      << MatchedDecl->getSourceRange() << MatchedDecl
      << FixItHint::CreateReplacement(R, NewName.c_str());
      return;
    }
  }
  diag(MatchedDecl->getLocation(), "use of non-reserved name %0")
      << MatchedDecl->getSourceRange() << MatchedDecl;
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
