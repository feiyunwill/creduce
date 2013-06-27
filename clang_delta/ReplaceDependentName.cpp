//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceDependentName.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"This pass replaces a dependent name (referred by typename) with \n\
a resolved type by looking up this dependent name in its decl context. \n\
";

static RegisterTransformation<ReplaceDependentName>
         Trans("replace-dependent-name", DescriptionMsg);

class ReplaceDependentNameCollectionVisitor : public
  RecursiveASTVisitor<ReplaceDependentNameCollectionVisitor> {

public:
  explicit 
  ReplaceDependentNameCollectionVisitor(ReplaceDependentName *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitDependentNameTypeLoc(DependentNameTypeLoc TLoc);

  bool VisitElaboratedTypeLoc(ElaboratedTypeLoc TLoc);

private:
  ReplaceDependentName *ConsumerInstance;

};

bool ReplaceDependentNameCollectionVisitor::VisitDependentNameTypeLoc(
       DependentNameTypeLoc TLoc)
{
  ConsumerInstance->handleOneDependentNameTypeLoc(TLoc);
  return true;
}

bool ReplaceDependentNameCollectionVisitor::VisitElaboratedTypeLoc(
       ElaboratedTypeLoc TLoc)
{
  ConsumerInstance->handleOneElaboratedTypeLoc(TLoc);
  return true;
}

void ReplaceDependentName::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceDependentNameCollectionVisitor(this);
}

void ReplaceDependentName::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  rewriteDependentName();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

SourceLocation ReplaceDependentName::getElaboratedTypeLocBegin(
                 const ElaboratedTypeLoc &TLoc)
{
  SourceLocation Loc = TLoc.getElaboratedKeywordLoc();
  if (Loc.isValid())
    return Loc;
  NestedNameSpecifierLoc SpecLoc = TLoc.getQualifierLoc();
  NestedNameSpecifierLoc Prefix = SpecLoc.getPrefix();

  while (Prefix.getBeginLoc().isValid()) {
    SpecLoc = Prefix;
    Prefix = Prefix.getPrefix();
  }
  Loc = SpecLoc.getBeginLoc();
  TransAssert(Loc.isValid() && "Failed to get ElaboratedTypeLoc!");
  return Loc;
}

void ReplaceDependentName::handleOneElaboratedTypeLoc(
       const ElaboratedTypeLoc &TLoc)
{
  if (TLoc.getBeginLoc().isInvalid())
    return;
  const ElaboratedType *ET = TLoc.getTypePtr();
  if ((ET->getKeyword() != ETK_Typename) && (ET->getKeyword() != ETK_None))
    return;

  if (TLoc.getQualifierLoc().getBeginLoc().isInvalid())
    return;
  std::string Str = "";
  bool Typename = false;
  if (!getTypeString(ET->getNamedType(), Str, Typename))
    return;
  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheTyName = Str;
    NeedTypenameKeyword = Typename;
    TheLocBegin = getElaboratedTypeLocBegin(TLoc);
    TheNameLocEnd = TLoc.getEndLoc();
  }
}

void ReplaceDependentName::handleOneDependentNameTypeLoc(
       const DependentNameTypeLoc &TLoc)
{
  if (TLoc.getBeginLoc().isInvalid())
    return;

  const DependentNameType *DNT = 
    dyn_cast<DependentNameType>(TLoc.getTypePtr());
  TransAssert(DNT && "NULL DependentNameType!");
  if (DNT->getKeyword() != ETK_Typename)
    return;

  std::string Str = "";
  bool Typename = false;
  if (!getDependentNameTypeString(DNT, Str, Typename))
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheTyName = Str;
    NeedTypenameKeyword = Typename;
    TheLocBegin = TLoc.getElaboratedKeywordLoc();
    TheNameLocEnd = TLoc.getEndLoc();
  }
}

void ReplaceDependentName::rewriteDependentName()
{
  std::string NewStr = "";
  if (NeedTypenameKeyword)
    NewStr += "typename ";
  NewStr += TheTyName;
  TheRewriter.ReplaceText(SourceRange(TheLocBegin, TheNameLocEnd), NewStr);
}

ReplaceDependentName::~ReplaceDependentName()
{
  delete CollectionVisitor;
}
