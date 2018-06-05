//===--- ExternTemplateVisibilityCheck.cpp -
//clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExternTemplateVisibilityCheck.h"
#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {

AST_MATCHER(FunctionDecl, dummyMatcher) { return true; }

AST_MATCHER(FunctionDecl, isRedeclaration) {
  const FunctionDecl *FD = &Node;
  const FunctionDecl *Def = nullptr;
  return (FD->hasBody(Def) && Def == FD && FD->getCanonicalDecl() != FD);
}

AST_POLYMORPHIC_MATCHER(
    isExplicitInstantiation,
    AST_POLYMORPHIC_SUPPORTED_TYPES(FunctionDecl, VarDecl, CXXRecordDecl,
                                    ClassTemplateSpecializationDecl)) {
  TemplateSpecializationKind TSK = Node.getTemplateSpecializationKind();
  return (TSK == TSK_ExplicitInstantiationDeclaration ||
          TSK == TSK_ExplicitInstantiationDefinition);
}

static bool hasLibcxxMacro(ASTContext &Context, const FunctionDecl *FD,
                           StringRef &Name, SourceLocation &MacroLoc,
                           SourceLocation &ArgLoc) {
  if (!FD)
    return false;

  SourceManager &SM = Context.getSourceManager();
  auto isMatchingMacro = [&](const Attr *A) {
    if (!getMacroAndArgLocations(SM, Context, A->getLocation(), ArgLoc,
                                 MacroLoc, Name))
      return false;

    if (Name == "_LIBCPP_INLINE_VISIBILITY" ||
        Name == "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY")
      return true;
    return false;
  };
  for (auto *A : FD->attrs()) {
    if (isMatchingMacro(A))
      return true;
  }
  return false;
}

AST_POLYMORPHIC_MATCHER(hasLibcxxVisibilityMacro,
                        AST_POLYMORPHIC_SUPPORTED_TYPES(FunctionDecl,

                                                        CXXMethodDecl)) {

  auto *Fn = dyn_cast<FunctionDecl>(&Node);
  // Fn = Fn->getTemplateInstantiationPattern();
  if (!Fn)
    return false;
  StringRef Name;
  SourceLocation MacroLoc, ArgLoc;
  return hasLibcxxMacro(Finder->getASTContext(), Fn, Name, MacroLoc, ArgLoc);
}

void ExternTemplateVisibilityCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(functionDecl(allOf(dummyMatcher(), isFromStdNamespace(),
                                        isExplicitInstantiation()))
                         .bind("func"),
                     this);
}

void ExternTemplateVisibilityCheck::performFixIt(const FunctionDecl *FD,
                                                 SourceManager &SM,
                                                 ASTContext &Context) {
  FD = FD->getTemplateInstantiationPattern();
  const auto *MD = dyn_cast<CXXMethodDecl>(FD);
  bool IsInlineDef = MD && MD->hasInlineBody();
  StringRef FoundName;
  {
    SourceLocation MacroLoc, ArgLoc;
    bool Res = hasLibcxxMacro(Context, FD, FoundName, MacroLoc, ArgLoc);
    if (Res) {
      assert(ArgLoc.isValid());
      CharSourceRange Range(ArgLoc, true);
      if (FoundName != "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY") {
        diag(ArgLoc, "function %0 is explicitly instantiated and hidden")
            << FD << FD->getSourceRange()
            << FixItHint::CreateReplacement(
                   Range, "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY");
      }
    } else {
      diag(FD->getInnerLocStart(), "function %0 is missing declaration")
          << FD << FD->getSourceRange()
          << FixItHint::CreateInsertion(
                 FD->getInnerLocStart(),
                 "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY ");
    }
    if (!FD->isInlineSpecified() && !IsInlineDef) {
      SourceLocation Loc = FD->getInnerLocStart();
      diag(Loc, "function %0 is missing inline")
          << FD << FD->getSourceRange()
          << FixItHint::CreateInsertion(Loc, "inline ", true);
    }
  }
  {

    const FunctionDecl *Parent = FD->getCanonicalDecl();
    assert(Parent);
    if (FD == Parent || IsInlineDef) {

      // assert(!isa<CXXMethodDecl>(Templ));
      return;
    }
    assert(Parent && Parent != FD);

    StringRef Name;
    SourceLocation MacroLoc, ArgLoc;
    bool Res = hasLibcxxMacro(Context, Parent, Name, MacroLoc, ArgLoc);
    if (!Res) {
      diag(Parent->getInnerLocStart(), "function %0 is missing declaration")
          << Parent << Parent->getSourceRange()
          << FixItHint::CreateInsertion(
                 Parent->getInnerLocStart(),
                 "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY ");

    } else if (Name.data() && Name != FoundName) {
      CharSourceRange Range(ArgLoc, true);
      diag(ArgLoc, "incorrect macro name %0")
          << Name << Parent->getSourceRange()
          << FixItHint::CreateReplacement(
                 Range, "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY");
    }
  }
}

void ExternTemplateVisibilityCheck::check(
    const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("func");

  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;
  performFixIt(MatchedDecl, SM, Context);
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
