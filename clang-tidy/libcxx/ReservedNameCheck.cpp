//===--- LibcxxTemplateParameterNameCheck.cpp - clang-tidy -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReservedNameCheck.h"
#include "LibcxxCheckUtil.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Debug.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {


namespace {
AST_MATCHER(VarDecl, isLocalVarDeclOrParm) {
  return Node.isLocalVarDeclOrParm();
}
AST_MATCHER(NamedDecl, hasIdentifier) {
  return Node.getIdentifier() != nullptr;
}
AST_MATCHER(Decl, isWithinStdNamespaceMatcher) {
  return isInStdNamespace(cast<Decl>(&Node));
}
}

void
ReservedNameCheck::registerMatchers(ast_matchers::MatchFinder *Finder) {
  Finder->addMatcher(
        namedDecl(anyOf(
          templateTypeParmDecl(),
          nonTypeTemplateParmDecl(),
          varDecl(isLocalVarDeclOrParm()),
          fieldDecl(isPrivate()),
          cxxMethodDecl(isPrivate())),
          hasIdentifier(),
          isWithinStdNamespaceMatcher()).bind("decl"),
      this);
}

void
ReservedNameCheck::check(const MatchFinder::MatchResult &Result){
  const auto* TD = Result.Nodes.getNodeAs<NamedDecl>("decl");
  const IdentifierInfo *II = TD->getIdentifier();

  if (!II || isReservedName(II))
    return;
  SourceLocation L = TD->getLocation();

  StringRef Filename = Result.SourceManager->getFilename(
          Result.SourceManager->getSpellingLoc(L));
  if (Filename.endswith(".cpp"))
    return;

  diag(L, "non-reserved name '%0' used as identifier")
    << TD->getName();
}


} // namespace libcxx
} // namespace tidy
} // namespace clang
