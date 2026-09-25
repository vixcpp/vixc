/**
 *
 *  @file Lexer.hpp
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

#if !defined(VIXC_SYNTAX_LEXER_HPP)
#define VIXC_SYNTAX_LEXER_HPP

#include "Token.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"

#include <vixc/SourceLocation.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace vixc::syntax
{
  /**
   * @brief Converts source text into lexical tokens understood by VixC.
   *
   * Lexer is the first syntax-processing stage of the VixC frontend. It scans
   * one immutable source buffer from beginning to end and produces Token
   * objects whose ranges refer directly to positions in the original source.
   *
   * The lexer recognizes identifiers, literals, punctuation, and the keywords
   * owned by the VixC language. Ordinary C++ words that are not reserved by
   * VixC remain Identifier tokens. This allows later frontend stages to decide
   * which parts of the surrounding C++ program require VixC interpretation
   * without teaching the lexer the semantic meaning of every C++ construct.
   *
   * Whitespace and comments are treated as trivia and are not emitted as
   * tokens. Their bytes remain present in the original source buffer, so later
   * stages that preserve or rewrite source can continue to use the original
   * source text rather than reconstructing it from tokens.
   *
   * The lexer is byte-oriented. SourceLocation and SourceRange offsets refer to
   * byte positions in the original input. Character encoding and display-column
   * interpretation belong to higher-level source and diagnostic presentation
   * facilities.
   *
   * Lexer does not own the source text. The supplied std::string_view must
   * remain valid for the lifetime of the lexer and for the lifetime of every
   * Token returned from it.
   *
   * Lexical diagnostics are reported through DiagnosticEngine. The lexer does
   * not print diagnostics or otherwise perform presentation.
   */
  class Lexer final
  {
  public:
    /**
     * @brief Creates a lexer for one source unit.
     *
     * The lexer starts at byte offset zero.
     *
     * The source identifier is attached to every Token and diagnostic range
     * produced by this lexer.
     *
     * @param source_id Identifier assigned to the source unit.
     * @param source Complete immutable source text.
     * @param diagnostics Diagnostic engine that receives lexical diagnostics.
     */
    Lexer(
        SourceId source_id,
        std::string_view source,
        diagnostics::DiagnosticEngine &diagnostics) noexcept;

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;

    Lexer(Lexer &&) = delete;
    Lexer &operator=(Lexer &&) = delete;

    ~Lexer() = default;

    /**
     * @brief Lexes and returns the next token.
     *
     * Leading whitespace and comments are skipped before the token is read.
     *
     * Once the source has been consumed, repeated calls return EndOfFile tokens
     * at the source end position.
     *
     * @return Next token in source order.
     */
    [[nodiscard]]
    Token next();

    /**
     * @brief Lexes the complete remaining input.
     *
     * Tokens are returned in source order and the resulting collection always
     * ends with exactly one EndOfFile token.
     *
     * Calling lex_all() after partially consuming the lexer processes only the
     * unconsumed remainder of the source.
     *
     * @return Tokens from the current lexer position through EndOfFile.
     */
    [[nodiscard]]
    std::vector<Token> lex_all();

    /**
     * @brief Returns the current byte offset in the source.
     *
     * The offset identifies the next byte that will be examined before trivia
     * processing.
     *
     * @return Current zero-based source byte offset.
     */
    [[nodiscard]]
    std::size_t offset() const noexcept;

    /**
     * @brief Reports whether all source bytes have been consumed.
     *
     * @return true when the current offset is at or beyond source.size().
     */
    [[nodiscard]]
    bool at_end() const noexcept;

  private:
    /**
     * @brief Returns the byte at the current lexer position.
     *
     * The function returns '\\0' when the lexer is at end of input.
     *
     * @return Current source byte or '\\0' at end of input.
     */
    [[nodiscard]]
    char current() const noexcept;

    /**
     * @brief Returns a byte ahead of the current lexer position.
     *
     * @param distance Number of bytes ahead of the current position.
     *
     * @return Requested source byte or '\\0' when the position is outside the
     *         source.
     */
    [[nodiscard]]
    char peek(std::size_t distance = 1) const noexcept;

    /**
     * @brief Advances the lexer by one source byte.
     *
     * Calling advance() at end of input has no effect.
     */
    void advance() noexcept;

    /**
     * @brief Consumes an expected byte when it is present.
     *
     * @param expected Byte expected at the current position.
     *
     * @return true when the byte matched and was consumed, otherwise false.
     */
    [[nodiscard]]
    bool match(char expected) noexcept;

    /**
     * @brief Skips whitespace and comments.
     *
     * Line comments are consumed through the byte before the terminating
     * newline. Block comments are consumed through their closing marker.
     *
     * Unterminated block comments produce a lexical diagnostic.
     */
    void skip_trivia();

    /**
     * @brief Lexes an identifier or VixC reserved word.
     *
     * @return Identifier or keyword token.
     */
    [[nodiscard]]
    Token lex_identifier_or_keyword();

    /**
     * @brief Lexes a numeric literal.
     *
     * The scanner preserves the original spelling and distinguishes integer
     * and floating-point forms using lexical markers such as decimal points
     * and exponent syntax.
     *
     * @return Numeric literal token.
     */
    [[nodiscard]]
    Token lex_number();

    /**
     * @brief Lexes a string literal beginning at the current position.
     *
     * Escape sequences are preserved as source text rather than interpreted by
     * the lexer.
     *
     * Unterminated literals are returned as Invalid tokens and produce a
     * diagnostic.
     *
     * @return StringLiteral or Invalid token.
     */
    [[nodiscard]]
    Token lex_string_literal();

    /**
     * @brief Lexes a character literal beginning at the current position.
     *
     * Escape sequences are preserved as source text rather than interpreted by
     * the lexer.
     *
     * Unterminated literals are returned as Invalid tokens and produce a
     * diagnostic.
     *
     * @return CharacterLiteral or Invalid token.
     */
    [[nodiscard]]
    Token lex_character_literal();

    /**
     * @brief Lexes punctuation from the current source position.
     *
     * Multi-byte punctuation recognized by the current token vocabulary is
     * preferred over its single-byte forms.
     *
     * Source bytes outside the current VixC token vocabulary are preserved as
     * Invalid tokens. Their presence alone does not cause the lexer to invent a
     * semantic interpretation for them.
     *
     * @return Punctuation token or Invalid token.
     */
    [[nodiscard]]
    Token lex_punctuation();

    /**
     * @brief Creates a token for a source interval.
     *
     * @param kind Token kind.
     * @param begin Inclusive byte offset.
     * @param end Exclusive byte offset.
     *
     * @return Token referring directly to the corresponding source bytes.
     */
    [[nodiscard]]
    Token make_token(
        TokenKind kind,
        std::size_t begin,
        std::size_t end) const noexcept;

    /**
     * @brief Reports an unterminated lexical construct.
     *
     * @param message Human-readable diagnostic message.
     * @param begin Beginning byte offset of the malformed construct.
     * @param end Exclusive ending byte offset reached by the lexer.
     */
    void report_unterminated(
        std::string_view message,
        std::size_t begin,
        std::size_t end);

    /**
     * @brief Reports whether a byte can begin an identifier.
     *
     * The first frontend generation recognizes ASCII alphabetic bytes and
     * underscore here. Extended identifier handling can be added without
     * changing Token or SourceRange representation.
     *
     * @param value Byte to inspect.
     *
     * @return true when the byte may begin an identifier.
     */
    [[nodiscard]]
    static bool is_identifier_start(char value) noexcept;

    /**
     * @brief Reports whether a byte can continue an identifier.
     *
     * @param value Byte to inspect.
     *
     * @return true when the byte may continue an identifier.
     */
    [[nodiscard]]
    static bool is_identifier_continue(char value) noexcept;

    /**
     * @brief Reports whether a byte is an ASCII decimal digit.
     *
     * @param value Byte to inspect.
     *
     * @return true for bytes '0' through '9'.
     */
    [[nodiscard]]
    static bool is_digit(char value) noexcept;

    /// Identifier of the source currently being lexed.
    SourceId source_id_;

    /// Non-owning view of the immutable source text.
    std::string_view source_;

    /// Diagnostic sink used for lexical failures.
    diagnostics::DiagnosticEngine &diagnostics_;

    /// Current zero-based byte offset in source_.
    std::size_t offset_{0};
  };

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_LEXER_HPP
