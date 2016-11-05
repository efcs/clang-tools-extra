set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyLibcxxModule
  LibcxxTidyModule.cpp
  HiddenExternTemplateCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils
  )
