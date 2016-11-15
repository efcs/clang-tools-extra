set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyLibcxxModule
  AttributeNotOnFirstDeclCheck.cpp
  LibcxxTidyModule.cpp
  HiddenExternTemplateCheck.cpp
  ReservedNamesCheck.cpp
  TemplateMissingInlineCheck.cpp
  CommonUtils.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils
  )
