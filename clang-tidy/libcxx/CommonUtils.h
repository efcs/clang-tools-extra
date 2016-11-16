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

inline bool IsFromStdNamespace(const Decl *Dec) {
  const DeclContext *D = Dec->getDeclContext();
  D = D->getEnclosingNamespaceContext();
  if (!D)
    return false;
  while (D->isInlineNamespace() ||
         (D->getParent() && D->getParent()->isNamespace()))
    D = D->getParent();
  if (!D->isNamespace() || !D->getParent()->isTranslationUnit())
    return false;
  const IdentifierInfo *Info = cast<NamespaceDecl>(D)->getIdentifier();
  return (Info && Info->isStr("std"));
}
AST_MATCHER(NamedDecl, isFromStdNamespace) { return IsFromStdNamespace(&Node); }

inline bool hasReservedName(const NamedDecl *D) {
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

inline bool hasNonReservedName(const NamedDecl *D) {
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

inline bool isReservedName(StringRef Name) {
  if (Name.size() < 2)
    return false;
  return Name[0] == '_' &&
         (Name[1] == '_' || (Name[1] >= 'A' && Name[1] <= 'Z'));
}

inline bool hasGoodReservedName(const NamedDecl *D) {
  const IdentifierInfo *Info = D->getIdentifier();
  if (!Info || Info->getLength() == 0)
    return true;
  // don't allow identifiers fewer than 3 characters.
  if (Info->getLength() < 3)
    return false;
  return isReservedName(Info->getNameStart());
}

inline bool IsAllowableReservedName(StringRef Name,
                                    const NamedDecl *ND = nullptr) {
  if (Name.size() == 0)
    return true;
  if (Name.size() < 3)
    return false;
  if (isReservedName(Name))
    return true;
  if (Name == "type" && ND && isa<TypedefNameDecl>(ND))
    return true;
  const auto *VD = dyn_cast_or_null<VarDecl>(ND);
  if (Name == "value" && VD && VD->isStaticDataMember())
    return true;
  return false;
}

inline bool IsAllowableReservedName(const NamedDecl *ND) {
  assert(ND);
  const IdentifierInfo *Info = ND->getIdentifier();
  if (!Info)
    return true;
  return IsAllowableReservedName(Info->getNameStart(), ND);
}

AST_MATCHER(NamedDecl, isAllowableReservedName) {
  return IsAllowableReservedName(&Node);
}

AST_MATCHER(DeclRefExpr, declRefIsAllowableReservedName) {
  const auto *VD = Node.getDecl();
  assert(VD);
  return IsAllowableReservedName(VD);
}

bool getMacroAndArgLocations(SourceManager &SM, ASTContext &Context,
                             SourceLocation Loc, SourceLocation &ArgLoc,
                             SourceLocation &MacroLoc, StringRef &Name);

bool isInLibcxxHeaderFile(const SourceManager &SM, const Decl *D);

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_COMMON_UTILS_H
