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
#include <vector>

namespace clang {
namespace tidy {
namespace libcxx {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/libcxx-reserved-names.html
class ReservedNamesCheck : public ClangTidyCheck {
public:
  ReservedNamesCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void onEndOfTranslationUnit() override;

  void checkDependentExpr(const Expr *E);
  bool hasFailedDecl(const NamedDecl *ND) const;

  struct NamingCheckFailure {
    std::string Fixup;
    bool ShouldFix;
    std::vector<SourceRange> ReplacementRanges;
    NamingCheckFailure(std::string F, bool SF = false)
        : Fixup(F), ShouldFix(SF) {}

    void addNewRange(SourceRange R) {
      for (auto const &ExistingRange : ReplacementRanges) {
        if (R == ExistingRange)
          return;
      }
      ReplacementRanges.push_back(R);
    }
  };

  typedef std::pair<SourceLocation, std::string> NamingCheckId;
  // typedef const NamedDecl * NamingCheckId;
  typedef llvm::DenseMap<NamingCheckId, NamingCheckFailure>
      NamingCheckFailureMap;
  typedef std::vector<std::pair<std::string, NamingCheckId>> ReplacedNamesSet;
  typedef llvm::DenseMap<std::string, ReplacedNamesSet> ReplacedMembersMap;

private:
  NamingCheckFailureMap Failures;
  std::vector<const Expr *> DependentExprs;
  ReplacedMembersMap TransformedMembers;
};

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_RESERVED_NAMES_H
