//===--- CommonUtils.h - clang-tidy------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_COMMON_UTILS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_COMMON_UTILS_H

#include "../ClangTidy.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace clang {
namespace tidy {
namespace libcxx {

/// Matches declarations whose declaration context is the C++ standard library
/// namespace std.
///
/// Note that inline namespaces are silently ignored during the lookup since
/// both libstdc++ and libc++ are known to use them for versioning purposes.
///
/// Given:
/// \code
///   namespace ns {
///     struct my_type {};
///     using namespace std;
///   }
///
///   using std::vector;
///   using ns:my_type;
///   using ns::list;
/// \code
///
/// usingDecl(hasAnyUsingShadowDecl(hasTargetDecl(isFromStdNamespace())))
/// matches "using std::vector" and "using ns::list".
AST_MATCHER(NamedDecl, isFromStdNamespace) {
  const DeclContext *D = Node.getDeclContext();
  D = D->getEnclosingNamespaceContext();
  if (!D)
    return false;
  while (D->isInlineNamespace())
    D = D->getParent();
  if (!D->isNamespace() || !D->getParent()->isTranslationUnit())
    return false;
  const IdentifierInfo *Info = cast<NamespaceDecl>(D)->getIdentifier();
  return (Info && Info->isStr("std"));
}

inline bool hasReservedName(NamedDecl *D) {
  const IdentifierInfo *Info = D->getIdentifier();
  if (!Info || Info->getLength() == 0)
    return true;
  // don't allow identifiers fewer than 3 characters.
  if (Info->getLength() < 2)
    return false;
  const char *Name = Info->getNameStart();
  return Name[0] == '_' &&
         (Name[1] == '_' || (Name[1] >= 'A' && Name[1] <= 'Z'));
}

inline bool hasNonReservedName(NamedDecl *D) {
  const IdentifierInfo *Info = D->getIdentifier();
  if (!Info || Info->getLength() == 0)
    return false;
  // don't allow identifiers fewer than 3 characters.
  if (Info->getLength() < 2)
    return true;
  const char *Name = Info->getNameStart();
  return Name[0] != '_' ||
         !(Name[1] == '_' && (Name[1] >= 'A' && Name[1] <= 'Z'));
}

inline bool hasGoodReservedName(NamedDecl *D) {
  const IdentifierInfo *Info = D->getIdentifier();
  if (!Info || Info->getLength() == 0)
    return true;
  // don't allow identifiers fewer than 3 characters.
  if (Info->getLength() < 3)
    return false;
  const char *Name = Info->getNameStart();
  return Name[0] == '_' &&
         (Name[1] == '_' || (Name[1] >= 'A' && Name[1] <= 'Z'));
}

AST_MATCHER(NamedDecl, isReservedOrUnnamed) {
  return hasGoodReservedName(&Node);
}

bool getMacroAndArgLocations(SourceManager &SM, ASTContext &Context,
                             SourceLocation Loc, SourceLocation &ArgLoc,
                             SourceLocation &MacroLoc, StringRef &Name);

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_COMMON_UTILS_H
