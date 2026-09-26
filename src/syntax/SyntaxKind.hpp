/**
 *
 *  @file SyntaxKind.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright (c) 2026 Gaspard Kirira.
 *  https://github.com/vixcpp/vixc
 *
 *  Licensed under the MIT License.
 *  See LICENSE in the project root for license information.
 *
 *  VixC
 *
 */

#if !defined(VIXC_SYNTAX_SYNTAX_KIND_HPP)
#define VIXC_SYNTAX_SYNTAX_KIND_HPP

#include <string_view>

namespace vixc::syntax
{
  /**
   * @brief Classifies nodes produced by the VixC parser.
   *
   * SyntaxKind describes the structural role of a SyntaxNode independently from
   * its source spelling and semantic meaning.
   *
   * The VixC syntax tree is not intended to reproduce the complete C++ abstract
   * syntax tree. Ordinary C++ that does not require VixC interpretation can be
   * preserved as CxxRegion nodes while VixC-owned constructs receive explicit
   * syntax kinds.
   *
   * This allows the frontend to understand the language mechanisms it owns
   * without requiring a complete reimplementation of the C++ grammar in the
   * first frontend generation.
   *
   * Syntax kinds describe parsed structure only. Whether a construct is
   * semantically valid is decided later by semantic analysis.
   */
  enum class SyntaxKind
  {
    /**
     * @brief Invalid or unavailable syntax.
     *
     * Invalid nodes may be produced during parser recovery when a useful source
     * range exists but no valid syntax structure can be formed.
     */
    Invalid,

    /**
     * @brief Root node representing one complete source unit.
     *
     * A TranslationUnit contains the top-level syntax regions discovered while
     * processing the source.
     */
    TranslationUnit,

    /**
     * @brief Source region preserved as ordinary C++.
     *
     * CxxRegion allows VixC to retain source that it does not need to interpret
     * structurally. The original source range remains available for later
     * lowering and emission.
     */
    CxxRegion,

    /**
     * @brief Function declaration that owns a `fails` specification and body.
     *
     * The declaration retains its success type and declarator as source-ranged
     * children, its FailureSpecification, and ordered body fragments that may
     * contain VixC failure constructs or value-bearing returns. This provides
     * the declaration-scoped boundary required by semantic analysis without
     * reproducing the full C++ grammar.
     */
    FunctionDeclaration,

    /**
     * @brief Success return type of a failure-aware function declaration.
     */
    FunctionReturnType,

    /**
     * @brief Name, parameters, and retained declarator source of a function.
     */
    FunctionDeclarator,

    /**
     * @brief Identifier syntax.
     */
    Identifier,

    /**
     * @brief Integer literal syntax.
     */
    IntegerLiteral,

    /**
     * @brief Floating-point literal syntax.
     */
    FloatingLiteral,

    /**
     * @brief String literal syntax.
     */
    StringLiteral,

    /**
     * @brief Character literal syntax.
     */
    CharacterLiteral,

    /**
     * @brief Parenthesized expression understood by VixC.
     *
     * This kind represents grouping owned by the VixC parser when it is needed
     * to understand a VixC expression.
     */
    ParenthesizedExpression,

    /**
     * @brief Function failure specification introduced with `fails`.
     *
     * A FailureSpecification records the source-level declaration that an
     * operation can produce a recoverable failure.
     *
     * Semantic analysis determines the actual failure type and verifies whether
     * the specification is valid in its declaration context.
     */
    FailureSpecification,

    /**
     * @brief Explicit failure statement introduced with `fail`.
     *
     * A FailStatement represents an operation that terminates the current
     * failure-aware computation with a recoverable failure value.
     *
     * The parser records its structure. Semantic analysis determines whether
     * the statement appears in a valid failure-aware context and whether its
     * value satisfies the corresponding failure contract.
     */
    FailStatement,

    /**
     * @brief Return statement structurally owned by a failure-aware function.
     */
    ReturnStatement,

    /**
     * @brief Failure-propagating expression introduced with `try`.
     *
     * A TryExpression evaluates another operation and propagates its failure
     * according to the surrounding failure contract.
     *
     * The exact propagation semantics belong to semantic analysis and lowering,
     * not to the parser.
     */
    TryExpression
  };

  /**
   * @brief Returns the stable textual name of a syntax kind.
   *
   * The returned value identifies the SyntaxKind enum value and is intended for
   * diagnostics, debugging, tests, tracing, and frontend development.
   *
   * @param kind Syntax kind to describe.
   *
   * @return Stable textual name of the syntax kind.
   */
  [[nodiscard]]
  constexpr std::string_view
  syntax_kind_name(SyntaxKind kind) noexcept
  {
    switch (kind)
    {
    case SyntaxKind::Invalid:
      return "Invalid";

    case SyntaxKind::TranslationUnit:
      return "TranslationUnit";

    case SyntaxKind::CxxRegion:
      return "CxxRegion";

    case SyntaxKind::FunctionDeclaration:
      return "FunctionDeclaration";

    case SyntaxKind::FunctionReturnType:
      return "FunctionReturnType";

    case SyntaxKind::FunctionDeclarator:
      return "FunctionDeclarator";

    case SyntaxKind::Identifier:
      return "Identifier";

    case SyntaxKind::IntegerLiteral:
      return "IntegerLiteral";

    case SyntaxKind::FloatingLiteral:
      return "FloatingLiteral";

    case SyntaxKind::StringLiteral:
      return "StringLiteral";

    case SyntaxKind::CharacterLiteral:
      return "CharacterLiteral";

    case SyntaxKind::ParenthesizedExpression:
      return "ParenthesizedExpression";

    case SyntaxKind::FailureSpecification:
      return "FailureSpecification";

    case SyntaxKind::FailStatement:
      return "FailStatement";

    case SyntaxKind::ReturnStatement:
      return "ReturnStatement";

    case SyntaxKind::TryExpression:
      return "TryExpression";
    }

    return "Unknown";
  }

  /**
   * @brief Reports whether a syntax kind represents a literal.
   *
   * @param kind Syntax kind to inspect.
   *
   * @return true when the kind represents a literal, otherwise false.
   */
  [[nodiscard]]
  constexpr bool
  syntax_kind_is_literal(SyntaxKind kind) noexcept
  {
    return kind == SyntaxKind::IntegerLiteral || kind == SyntaxKind::FloatingLiteral || kind == SyntaxKind::StringLiteral || kind == SyntaxKind::CharacterLiteral;
  }

  /**
   * @brief Reports whether a syntax kind represents an expression.
   *
   * Only expressions structurally understood by the current VixC frontend are
   * classified here. Ordinary C++ expressions preserved inside CxxRegion are
   * not considered VixC expression nodes.
   *
   * @param kind Syntax kind to inspect.
   *
   * @return true when the kind represents a VixC expression.
   */
  [[nodiscard]]
  constexpr bool
  syntax_kind_is_expression(SyntaxKind kind) noexcept
  {
    return kind == SyntaxKind::Identifier || syntax_kind_is_literal(kind) || kind == SyntaxKind::ParenthesizedExpression || kind == SyntaxKind::TryExpression;
  }

  /**
   * @brief Reports whether a syntax kind belongs to failure syntax.
   *
   * @param kind Syntax kind to inspect.
   *
   * @return true for syntax introduced by the VixC failure model.
   */
  [[nodiscard]]
  constexpr bool
  syntax_kind_is_failure_construct(SyntaxKind kind) noexcept
  {
    return kind == SyntaxKind::FailureSpecification || kind == SyntaxKind::FailStatement || kind == SyntaxKind::TryExpression;
  }

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_SYNTAX_KIND_HPP
