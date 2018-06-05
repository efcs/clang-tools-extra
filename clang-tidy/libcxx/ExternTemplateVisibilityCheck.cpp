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

AST_MATCHER(FunctionDecl, isDefiningDecl) {
  const FunctionDecl *FD = &Node;
  const FunctionDecl *Body = nullptr;
  return (FD->hasBody(Body) && Body == FD);
  FD = FD->getTemplateInstantiationPattern();
  if (!FD)
    return false;
  return FD->isThisDeclarationADefinition();
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
    if (A->isInherited())
      continue;
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
  Finder->addMatcher(
      functionDecl(allOf(isFromStdNamespace(), isExplicitInstantiation(),
                         isDefiningDecl()))
          .bind("func"),
      this);
}

static SourceRange getWhitespaceCorrectRange(SourceManager &SM,
                                             SourceRange Range) {
  // FIXME(EricWF): Remove leading whitespace when removing a token.
  bool Invalid = false;
  const char *TextAfter =
      SM.getCharacterData(Range.getEnd().getLocWithOffset(1), &Invalid);
  if (Invalid)
    return Range;
  unsigned Offset = std::strspn(TextAfter, " \t\r\n");
  return SourceRange(Range.getBegin(), Range.getEnd().getLocWithOffset(Offset));
}

void useTrailingSpace(SourceManager &SM, SourceLocation Loc, std::string &Str) {
  bool Invalid;
  const char *TextAfter =
      SM.getCharacterData(Loc.getLocWithOffset(1), &Invalid);
  if (Invalid)
    return;

  if (*TextAfter != '\n')
    Str += " ";
}

void ExternTemplateVisibilityCheck::performFixIt(const FunctionDecl *FD,
                                                 SourceManager &SM,
                                                 ASTContext &Context) {
  FD = FD->getTemplateInstantiationPattern();

  const auto *MD = dyn_cast<CXXMethodDecl>(FD);
  bool IsInlineDef = MD && MD->hasInlineBody();
  bool IsDtor = isa<CXXDestructorDecl>(FD);

  // If this isn't the first declaration, remove any present visibility
  // attributes. We'll insert them on the first declaration below.
  if (FD->getPreviousDecl() != FD) {
    StringRef Name;
    SourceLocation MacroLoc, ArgLoc;
    if (hasLibcxxMacro(Context, FD, Name, MacroLoc, ArgLoc) &&
        !FD->isFirstDecl()) {
      assert(ArgLoc.isValid());
      SourceRange Range = getWhitespaceCorrectRange(
          SM, SM.getExpansionRange(ArgLoc).getAsRange());
      diag(ArgLoc, "visibility declaration occurs does not occur on first "
                   "declaration of %0")
          << FD << FixItHint::CreateRemoval(Range);
    }
  }

  // Get the first declaration for the function. This should contain the
  // inline specification as well as the required visibility attributes.
  const FunctionDecl *Parent = FD->getFirstDecl();
  assert(Parent);

  // If the function isn't specified inline, either implicitly or explicitly,
  // then insert 'inline' on the first declaration.
  if (!Parent->isInlineSpecified() && !IsInlineDef) {
    SourceLocation Loc = Parent->getInnerLocStart();
    std::string Text = "inline";
    useTrailingSpace(SM, Loc, Text);
    diag(Loc, "explicitly instantiated function %0 is missing inline")
        << Parent << Parent->getSourceRange()
        << FixItHint::CreateInsertion(Loc, Text.c_str(), true);
  }

  // Don't mess with visibility declarations on destructors. Libc++ expects
  // externally instantiated definitions cannot have their addresses taken,
  // but this isn't true for destructors, whose addresses are required when
  // destructing global variables.
  if (!isa<CXXDestructorDecl>(FD)) {
    StringRef Name;
    SourceLocation MacroLoc, ArgLoc;
    bool Res = hasLibcxxMacro(Context, Parent, Name, MacroLoc, ArgLoc);
    // If there is no visibility attribute on the first declaration, insert
    // the correct one.
    if (!Res) {
      diag(Parent->getInnerLocStart(),
           "function %0 is missing visibility declaration")
          << Parent << Parent->getSourceRange()
          << FixItHint::CreateInsertion(
                 Parent->getInnerLocStart(),
                 "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY ");
    }
    // If we have a visibility attribute, but it's not what we expect, replace
    // it with the expected attribute.
    else if (Res && Name != "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY") {
      assert(ArgLoc.isValid());
      CharSourceRange Range(ArgLoc, true);
      diag(ArgLoc, "function %0 has incorrect visibility declaration '%1'")
          << Name << Parent
          << FixItHint::CreateReplacement(
                 Range, "_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY");
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
