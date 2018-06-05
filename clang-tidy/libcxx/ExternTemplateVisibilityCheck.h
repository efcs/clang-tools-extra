//===--- ExternTemplateVisibilityCheck.h - clang-tidy----------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_EXTERN_TEMPLATE_VISIBILITY_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_EXTERN_TEMPLATE_VISIBILITY_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace libcxx {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-hidden-extern-template.html
class ExternTemplateVisibilityCheck : public ClangTidyCheck {
public:
  ExternTemplateVisibilityCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  void performFixIt(FunctionDecl *FD, SourceManager &SM, ASTContext &Context);
};

} // namespace libcxx
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LIBCXX_EXTERN_TEMPLATE_VISIBILITY_H
