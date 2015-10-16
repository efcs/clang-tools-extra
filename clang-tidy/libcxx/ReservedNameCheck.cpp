//===--- LibcxxTemplateParameterNameCheck.cpp - clang-tidy -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReservedNameCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Debug.h"

using namespace clang::ast_matchers;

namespace clang {

// FIXME
namespace {
AST_MATCHER(VarDecl, isLocalVarDeclOrParm) {
  return Node.isLocalVarDeclOrParm();
}
} // end namespace

namespace tidy {
namespace libcxx {

/// Determine whether \p Id is a name reserved for the implementation (C99
/// 7.1.3, C++ [lib.global.names]).
static bool isReservedName(const IdentifierInfo *Id) {
  if (Id->getLength() < 2)
    return false;
  const char *Name = Id->getNameStart();
  return Name[0] == '_' &&
         (Name[1] == '_' || (Name[1] >= 'A' && Name[1] <= 'Z'));
}


void
ReservedNameCheck::registerMatchers(ast_matchers::MatchFinder *Finder) {
  Finder->addMatcher(
        namedDecl(anyOf(
          templateTypeParmDecl(),
          nonTypeTemplateParmDecl(),
          varDecl(isLocalVarDeclOrParm()))).bind("decl"),
      this);
}

void
ReservedNameCheck::check(const MatchFinder::MatchResult &Result){
  const auto* TD = Result.Nodes.getNodeAs<NamedDecl>("decl");
  if (isReservedName(TD->getIdentifier()))
    return;

  diag(TD->getLocStart(), "non-reserved name '%0' used as identifier")
    << TD->getName();
}


} // namespace libcxx
} // namespace tidy
} // namespace clang
