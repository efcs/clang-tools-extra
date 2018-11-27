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

AST_POLYMORPHIC_MATCHER(
    isExplicitInstantiation,
    AST_POLYMORPHIC_SUPPORTED_TYPES(FunctionDecl, VarDecl, CXXRecordDecl,
                                    ClassTemplateSpecializationDecl)) {
  TemplateSpecializationKind TSK = Node.getTemplateSpecializationKind();
  return (TSK == TSK_ExplicitInstantiationDefinition);
}

static bool hasLibcxxMacro(ASTContext &Context, const FunctionDecl *FD,
                           MacroInfo &Info) {
  if (!FD)
    return false;

  SourceManager &SM = Context.getSourceManager();
  auto isMatchingMacro = [&](const Attr *A) {
    std::vector<MacroInfo> InfoList;
    if (!getMacroAndArgLocationList(SM, Context, A->getLocation(), InfoList))
      return false;

    assert(!InfoList.empty());
    Info = InfoList.back();
    if (Info.Name == "_LIBCPP_INLINE_VISIBILITY" ||
        Info.Name == "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY" ||
        Info.Name == "_LIBCPP_ALWAYS_INLINE")
      return true;
    return false;
  };
  for (auto *A : FD->attrs()) {
    if (A->isInherited())
      continue;
    if (isMatchingMacro(A))
      return true;
  }
  return false;
}

void ExternTemplateVisibilityCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(allOf(isFromStdNamespace(), isExplicitInstantiation(),
                         isDefinition(),
                         anyOf(
                                 hasAncestor(classTemplateSpecializationDecl(isExplicitInstantiation())),
                                 hasAncestor(functionTemplateDecl()))
                   ))
          .bind("func"),
      this);
}

void ExternTemplateVisibilityCheck::performFixIt(const FunctionDecl *FD,
                                                 SourceManager &SM,
                                                 ASTContext &Context) {
  FD = FD->getTemplateInstantiationPattern();
  assert(FD && "expected externally instantiated template");

  bool IsInlineDef =
      isa<CXXMethodDecl>(FD) && cast<CXXMethodDecl>(FD)->hasInlineBody();

  // If this isn't the first declaration, remove any present visibility
  // attributes. We'll insert them on the first declaration below.
  if (FD->getPreviousDecl() != FD) {
    MacroInfo Info;
    if (hasLibcxxMacro(Context, FD, Info) && !FD->isFirstDecl()) {
      assert(Info.ExpansionRange.isValid());
      CharSourceRange Range = Info.ExpansionRange;
      diag(FD->getLocation(),
           "visibility declaration does not occur on the first "
           "declaration of %0")
          << FD
          << FixItHint::CreateRemoval(getWhitespaceCorrectRange(SM, Range));
    }
  }

  // Get the first declaration for the function. This should contain the
  // inline specification as well as the required visibility attributes.
  const FunctionDecl *First = FD->getFirstDecl();
  assert(First);

  // If the function isn't specified inline, either implicitly or explicitly,
  // then insert 'inline' on the first declaration.
  if (!First->isInlineSpecified() && !IsInlineDef) {
    SourceLocation Loc = First->getInnerLocStart();
    diag(First->getLocation(), "explicitly instantiated function %0 is missing inline")
        << First << First->getSourceRange()
        << FixItHint::CreateInsertion(Loc, nameWithTrailingSpace(SM, Loc, "inline"),
                                      true);
  }

  // Don't mess with visibility declarations on destructors. Libc++ expects
  // externally instantiated definitions cannot have their addresses taken,
  // but this isn't true for destructors, whose addresses are required when
  // destructing global variables.
  if (!isa<CXXDestructorDecl>(FD)) {
    MacroInfo Info;
    bool Res = hasLibcxxMacro(Context, First, Info);
    // If there is no visibility attribute on the first declaration, insert
    // the correct one.
    if (!Res) {
      diag(First->getLocation(),
           "function %0 is missing visibility declaration")
          << First << First->getSourceRange()
          << FixItHint::CreateInsertion(
                 First->getInnerLocStart(),
                 "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY ");
    }
    // If we have a visibility attribute, but it's not what we expect, replace
    // it with the expected attribute.
    else if (Res && Info.Name != "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY") {
      assert(Info.ExpansionRange.isValid());

      diag(First->getLocation(),
           "function %0 has incorrect visibility declaration '%1'")
          << First << Info.Name
          << FixItHint::CreateReplacement(
                 Info.ExpansionRange,
                 "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY");
    }
  }
}

void ExternTemplateVisibilityCheck::check(
    const MatchFinder::MatchResult &Result) {
  performFixIt(Result.Nodes.getNodeAs<FunctionDecl>("func"),
               *Result.SourceManager, *Result.Context);
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
