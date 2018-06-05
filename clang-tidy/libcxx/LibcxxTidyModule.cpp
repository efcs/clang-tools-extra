//===--- GoogleTidyModule.cpp - clang-tidy --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "AttributeNotOnFirstDeclCheck.h"
#include "ExternTemplateVisibilityCheck.h"
#include "HiddenExternTemplateCheck.h"
#include "ReservedNamesCheck.h"
#include "TemplateMissingInlineCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {

class LibcxxModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<AttributeNotOnFirstDeclCheck>(
        "libcxx-attribute-not-on-first-decl");
    CheckFactories.registerCheck<HiddenExternTemplateCheck>(
        "libcxx-hidden-extern-template");

    CheckFactories.registerCheck<ReservedNamesCheck>(
        "libcxx-reserved-names");
    CheckFactories.registerCheck<TemplateMissingInlineCheck>(
        "libcxx-template-missing-inline");
    CheckFactories.registerCheck<ExternTemplateVisibilityCheck>(
        "libcxx-extern-template");
  }

  ClangTidyOptions getModuleOptions() override {
    ClangTidyOptions Options;
    auto &Opts = Options.CheckOptions;
    return Options;
  }
};

// Register the GoogleTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<LibcxxModule> X("libcxx-module",
                                                    "Adds Libcxx lint checks.");

} // namespace libcxx

// This anchor is used to force the linker to link in the generated object file
// and thus register the GoogleModule.
volatile int LibcxxModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
