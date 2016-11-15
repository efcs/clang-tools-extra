//===--- ReservedNamesCheck.h - clang-tidy-----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_RESERVED_NAMES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_RESERVED_NAMES_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace libcxx {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/libcxx-reserved-names.html
class ReservedNamesCheck : public ClangTidyCheck {
public:
  ReservedNamesCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  void addReplacableDecl(const NamedDecl *D) { ReplacableDecls.insert(D); }

  bool hasReplacableDecl(const Decl *D) const {
    const auto *ND = dyn_cast<NamedDecl>(D);
    if (!ND)
      return false;
    return ReplacableDecls.count(ND) != 0;
  }

private:
  llvm::DenseSet<const NamedDecl *> ReplacableDecls;
};

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_RESERVED_NAMES_H
