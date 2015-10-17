set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyLibcxxModule
  LibcxxTidyModule.cpp
  ReservedNameCheck.cpp
  UnqualifiedFunctionCallCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils
  clangTooling
  )
