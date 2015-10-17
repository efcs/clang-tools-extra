//===--- LibcxxTemplateParameterNameCheck.cpp - clang-tidy -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UnqualifiedFunctionCallCheck.h"
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
AST_MATCHER(NamespaceDecl, isStdNamespace) {
  return cast<Decl>(&Node)->getDeclContext()->isStdNamespace();
}
}

void
UnqualifiedFunctionCallCheck::registerMatchers(ast_matchers::MatchFinder *Finder) {
  Finder->addMatcher(
        namespaceDecl(isStdNamespace(),
              hasDescendant(
       callExpr(
         unless(anyOf(cxxMemberCallExpr(), cxxOperatorCallExpr())),
         unless(hasDescendant(nestedNameSpecifier()))
         ).bind("decl"))), this);
}

void
UnqualifiedFunctionCallCheck::check(const MatchFinder::MatchResult &Result) {
  auto CE = Result.Nodes.getNodeAs<CallExpr>("decl");
  auto Callee = CE->getCalleeDecl();
  if (!Callee)
    return;
  auto NC = dyn_cast<NamedDecl>(Callee);
  if (!NC || !NC->getIdentifier())
    return;
  NC->dump();

}


} // namespace libcxx
} // namespace tidy
} // namespace clang
