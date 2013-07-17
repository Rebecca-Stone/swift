//===--- ParseGeneric.cpp - Swift Language Parser for Generics ------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// Generic Parsing and AST Building
//
//===----------------------------------------------------------------------===//

#include "swift/Parse/Parser.h"
#include "swift/AST/Diagnostics.h"
#include "swift/Parse/Lexer.h"
using namespace swift;

/// parseGenericParameters - Parse a sequence of generic parameters, e.g.,
/// < T : Comparable, U : Container> along with an optional requires clause.
///
///   generic-params:
///     '<' generic-param (',' generic-param)? where-clause? '>'
///
///   generic-param:
///     identifier
///     identifier ':' type-identifier
///     identifier ':' type-composition
///
/// When parsing the generic parameters, this routine establishes a new scope
/// and adds those parameters to the scope.
GenericParamList *Parser::parseGenericParameters() {
  // Parse the opening '<'.
  assert(startsWithLess(Tok) && "Generic parameter list must start with '<'");
  return parseGenericParameters(consumeStartingLess());
}

GenericParamList *Parser::parseGenericParameters(SourceLoc LAngleLoc) {
  // Parse the generic parameter list.
  // FIXME: Allow a bare 'where' clause with no generic parameters?
  SmallVector<GenericParam, 4> GenericParams;
  bool Invalid = false;
  do {
    // Parse the name of the parameter.
    Identifier Name;
    SourceLoc NameLoc;
    if (parseIdentifier(Name, NameLoc, diag::expected_generics_parameter_name)) {
      Invalid = true;
      break;
    }

    // Parse the ':' followed by a type.
    SmallVector<TypeLoc, 1> Inherited;
    if (Tok.is(tok::colon)) {
      (void)consumeToken();
      TypeRepr *Ty = nullptr;
      if (Tok.getKind() == tok::identifier) {
        Ty = parseTypeIdentifier();
      } else if (Tok.getKind() == tok::kw_protocol) {
        Ty = parseTypeComposition();
      } else {
        diagnose(Tok.getLoc(), diag::expected_generics_type_restriction, Name);
        Invalid = true;
      }

      if (Ty)
        Inherited.push_back(Ty);
    }

    // FIXME: Bad location info here
    TypeAliasDecl *Param
      = new (Context) TypeAliasDecl(NameLoc, Name, NameLoc, TypeLoc(),
                                    CurDeclContext,
                                    Context.AllocateCopy(Inherited));
    Param->setGenericParameter();
    GenericParams.push_back(Param);

    // Add this parameter to the scope.
    ScopeInfo.addToScope(Param);

    // Parse the comma, if the list continues.
  } while (consumeIf(tok::comma));

  // Parse the optional where-clause.
  SourceLoc WhereLoc;
  SmallVector<Requirement, 4> Requirements;
  if (Tok.is(tok::kw_where) &&
      parseGenericWhereClause(WhereLoc, Requirements)) {
    Invalid = true;
  }
  
  // Parse the closing '>'.
  SourceLoc RAngleLoc;
  if (!startsWithGreater(Tok)) {
    if (!Invalid) {
      diagnose(Tok.getLoc(), diag::expected_rangle_generics_param);
      diagnose(LAngleLoc, diag::opening_angle);
      
      Invalid = true;
    }
    
    // Skip until we hit the '>'.
    skipUntilAnyOperator();
    if (startsWithGreater(Tok))
      RAngleLoc = consumeStartingGreater();
    else
      RAngleLoc = Tok.getLoc();
  } else {
    RAngleLoc = consumeStartingGreater();
  }

  if (GenericParams.empty())
    return nullptr;

  return GenericParamList::create(Context, LAngleLoc, GenericParams,
                                  WhereLoc, Requirements, RAngleLoc);
}

GenericParamList *Parser::maybeParseGenericParams() {
  if (!startsWithLess(Tok))
    return nullptr;

  return parseGenericParameters();
}

/// parseGenericWhereClause - Parse a 'where' clause, which places additional
/// constraints on generic parameters or types based on them.
///
///   where-clause:
///     'where' requirement (',' requirement) *
///
///   requirement:
///     conformance-requirement
///     same-type-requirement
///
///   conformance-requirement:
///     type-identifier ':' type-identifier
///     type-identifier ':' type-composition
///
///   same-type-requirement:
///     type-identifier '==' type-identifier
bool Parser::parseGenericWhereClause(SourceLoc &WhereLoc,
                                     SmallVectorImpl<Requirement> &Requirements) {
  // Parse the 'requires'.
  WhereLoc = consumeToken(tok::kw_where);
  bool Invalid = false;
  do {
    // Parse the leading type-identifier.
    // FIXME: Dropping TypeLocs left and right.
    TypeRepr *FirstType = parseTypeIdentifier();
    if (!FirstType) {
      Invalid = true;
      break;
    }

    if (Tok.is(tok::colon)) {
      // A conformance-requirement.
      SourceLoc ColonLoc = consumeToken();

      // Parse the protocol or composition.
      TypeRepr *Protocol = nullptr;
      if (Tok.is(tok::kw_protocol)) {
        Protocol = parseTypeComposition();
      } else {
        Protocol = parseTypeIdentifier();   
      }
      if (!Protocol) {
        Invalid = true;
        break;
      }

      // Add the requirement.
      Requirements.push_back(Requirement::getConformance(FirstType,
                                                         ColonLoc,
                                                         Protocol));
    } else if ((Tok.isAnyOperator() && Tok.getText() == "==") ||
               Tok.is(tok::equal)) {
      // A same-type-requirement
      if (Tok.is(tok::equal)) {
        diagnose(Tok, diag::requires_single_equal)
          .fixItReplace(SourceRange(Tok.getLoc()), "==");
      }
      SourceLoc EqualLoc = consumeToken();

      // Parse the second type.
      TypeRepr *SecondType = parseTypeIdentifier();
      if (!SecondType) {
        Invalid = true;
        break;
      }

      // Add the requirement
      Requirements.push_back(Requirement::getSameType(FirstType,
                                                      EqualLoc,
                                                      SecondType));
    } else {
      diagnose(Tok, diag::expected_requirement_delim);
      Invalid = true;
      break;
    }
    // If there's a comma, keep parsing the list.
  } while (consumeIf(tok::comma));

  return Invalid;
}
