//===--- ReservedNamesCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReservedNamesCheck.h"
#include "CommonUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace libcxx {


static std::string getNewName(const IdentifierInfo *Info) {
  assert(Info && Info->getLength() != 0);
  const auto Len = Info->getLength();
  const char* Name = Info->getNameStart();
  bool HasLeadingUnderscore = Name[0] == '_';
  auto isLower = [](char ch) { return ch >= 'a' && ch <= 'z'; };
  auto isUpper = [](char ch) { return ch >= 'A' && ch <= 'Z'; };
  if (HasLeadingUnderscore) {
    if (Len == 1)
      return "";
    assert(Name[1] != '_');
    if (isUpper(Name[1])) {
      return "";
    }
    else if (isLower(Name[1])) {
      return "";
    } else {
      return "";
    }
  } else {
    if (isLower(Name[0])) {
      std::string NewName = "__";
      NewName += Name;
      return NewName;
    }
  }
  return "";
}

static const NamedDecl *extractNamedDecl(const TypeLoc *Loc) {
  const NamedDecl *Decl = nullptr;
  if (const auto &Ref = Loc->getAs<TagTypeLoc>()) {
    Decl = Ref.getDecl();
  } else if (const auto &Ref = Loc->getAs<InjectedClassNameTypeLoc>()) {
    Decl = Ref.getDecl();
  } else if (const auto &Ref = Loc->getAs<UnresolvedUsingTypeLoc>()) {
    Decl = Ref.getDecl();
  } else if (const auto &Ref = Loc->getAs<TemplateTypeParmTypeLoc>()) {
    Decl = Ref.getDecl();
  }

  if (Decl)
    return Decl;

  if (const auto &Ref = Loc->getAs<TemplateSpecializationTypeLoc>()) {
    const auto *Decl = Ref.getTypePtr()->getTemplateName().getAsTemplateDecl();

    SourceRange Range(Ref.getTemplateNameLoc(), Ref.getTemplateNameLoc());
    if (const auto *ClassDecl = dyn_cast<TemplateDecl>(Decl)) {
      if (const auto *TemplDecl = ClassDecl->getTemplatedDecl())
        return cast<NamedDecl>(TemplDecl);
    }
  }

  if (const auto &Ref = Loc->getAs<DependentTemplateSpecializationTypeLoc>()) {
    if (const auto *Decl = Ref.getTypePtr()->getAsTagDecl())
      return cast<NamedDecl>(Decl);
  }
  return nullptr;
}

static bool declShouldUseReservedName(const Decl *D) {
  assert(D);
  if (D->getAccess() == AS_private)
    return true;
  if (isa<NonTypeTemplateParmDecl>(D) || isa<TemplateTypeParmDecl>(D) ||
      isa<ParmVarDecl>(D))
    return true;
  if (const auto *VD = dyn_cast<VarDecl>(D))
    return VD->isLocalVarDeclOrParm();
  return false;
}

static bool canReplaceDecl(const Decl *D) {
  assert(D);
  if (isa<ParmVarDecl>(D))
    return true;
  if (const auto *VD = dyn_cast<VarDecl>(D))
    return VD->getAccess() == AS_private || VD->isLocalVarDeclOrParm();
  return false;
}

AST_MATCHER(Decl, shouldUseReservedName) {
  return declShouldUseReservedName(&Node);
}

AST_MATCHER(DeclRefExpr, exprShouldUseReservedName) {
  auto *ND = Node.getDecl();
  assert(ND);
  return declShouldUseReservedName(ND);
}

AST_MATCHER(TypeLoc, typeIsReservedOrUnnamed) {
  auto *ND = extractNamedDecl(&Node);
  if (!ND)
    return true;
  return hasGoodReservedName(ND);
}

void ReservedNamesCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(namedDecl(isFromStdNamespace(), shouldUseReservedName(),
                               unless(isReservedOrUnnamed()))
                         .bind("decl"),
                     this);
  Finder->addMatcher(declRefExpr(exprShouldUseReservedName(),
                                 unless(declRefIsReservedOrUnnamed()))
                         .bind("declRef"),
                     this);
  Finder->addMatcher(typeLoc(unless(typeIsReservedOrUnnamed())).bind("typeLoc"),
                     this);
}

void ReservedNamesCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const NamedDecl *ND = Result.Nodes.getNodeAs<NamedDecl>("decl");
  const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("declRef");
  const TypeLoc *TL = Result.Nodes.getNodeAs<TypeLoc>("typeLoc");
  assert(!ND || !DRE);
  SourceLocation Loc;
  SourceRange Range;
  if (DRE) {
    assert(!ND);
    Loc = DRE->getLocation();
    Range = DRE->getNameInfo().getSourceRange();
    ND = DRE->getDecl();
  } else if (TL) {
    Range = TL->getSourceRange();
    Loc = Range.getBegin();
    ND = extractNamedDecl(TL);
    if (!ND)
      return;
    if (Loc.isInvalid()) {
      llvm::outs() << TL;
      return;
    }
    if (!hasReplacableDecl(ND))
      return;
  } else {
    assert(ND);
    Loc = ND->getLocation();
    Range = DeclarationNameInfo(ND->getDeclName(), ND->getLocation())
                .getSourceRange();
  }
  assert(ND);

  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;

  if (!isInLibcxxHeaderFile(SM, ND))
    return;
  const std::string NewName = getNewName(ND->getIdentifier());
  bool CanFix;
  if (DRE || TL) {
    CanFix = hasReplacableDecl(ND);
  } else {
    CanFix = !NewName.empty() && canReplaceDecl(ND);
    if (CanFix)
      addReplacableDecl(ND);
  }
  if (!CanFix) {
    diag(Loc, "use of non-reserved name %0") << Range << ND;
  } else {
    diag(Loc, "use of non-reserved name %0")
        << Range << ND << FixItHint::CreateReplacement(Range, NewName.c_str());
  }
}

} // namespace libcxx
} // namespace tidy
} // namespace clang
