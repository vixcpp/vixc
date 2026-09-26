/**
 *
 *  @file Parser.hpp
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

#if !defined(VIXC_SYNTAX_PARSER_HPP)
#define VIXC_SYNTAX_PARSER_HPP

#include "SyntaxNode.hpp"
#include "Token.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <vector>

namespace vixc::syntax
{
  /**
   * @brief Builds VixC syntax structure from a lexical token stream.
   *
   * Parser consumes tokens produced by Lexer and identifies the syntax that the
   * VixC frontend owns. The first parser generation is intentionally focused on
   * VixC constructs rather than attempting to reproduce the complete C++
   * grammar.
   *
   * Source that does not require VixC structural interpretation is preserved as
   * CxxRegion syntax. This allows ordinary C++ to remain connected to its
   * original source while VixC constructs receive explicit syntax nodes.
   *
   * The first VixC syntax handled by the parser is the failure model:
   * FunctionDeclaration groups a C++ declaration prefix, FailureSpecification
   * for `fails`, and its body; FailStatement represents `fail`; and
   * TryExpression represents failure propagation with `try`.
   *
   * The parser is responsible only for source structure. It does not decide
   * whether a failure type is valid, whether `fail` appears inside a
   * failure-aware function, whether a propagated operation has compatible
   * failure semantics, or whether the surrounding C++ program is semantically
   * valid. Those decisions belong to semantic analysis.
   *
   * Parser diagnostics describe structural failures such as a missing failure
   * type, missing expression, or missing statement terminator. Diagnostics are
   * returned through DiagnosticEngine and are never printed directly.
   *
   * Tokens are owned by Parser after construction. Their source text remains
   * non-owning, so the original source buffer must outlive the parser and every
   * syntax object whose ranges refer to that source.
   */
  class Parser final
  {
  public:
    /**
     * @brief Creates a parser from a complete token stream.
     *
     * The normal input is the result of Lexer::lex_all(), including its final
     * EndOfFile token.
     *
     * Parser takes ownership of the token collection.
     *
     * @param tokens Tokens in source order.
     * @param diagnostics Diagnostic engine used for parser diagnostics.
     */
    Parser(
        std::vector<Token> tokens,
        diagnostics::DiagnosticEngine &diagnostics) noexcept;

    Parser(const Parser &) = delete;
    Parser &operator=(const Parser &) = delete;

    Parser(Parser &&) = delete;
    Parser &operator=(Parser &&) = delete;

    ~Parser() = default;

    /**
     * @brief Parses the token stream as one translation unit.
     *
     * The returned node has SyntaxKind::TranslationUnit. Its children preserve
     * the source order of ordinary C++ regions and VixC-owned syntax.
     *
     * Calling parse() consumes the remaining parser input.
     *
     * @return Root syntax node for the translation unit.
     */
    [[nodiscard]]
    SyntaxNode parse();

    /**
     * @brief Returns the current token index.
     *
     * @return Zero-based parser position in the token collection.
     */
    [[nodiscard]]
    std::size_t position() const noexcept;

    /**
     * @brief Reports whether the parser reached the logical end of input.
     *
     * End of input is reached when the current token is EndOfFile or when the
     * parser position lies beyond the supplied token collection.
     *
     * @return true when no ordinary source token remains.
     */
    [[nodiscard]]
    bool at_end() const noexcept;

  private:
    /**
     * @brief Returns the token at the current parser position.
     *
     * An invalid token is returned when the position lies outside the token
     * collection.
     *
     * @return Current token.
     */
    [[nodiscard]]
    const Token &current() const noexcept;

    /**
     * @brief Returns the previously consumed token.
     *
     * An invalid token is returned when no token has been consumed.
     *
     * @return Previous token.
     */
    [[nodiscard]]
    const Token &previous() const noexcept;

    /**
     * @brief Returns a token ahead of the current parser position.
     *
     * @param distance Number of token positions ahead.
     *
     * @return Requested token, or an invalid token when it lies outside the
     *         token collection.
     */
    [[nodiscard]]
    const Token &peek(std::size_t distance = 1) const noexcept;

    /**
     * @brief Reports whether the current token has a specific kind.
     *
     * @param kind Token kind to test.
     *
     * @return true when the current token matches kind.
     */
    [[nodiscard]]
    bool check(TokenKind kind) const noexcept;

    /**
     * @brief Consumes the current token when it has the requested kind.
     *
     * @param kind Token kind to match.
     *
     * @return true when a matching token was consumed.
     */
    bool match(TokenKind kind) noexcept;

    /**
     * @brief Consumes and returns the current token.
     *
     * EndOfFile is not consumed.
     *
     * @return Token that was current before advancing.
     */
    const Token &advance() noexcept;

    /**
     * @brief Parses the remaining source as a translation unit.
     *
     * @return TranslationUnit syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_translation_unit();

    /**
     * @brief Parses the next source structure.
     *
     * The parser selects between a VixC-owned construct and an ordinary C++
     * region while preserving source order.
     *
     * @return Next syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_next();

    /**
     * @brief Preserves ordinary source as one CxxRegion node.
     *
     * Tokens are consumed until the next VixC construct or EndOfFile.
     *
     * @return CxxRegion syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_cxx_region();

    /**
     * @brief Parses a failure-aware function declaration after its C++ prefix.
     *
     * The declaration owns the ordinary C++ signature, the `fails`
     * specification, and all body fragments through the matching closing brace.
     *
     * @param declaration_prefix Ordinary C++ function signature before `fails`.
     * @return FunctionDeclaration syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_failure_aware_function_declaration(
        SyntaxNode declaration_prefix);

    /**
     * @brief Parses a value-bearing return in a failure-aware function body.
     *
     * This parser intentionally recognizes `return expression;` only while
     * FunctionDeclaration owns the enclosing body. Bare returns for `void`
     * failure-aware functions remain outside the first Failure lowering slice.
     *
     * @return ReturnStatement syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_return_statement();

    /**
     * @brief Finds the matching opening parenthesis for a closing parenthesis.
     *
     * @param closing_index Index of a RightParen token.
     *
     * @return Matching LeftParen token index, or tokens_.size() when absent.
     */
    [[nodiscard]]
    std::size_t matching_left_paren(
        std::size_t closing_index) const noexcept;

    /**
     * @brief Finds the first token of the final declaration prefix segment.
     *
     * The scan respects brace nesting so earlier complete declarations are not
     * absorbed into the retained success return type of a `fails` declaration.
     *
     * @param begin_index First token of the opaque prefix region.
     * @param end_index One past the final token before `fails`.
     *
     * @return First token of the final declaration segment.
     */
    [[nodiscard]]
    std::size_t declaration_begin(
        std::size_t begin_index,
        std::size_t end_index) const noexcept;

    /**
     * @brief Parses a `fail` statement.
     *
     * The statement begins with KeywordFail and extends through its terminating
     * semicolon when one is present.
     *
     * The payload remains structurally opaque unless it begins with another
     * VixC construct that the parser explicitly understands.
     *
     * @return FailStatement syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_fail_statement();

    /**
     * @brief Parses a `fails` failure specification.
     *
     * The first frontend generation preserves the failure type as source syntax
     * until semantic analysis establishes its meaning.
     *
     * The specification ends before the declaration body or declaration
     * semicolon.
     *
     * @return FailureSpecification syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_failure_specification();

    /**
     * @brief Parses a failure-propagating `try` expression.
     *
     * The operand is preserved as source syntax until semantic analysis and
     * later frontend stages interpret it.
     *
     * @return TryExpression syntax node.
     */
    [[nodiscard]]
    SyntaxNode parse_try_expression();

    /**
     * @brief Reports whether the current token begins syntax owned by VixC.
     *
     * Native C++ `try { ... }` remains ordinary C++ and is therefore not
     * classified as a VixC TryExpression.
     *
     * @return true when the current position begins a VixC construct.
     */
    [[nodiscard]]
    bool starts_vixc_construct() const noexcept;

    /**
     * @brief Scans an opaque expression-like region.
     *
     * Tokens are consumed until a source-level delimiter is reached at the
     * outer nesting depth. Nested parentheses, brackets, and braces remain part
     * of the scanned region.
     *
     * @param stop_at_semicolon Stop before an outer semicolon.
     * @param stop_at_comma Stop before an outer comma.
     * @param stop_at_right_paren Stop before an unmatched outer right
     *        parenthesis.
     * @param stop_at_right_bracket Stop before an unmatched outer right
     *        bracket.
     * @param stop_at_right_brace Stop before an unmatched outer right brace.
     *
     * @return Token index immediately after the consumed region.
     */
    std::size_t scan_opaque_region(
        bool stop_at_semicolon,
        bool stop_at_comma,
        bool stop_at_right_paren,
        bool stop_at_right_bracket,
        bool stop_at_right_brace);

    /**
     * @brief Creates a CxxRegion from a token interval.
     *
     * The interval is half-open and expressed using token indexes.
     *
     * @param begin_index Index of the first token.
     * @param end_index Index immediately after the last token.
     *
     * @return CxxRegion node, or Invalid when the interval is empty.
     */
    [[nodiscard]]
    SyntaxNode make_cxx_region(
        std::size_t begin_index,
        std::size_t end_index) const noexcept;

    /**
     * @brief Creates one source range from a token interval.
     *
     * The interval is half-open. Both endpoint tokens must belong to the same
     * source.
     *
     * @param begin_index Index of the first token.
     * @param end_index Index immediately after the final token.
     *
     * @return Combined source range, or an invalid range when the interval
     *         cannot form one.
     */
    [[nodiscard]]
    SourceRange range_from_tokens(
        std::size_t begin_index,
        std::size_t end_index) const noexcept;

    /**
     * @brief Emits a parser error diagnostic.
     *
     * @param code Stable parser diagnostic code.
     * @param message Human-readable description.
     * @param range Source range associated with the error.
     */
    void report_error(
        const char *code,
        const char *message,
        SourceRange range);

    /// Token stream owned by the parser.
    std::vector<Token> tokens_;

    /// Diagnostic sink shared with the surrounding frontend operation.
    diagnostics::DiagnosticEngine &diagnostics_;

    /// Index of the current token.
    std::size_t position_{0};
  };

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_PARSER_HPP
