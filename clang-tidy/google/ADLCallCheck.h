//===--- ADLCallCheck.h - clang-tidy ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_ADLCALLCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_ADLCALLCHECK_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace google {

class ADLCallCheck : public ClangTidyCheck {
public:
  ADLCallCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::string getReplacementName(const DeclRefExpr *CalleeRef,
                                 const FunctionDecl *FD,
                                 const DeclContext *CallContext) const;

private:
  bool MinimizeReplacementName;
  bool OmitLeadingQualifier;
};

} // namespace google
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_ADLCALLCHECK_H
