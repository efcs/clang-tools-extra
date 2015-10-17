//===--- LLVMTidyModule.cpp - clang-tidy ----------------------------------===//
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
#include "ReservedNameCheck.h"
#include "UnqualifiedFunctionCallCheck.h"


namespace clang {
namespace tidy {
namespace libcxx {

class LibcxxTidyModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ReservedNameCheck>(
            "libcxx-reserved-name-check");
    CheckFactories.registerCheck<UnqualifiedFunctionCallCheck>(
            "libcxx-unqualified-function-call-check");
  }
};

// Register the LLVMTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<LibcxxTidyModule> X("libcxx-module",
                                                        "Adds libc++ lint checks.");

} // namespace libcxx

// This anchor is used to force the linker to link in the generated object file
// and thus register the LibcxxTidyModule
volatile int LibcxxModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
