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
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
using namespace clang::ast_matchers;

#if 1
namespace llvm {
/// Specialisation of DenseMapInfo to allow NamingCheckId objects in DenseMaps
template <>
struct DenseMapInfo<clang::tidy::libcxx::ReservedNamesCheck::NamingCheckId> {
  using NamingCheckId = clang::tidy::libcxx::ReservedNamesCheck::NamingCheckId;

  static inline NamingCheckId getEmptyKey() {
    return NamingCheckId(
        clang::SourceLocation::getFromRawEncoding(static_cast<unsigned>(-1)),
        "EMPTY");
  }

  static inline NamingCheckId getTombstoneKey() {
    return NamingCheckId(
        clang::SourceLocation::getFromRawEncoding(static_cast<unsigned>(-2)),
        "TOMBSTONE");
  }

  static unsigned getHashValue(NamingCheckId Val) {
    assert(Val != getEmptyKey() && "Cannot hash the empty key!");
    assert(Val != getTombstoneKey() && "Cannot hash the tombstone key!");

    std::hash<NamingCheckId::second_type> SecondHash;
    return Val.first.getRawEncoding() + SecondHash(Val.second);
  }

  static bool isEqual(NamingCheckId LHS, NamingCheckId RHS) {
    if (RHS == getEmptyKey())
      return LHS == getEmptyKey();
    if (RHS == getTombstoneKey())
      return LHS == getTombstoneKey();
    return LHS == RHS;
  }
};
template <> struct DenseMapInfo<std::string> {

  static inline std::string getEmptyKey() { return "__EMPTY__"; }

  static inline std::string getTombstoneKey() { return "__TOMBSTONE__"; }

  static unsigned getHashValue(std::string const &Val) {
    assert(Val != getEmptyKey() && "Cannot hash the empty key!");
    assert(Val != getTombstoneKey() && "Cannot hash the tombstone key!");

    std::hash<std::string> Hash;
    return Hash(Val);
  }

  static bool isEqual(std::string const &LHS, std::string const &RHS) {
    if (RHS == getEmptyKey())
      return LHS == getEmptyKey();
    if (RHS == getTombstoneKey())
      return LHS == getTombstoneKey();
    return LHS == RHS;
  }
};
using clang::SourceLocation;
template <> struct DenseMapInfo<clang::SourceLocation> {
  static inline SourceLocation getEmptyKey() {
    return clang::SourceLocation::getFromRawEncoding(static_cast<unsigned>(-1));
  }

  static inline SourceLocation getTombstoneKey() {
    return clang::SourceLocation::getFromRawEncoding(static_cast<unsigned>(-2));
  }

  static unsigned getHashValue(SourceLocation Val) {
    assert(Val != getEmptyKey() && "Cannot hash the empty key!");
    assert(Val != getTombstoneKey() && "Cannot hash the tombstone key!");

    return Val.getRawEncoding();
  }

  static bool isEqual(SourceLocation LHS, SourceLocation RHS) {
    if (RHS == getEmptyKey())
      return LHS == getEmptyKey();
    if (RHS == getTombstoneKey())
      return LHS == getTombstoneKey();
    return LHS == RHS;
  }
};
} // namespace llvm
#endif
namespace clang {
namespace tidy {
namespace libcxx {

static StringRef getIdentString(const NamedDecl *ND) {
  if (!ND || !ND->getIdentifier())
    return "";
  return ND->getIdentifier()->getNameStart();
}

static std::string getNewName(StringRef Name, bool IsField = false) {
  const auto Len = Name.size();
  if (Len == 0)
    return "";

  bool HasLeadingUnderscore = Name[0] == '_';
  bool HasTrailingUnderscore = Name.back() == '_';
  const char *Suffix = (IsField && !HasTrailingUnderscore) ? "_" : "";
  auto isLower = [](char ch) { return ch >= 'a' && ch <= 'z'; };
  auto isUpper = [](char ch) { return ch >= 'A' && ch <= 'Z'; };
  std::string NewName;
  if (HasLeadingUnderscore) {
    if (Len == 1)
      return "";
    if (Name[1] == '_')
      return "";
    if (isUpper(Name[1])) {
      if (Len > 2)
         return ""; // TODO
      NewName += Name;
      NewName += "p";
      assert(Suffix == "");
    }
    else if (isLower(Name[1])) {
      NewName += "_";
      NewName += Name;
      NewName += Suffix;
    } else {
      return "";
    }
  } else {
    if (isLower(Name[0])) {
      NewName = "__";
      NewName += Name;
      NewName += Suffix;
    } else if (isUpper(Name[0]) && Len >= 2) {
      NewName = "_";
      NewName += Name;;
      // FIXME allows better diagnostics here
      assert(Suffix == StringRef(""));
    } else if (isUpper(Name[0]) && Len == 1) {
      NewName = "_";
      NewName += Name;
      NewName += "p";
      assert(Suffix == StringRef(""));
    } else if (Name == "T") {
      assert(Suffix == StringRef(""));
      return "_Tp";
    }
  }
  return NewName;
}

static std::string getNewName(const NamedDecl *ND) {
  bool AddSuffix = isa<FieldDecl>(ND);
  return getNewName(getIdentString(ND), AddSuffix);
}

static std::pair<const NamedDecl *, SourceLocation>
extractNamedDecl(const TypeLoc *Loc) {
  auto UQ = Loc->getUnqualifiedLoc();
  Loc = &UQ;
  const NamedDecl *Decl = nullptr;
  SourceLocation NameLoc = Loc->getBeginLoc();
  if (const auto &Ref = Loc->getAs<TagTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<InjectedClassNameTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<UnresolvedUsingTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<TemplateTypeParmTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  }

  if (Decl)
    return {Decl, NameLoc};

  if (const auto &Ref = Loc->getAs<TemplateSpecializationTypeLoc>()) {
    const auto *Decl = Ref.getTypePtr()->getTemplateName().getAsTemplateDecl();
    assert(Decl);
    SourceRange Range(Ref.getTemplateNameLoc(), Ref.getTemplateNameLoc());
    if (const auto *ClassDecl = dyn_cast<TemplateDecl>(Decl)) {
      if (const auto *TemplDecl = ClassDecl->getTemplatedDecl())
        return {cast<NamedDecl>(TemplDecl), Range.getBegin()};
    }
  }

  if (const auto &Ref = Loc->getAs<DependentTemplateSpecializationTypeLoc>()) {
    if (const auto *Decl = Ref.getTypePtr()->getAsTagDecl())
      return {cast<NamedDecl>(Decl), Ref.getBeginLoc()};
  }
  return {nullptr, NameLoc};
}

static std::pair<std::string, SourceLocation>
extractDependentName(const TypeLoc *Loc) {
  auto UQ = Loc->getUnqualifiedLoc();
  Loc = &UQ;
  const NamedDecl *Decl = nullptr;
  SourceLocation NameLoc = Loc->getBeginLoc();
  if (const auto &Ref = Loc->getAs<TagTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<InjectedClassNameTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<UnresolvedUsingTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  } else if (const auto &Ref = Loc->getAs<TemplateTypeParmTypeLoc>()) {
    Decl = Ref.getDecl();
    NameLoc = Ref.getNameLoc();
  }
  if (Decl)
    goto finish_extract;

#if 1
  if (const auto &Ref = Loc->getAs<TemplateSpecializationTypeLoc>()) {
    const auto *ODecl = Ref.getTypePtr()->getTemplateName().getAsTemplateDecl();
    Decl = cast<NamedDecl>(ODecl);
    NameLoc = Decl->getBeginLoc();
    goto finish_extract;
  }

  if (const auto &Ref = Loc->getAs<DependentTemplateSpecializationTypeLoc>()) {
    auto *ID = Ref.getTypePtr()->getIdentifier();
    if (!ID) {
      return {};
    }
    return {ID->getNameStart(), Ref.getBeginLoc()};
  }
#if 0
  if (const auto &Ref = Loc->getAs<ElaboratedTypeLoc>()) {
    auto *NNS = Ref.getQualifierLoc().getNestedNameSpecifier();
    if (NNS) {
      llvm::outs() << "\nDumping ElaboratedTypeLoc: ";
      NNS->dump();
      llvm::outs() << "\n";
    }
    auto NewLoc = Ref.getNamedTypeLoc();
    return extractDependentName(&NewLoc);
  }
#endif
#endif
finish_extract:
  if (Decl) {
    if (Decl->getIdentifier())
      return {Decl->getIdentifier()->getNameStart(), NameLoc};
    return {"", NameLoc};
  }
  return {"", NameLoc};
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
  if (D->getAccess() == AS_private && (isa<VarDecl>(D) || isa<FieldDecl>(D) || isa<CXXMethodDecl>(D)))
    return true;
  if (const auto *VD = dyn_cast<VarDecl>(D))
    return VD->isLocalVarDeclOrParm();
  if (isa<NonTypeTemplateParmDecl>(D) || isa<TemplateTypeParmDecl>(D))
    return true;
  return false;
}

AST_MATCHER(Decl, shouldUseReservedName) {
  return declShouldUseReservedName(&Node);
}

AST_MATCHER(Expr, exprShouldUseReservedName) {
  const NamedDecl *ND;
  if (const auto *DRE = dyn_cast<DeclRefExpr>(&Node))
    ND = DRE->getDecl();
  else if (const auto *ME = dyn_cast<MemberExpr>(&Node)) {
    const auto *VD = ME->getMemberDecl();
    ND = cast<NamedDecl>(VD);
  } else if (const auto *TS = dyn_cast<SubstNonTypeTemplateParmExpr>(&Node))
    ND = cast<NamedDecl>(TS->getParameter());
  else if (const auto *TS = dyn_cast<SubstNonTypeTemplateParmPackExpr>(&Node))
    ND = cast<NamedDecl>(TS->getParameterPack());
  else
    return false;
  assert(ND);
  return IsFromStdNamespace(ND) && declShouldUseReservedName(ND) &&
         !IsAllowableReservedName(ND);
}

AST_MATCHER(CXXCtorInitializer, isRealInit) {
  return Node.isWritten() && Node.isMemberInitializer() &&
          !Node.isInClassMemberInitializer();
}
AST_MATCHER(CXXConstructorDecl, isDefaultedCtor) {
  if (Node.isImplicit() || Node.isDefaulted()) {
    return true;
  }
  if (Node.isTemplateInstantiation()) {
    auto *Ctor = dyn_cast_or_null<CXXConstructorDecl>(Node.getTemplateInstantiationPattern());
    assert(Ctor);
    return Ctor->isImplicit() || Ctor->isDefaulted();
  }
  return false;
}
AST_MATCHER(TypeLoc, typeIsReservedOrUnnamed) {
  UnqualTypeLoc UL = Node.getUnqualifiedLoc();
  auto Pair = extractNamedDecl(&UL);
  if (!Pair.first)
    return true;
  return IsAllowableReservedName(Pair.first);
}

AST_MATCHER(TypeLoc, typeLocIsValid) {

  if (Node.isNull())
    return false;
  auto *ND = extractNamedDecl(&Node).first;
  if (!ND)
    return false;
  if (IsAllowableReservedName(ND))
    return false;
  if (!declShouldUseReservedName(ND))
    return false;
  if (const auto &Ref = Node.getAs<TagTypeLoc>())
    return false;
  if (const auto &Ref = Node.getAs<InjectedClassNameTypeLoc>())
    return false;
  if (isa<CXXConstructorDecl>(ND)) {
    assert(false);
  }
  if (!IsFromStdNamespace(ND->getCanonicalDecl()))
    return false;
  if (isa<CXXRecordDecl>(ND))
    return false;
  if (isa<UsingDecl>(ND))
    assert(false);
  return true;
}

AST_MATCHER(Expr, isTemplateArgumentSubstitution) {
  return isa<SubstNonTypeTemplateParmExpr>(&Node) ||
         isa<SubstNonTypeTemplateParmPackExpr>(&Node);
}

AST_MATCHER(Expr, isInterestingCXXDependentScopeMemberExpr) {
  if (auto *ME = dyn_cast<CXXDependentScopeMemberExpr>(&Node)) {
    auto Name = ME->getMemberNameInfo().getAsString();
    if (IsAllowableReservedName(Name))
      return false;
    if (ME->isImplicitAccess())
      return false;
    auto *E = dyn_cast<DeclRefExpr>(ME->getBase());
    if (!E)
      return false;
    return true;
  }
  return false;
}

static void addUsage(ReservedNamesCheck::NamingCheckFailureMap &Failures,
                     llvm::DenseSet<SourceLocation> &TransformedLocs,
                     const NamedDecl *ND, SourceRange Range,
                     SourceManager &SourceMgr, bool ShouldFix = false,
                     bool ShouldExist = false) {
  assert(ND && ND->getLocation().isValid() && Range.isValid());
  ND = cast<NamedDecl>(ND->getCanonicalDecl());
  assert(ND == ND->getCanonicalDecl());
  SourceLocation FixLocation = SourceMgr.getSpellingLoc(Range.getBegin());
  assert(!FixLocation.isInvalid());
  ReservedNamesCheck::NamingCheckId ID = {ND->getLocation(),
                                          getIdentString(ND)};
  auto It = Failures.find(ID);
  if (It == Failures.end()) {
    assert(!ShouldExist);
    auto NewName = getNewName(ND);
    ShouldFix = ShouldFix && NewName != "";
    ReservedNamesCheck::NamingCheckFailure Fail(NewName, ShouldFix);
    auto ItPair = Failures.try_emplace(ID, Fail);
    It = ItPair.first;
    assert(ItPair.second);
  } else {
    // assert(ShouldExist);
  }
  It->second.addNewRange(SourceRange(FixLocation));
  TransformedLocs.insert(FixLocation);
}

bool ReservedNamesCheck::hasFailedDecl(const NamedDecl *ND) const {
  ND = cast_or_null<NamedDecl>(ND->getCanonicalDecl());
  assert(ND && ND->getCanonicalDecl() == ND);
  if (ND->getIdentifier() == nullptr)
    return false;
  NamingCheckId ID{ND->getLocation(), ND->getIdentifier()->getNameStart()};
  return Failures.count(ID) != 0;
}

void ReservedNamesCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(namedDecl(isFromStdNamespace(), shouldUseReservedName(),
                               unless(isAllowableReservedName()))
                         .bind("decl"),
                     this);
  Finder->addMatcher(declRefExpr(exprShouldUseReservedName(),
                                 unless(hasAncestor(cxxConstructorDecl(isDefaultedCtor()))),
                                 unless(declRefIsAllowableReservedName()))
                         .bind("declRef"),
                     this);
  Finder->addMatcher(memberExpr(
                    exprShouldUseReservedName(),
                     unless(hasParent(cxxConstructorDecl(isDefaultedCtor())))
                     ).bind("memberRef"),
                     this);
  Finder->addMatcher(
      typeLoc(typeLocIsValid(), unless(typeIsReservedOrUnnamed()))
          .bind("typeLoc"),
      this);
  Finder->addMatcher(
      expr(isTemplateArgumentSubstitution(), exprShouldUseReservedName())
          .bind("subTempParm"),
      this);
  Finder->addMatcher(
      cxxCtorInitializer(
          isRealInit(),
          forField(allOf(isFromStdNamespace(), shouldUseReservedName(),
                         unless(isAllowableReservedName()))))
          .bind("memInit"),
      this);
  Finder->addMatcher(
      expr(isInterestingCXXDependentScopeMemberExpr()).bind("depMemberRef"),
      this);
}

void ReservedNamesCheck::checkDependentExpr(const Expr *E) {
  if (auto *DME = dyn_cast<CXXDependentScopeMemberExpr>(E)) {
    auto Name = DME->getMemberNameInfo().getAsString();
    SourceRange FixLocation = DME->getMemberNameInfo().getSourceRange();
    if (TransformedLocs.count(FixLocation.getBegin()) != 0)
      return;
    auto FIt = TransformedMembers.find(Name);
    if (FIt == TransformedMembers.end())
      return;
    auto *Base = dyn_cast<DeclRefExpr>(DME->getBase());
    const DeclaratorDecl *D = nullptr;
    if (Base) {
      D = dyn_cast_or_null<DeclaratorDecl>(Base->getDecl());
      assert(!Base->getDecl() || D);
    }
    if (!Base || !D) {
      diag(DME->getBeginLoc(), "possibly renamed member '%0'") << Name;
      return;
    }

    assert(!FixLocation.isInvalid());

    auto UnqualTL = D->getTypeSourceInfo()->getTypeLoc();
    const TypeLoc *TL = &UnqualTL;
    auto Pair = extractDependentName(TL);
    if (Pair.first.empty()) {
      const std::pair<std::string, NamingCheckId> *FoundRep = nullptr;

      auto &Vec = FIt->second;
      assert(!Vec.empty());
      FoundRep = &Vec.front();
      const std::string &FromRDName = FoundRep->first;
      for (const auto &KV : Vec) {
        if (KV.second != FoundRep->second) {
          diag(DME->getBeginLoc(), "ambigious something... '%0'") << Name;
          return;
        }
      }
      auto ExistingFailure = Failures.find(FoundRep->second);
      assert(ExistingFailure != Failures.end());
      // FIXME Only attempt to fix names with a trailing underscore.
      if (ExistingFailure->second.ShouldFix && Name.back() == '_') {
        ExistingFailure->second.addNewRange(FixLocation);
        TransformedLocs.insert(FixLocation.getBegin());
        diag(FixLocation.getBegin(),
             "attempting fix on dependent member name '%0' which was replaced in the class template '%1'",
             DiagnosticIDs::Remark)
            << Name << FromRDName;
        return;
      }
    }
    auto RDName = Pair.first;
    assert(RDName != "tuple");
    diag(FixLocation.getBegin(), "possibly dependent renamed member '%0'")
        << Name;
    // if (FoundRep) {
    //  diag(FoundRep->second.first, "member declared here");
    // }
  } else {
    assert(false && "unhandled");
  }
}

static const Decl *getSourceDecl(const Decl *D) {
  if (const auto *Func = dyn_cast<FunctionDecl>(D)) {
    // If this function was instantiated from a member function of a
    // class template, check whether that member function was defined out-of-line.
    if (FunctionDecl *FD = Func->getInstantiatedFromMemberFunction()) {
      FD = FD->getCanonicalDecl();
      if (!FD->isOutOfLine())
        return FD;
    }

    // If this function was instantiated from a function template,
    // check whether that function template was defined out-of-line.
    if (FunctionTemplateDecl *FunTmpl = Func->getPrimaryTemplate()) {
      return FunTmpl->getTemplatedDecl()->getCanonicalDecl();
    }
  }
  return D->getCanonicalDecl();
}

void ReservedNamesCheck::check(const MatchFinder::MatchResult &Result) {
  SourceMgr = Result.SourceManager;
  // FIXME: Add callback implementation.
  const NamedDecl *ND = Result.Nodes.getNodeAs<NamedDecl>("decl");
  const Decl *SourceDecl = nullptr;
  const bool MatchesDecl = ND != nullptr;
  const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("declRef");
  const TypeLoc *QualTL = Result.Nodes.getNodeAs<TypeLoc>("typeLoc");
  const CXXCtorInitializer *CIT =
      Result.Nodes.getNodeAs<CXXCtorInitializer>("memInit");
  UnqualTypeLoc TLStorage;
  const UnqualTypeLoc *TL = nullptr;
  if (QualTL) {
    TLStorage = QualTL->getUnqualifiedLoc();
    TL = &TLStorage;
  }
  const MemberExpr *ME = Result.Nodes.getNodeAs<MemberExpr>("memberRef");
  const Expr *TPS = Result.Nodes.getNodeAs<Expr>("subTempParm");
  const Expr *DepExpr = Result.Nodes.getNodeAs<Expr>("depMemberRef");
  const RecordDecl *OwningRecord = nullptr;
  if (DepExpr) {
    DependentExprs.push_back(cast<Expr>(DepExpr));
    return;
  }
  SourceLocation Loc;
  SourceRange Range;
  if (DRE) {
    assert(!ND);
    Loc = DRE->getLocation();
    Range = DRE->getNameInfo().getSourceRange();
    ND = DRE->getDecl();
  } else if (TL) {
    auto Pair = extractNamedDecl(TL);
    ND = Pair.first;
    if (!ND)
      return;
    Range = SourceRange(Pair.second);
    Loc = Pair.second;
    if (Loc.isInvalid()) {
      assert(TL->getBeginLoc().isInvalid());
      assert(TL->getBeginLoc().isInvalid());
      assert(TL->getLocalSourceRange().isInvalid());
      assert(TL->getUnqualifiedLoc().getSourceRange().isInvalid());
    }
    if (Loc.isInvalid()) {
      if (ND->getIdentifier())
        llvm::outs() << "INVALID TYPE LOC: '"
                     << ND->getIdentifier()->getNameStart() << "'\n";
      return;
    }

  } else if (ME) {
    ND = ME->getMemberDecl();
    Loc = ME->getMemberLoc();
    assert(Loc.isValid());
    Range = ME->getMemberNameInfo().getSourceRange();
    assert(Range.isValid());
  } else if (TPS) {
    if (const auto *TS = dyn_cast<SubstNonTypeTemplateParmExpr>(TPS)) {
      ND = cast<NamedDecl>(TS->getParameter());
      Loc = TS->getNameLoc();
      Range = SourceRange(Loc);
    } else if (const auto *TS =
                   dyn_cast<SubstNonTypeTemplateParmPackExpr>(TPS)) {
      ND = cast<NamedDecl>(TS->getParameterPack());
      Loc = TS->getBeginLoc();
      Range = TS->getSourceRange();

    } else {
      assert(false && "unsupported type");
    }
    assert(Loc.isValid());
    assert(Range.isValid());

  } else if (CIT) {
    assert(CIT->isWritten() && CIT->isMemberInitializer()
            && !CIT->isInClassMemberInitializer());
    ND = CIT->getMember();
    Loc = CIT->getMemberLocation();
    Range = SourceRange(Loc);

  } else {
    assert(ND);
    assert(MatchesDecl);
    Loc = ND->getLocation();
    Range = DeclarationNameInfo(ND->getDeclName(), ND->getLocation())
                .getSourceRange();

  }
  assert(ND);
  if (!SourceDecl) {
    SourceDecl = getSourceDecl(ND);
  }
  assert(SourceDecl);
  auto IDName = getIdentString(ND);
  if (IDName == "init" || IDName == "use_facet" || IDName == "has_facet"
          || IDName == "allocator_type" || IDName == "size_type")
    return; // FIXME
  auto &SM = *Result.SourceManager;
  auto &Context = *Result.Context;

  if (!isInLibcxxHeaderFile(SM, SourceDecl))
    return;
  if (IsAllowableReservedName(ND)) {
    llvm::outs() << "name is reserved: '" << getIdentString(ND) << "'\n";
    return;
  }
  assert(IsFromStdNamespace(ND));
  if (MatchesDecl) {
    addUsage(Failures, TransformedLocs, ND, Range, SM, canReplaceDecl(ND),
             false);
  } else {
    if (!hasFailedDecl(ND)) {
      assert(ND->getSourceRange().isValid());
      addUsage(Failures, TransformedLocs, ND, Range, SM, canReplaceDecl(ND),
               false);
    } else {

      addUsage(Failures, TransformedLocs, ND, Range, SM, false, true);
    }
  }
  if (auto *FD = dyn_cast<FieldDecl>(ND)) {
    auto *RD = FD->getParent();
    auto FIt = Failures.find({ND->getLocation(), getIdentString(ND)});
    assert(FIt != Failures.end());

    // llvm::outs() << getIdentString(ND) << " " << getIdentString(RD) << "\n";
    auto ItPair =
        TransformedMembers.try_emplace(getIdentString(ND), ReplacedNamesSet{});
    auto It = ItPair.first;
    It->second.push_back({getIdentString(RD), FIt->first});
  }
}

void ReservedNamesCheck::onEndOfTranslationUnit() {
  for (auto const &DepExprs : DependentExprs)
    checkDependentExpr(DepExprs);
  for (auto const &KV : Failures) {
    auto Loc = KV.first.first; // KV.first->getLocation();
    auto Diag = diag(Loc, "use of non-reserved name '%0'") << KV.first.second;
    if (!KV.second.ShouldFix)
      continue;
    for (auto const &FixupRange : KV.second.ReplacementRanges) {
      assert(FixupRange.isValid());
      Diag << FixItHint::CreateReplacement(FixupRange, KV.second.Fixup);
    }
  }
}

ReservedNamesCheck::ReservedNamesCheck(StringRef Name,
                                       ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

} // namespace libcxx
} // namespace tidy
} // namespace clang
