//===--- HeaderGuardCheck.h - clang-tidy ------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_CHECK_UTIL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_CHECK_UTIL_H

#include "../ClangTidy.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Debug.h"


namespace clang {
namespace tidy {
namespace libcxx {


/// Determine whether \p Id is a name reserved for the implementation (C99
/// 7.1.3, C++ [lib.global.names]).
static inline bool isReservedName(const IdentifierInfo *Id) {
  if (Id->getLength() < 2)
    return false;
  const char *Name = Id->getNameStart();
  return Name[0] == '_' &&
         (Name[1] == '_' || (Name[1] >= 'A' && Name[1] <= 'Z'));
}

static inline bool isInStdNamespace(const Decl *D) {
  const DeclContext *DC = D->getDeclContext()->getEnclosingNamespaceContext();
  const NamespaceDecl *ND = dyn_cast<NamespaceDecl>(DC);
  if (!ND)
    return false;

  while (const NamespaceDecl *Parent = dyn_cast<NamespaceDecl>(ND->getParent())) {
    if (!isa<NamespaceDecl>(Parent))
      break;
    ND = cast<NamespaceDecl>(Parent);
  }

  return ND->isStdNamespace();
}

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_CHECK_UTIL_H
