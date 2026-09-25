/**
 *
 *  @file TokenKind.hpp
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

#if !defined(VIXC_SYNTAX_TOKEN_KIND_HPP)
#define VIXC_SYNTAX_TOKEN_KIND_HPP

#include <string_view>

namespace vixc::syntax
{

  /**
   * @brief Classifies lexical tokens recognized by the VixC frontend.
   *
   * TokenKind describes the lexical category of a token independently from its
   * exact source spelling. The lexer assigns one TokenKind to every token it
   * produces, while Token stores the corresponding source range and text.
   *
   * The token set is intentionally centered on the syntax VixC needs to
   * understand. VixC is not required to duplicate the complete C++ grammar
   * before it can recognize its own language constructs.
   *
   * Ordinary identifiers remain Identifier unless their spelling is reserved by
   * the VixC language. The first reserved words are associated with failure and
   * propagation semantics.
   *
   * Invalid represents source text that could not be classified as a valid
   * token. EndOfFile represents the logical position immediately after the last
   * source token.
   */
  enum class TokenKind
  {
    /// Source text that could not be classified as a valid token.
    Invalid,

    /// Logical end of the source input.
    EndOfFile,

    /// User-defined or otherwise non-reserved identifier.
    Identifier,

    /// Integer numeric literal.
    IntegerLiteral,

    /// Floating-point numeric literal.
    FloatingLiteral,

    /// String literal.
    StringLiteral,

    /// Character literal.
    CharacterLiteral,

    /// `(`
    LeftParen,

    /// `)`
    RightParen,

    /// `{`
    LeftBrace,

    /// `}`
    RightBrace,

    /// `[`
    LeftBracket,

    /// `]`
    RightBracket,

    /// `,`
    Comma,

    /// `;`
    Semicolon,

    /// `:`
    Colon,

    /// `::`
    ColonColon,

    /// `.`
    Dot,

    /// `->`
    Arrow,

    /// `?`
    Question,

    /// `=`
    Equal,

    /// `==`
    EqualEqual,

    /// `!`
    Bang,

    /// `!=`
    BangEqual,

    /// `<`
    Less,

    /// `<=`
    LessEqual,

    /// `>`
    Greater,

    /// `>=`
    GreaterEqual,

    /// `+`
    Plus,

    /// `-`
    Minus,

    /// `*`
    Star,

    /// `/`
    Slash,

    /// `%`
    Percent,

    /// `&`
    Ampersand,

    /// `&&`
    AmpersandAmpersand,

    /// `|`
    Pipe,

    /// `||`
    PipePipe,

    /// `^`
    Caret,

    /// `~`
    Tilde,

    /**
     * @brief `fail`
     *
     * Introduces an explicit recoverable failure in a VixC failure-aware
     * context.
     */
    KeywordFail,

    /**
     * @brief `fails`
     *
     * Declares that an operation participates in VixC failure semantics.
     */
    KeywordFails,

    /**
     * @brief `try`
     *
     * Propagates failure from another failure-aware operation.
     */
    KeywordTry
  };

  /**
   * @brief Returns the stable descriptive name of a token kind.
   *
   * The returned string identifies the enum value rather than necessarily
   * reproducing its source spelling. It is suitable for debugging, diagnostics,
   * tests, tracing, and parser error messages.
   *
   * For example, TokenKind::LeftParen returns "LeftParen", while
   * TokenKind::KeywordFail returns "KeywordFail".
   *
   * @param kind Token kind to describe.
   *
   * @return Stable textual name of the token kind.
   */
  [[nodiscard]]
  constexpr std::string_view
  token_kind_name(TokenKind kind) noexcept
  {
    switch (kind)
    {
    case TokenKind::Invalid:
      return "Invalid";

    case TokenKind::EndOfFile:
      return "EndOfFile";

    case TokenKind::Identifier:
      return "Identifier";

    case TokenKind::IntegerLiteral:
      return "IntegerLiteral";

    case TokenKind::FloatingLiteral:
      return "FloatingLiteral";

    case TokenKind::StringLiteral:
      return "StringLiteral";

    case TokenKind::CharacterLiteral:
      return "CharacterLiteral";

    case TokenKind::LeftParen:
      return "LeftParen";

    case TokenKind::RightParen:
      return "RightParen";

    case TokenKind::LeftBrace:
      return "LeftBrace";

    case TokenKind::RightBrace:
      return "RightBrace";

    case TokenKind::LeftBracket:
      return "LeftBracket";

    case TokenKind::RightBracket:
      return "RightBracket";

    case TokenKind::Comma:
      return "Comma";

    case TokenKind::Semicolon:
      return "Semicolon";

    case TokenKind::Colon:
      return "Colon";

    case TokenKind::ColonColon:
      return "ColonColon";

    case TokenKind::Dot:
      return "Dot";

    case TokenKind::Arrow:
      return "Arrow";

    case TokenKind::Question:
      return "Question";

    case TokenKind::Equal:
      return "Equal";

    case TokenKind::EqualEqual:
      return "EqualEqual";

    case TokenKind::Bang:
      return "Bang";

    case TokenKind::BangEqual:
      return "BangEqual";

    case TokenKind::Less:
      return "Less";

    case TokenKind::LessEqual:
      return "LessEqual";

    case TokenKind::Greater:
      return "Greater";

    case TokenKind::GreaterEqual:
      return "GreaterEqual";

    case TokenKind::Plus:
      return "Plus";

    case TokenKind::Minus:
      return "Minus";

    case TokenKind::Star:
      return "Star";

    case TokenKind::Slash:
      return "Slash";

    case TokenKind::Percent:
      return "Percent";

    case TokenKind::Ampersand:
      return "Ampersand";

    case TokenKind::AmpersandAmpersand:
      return "AmpersandAmpersand";

    case TokenKind::Pipe:
      return "Pipe";

    case TokenKind::PipePipe:
      return "PipePipe";

    case TokenKind::Caret:
      return "Caret";

    case TokenKind::Tilde:
      return "Tilde";

    case TokenKind::KeywordFail:
      return "KeywordFail";

    case TokenKind::KeywordFails:
      return "KeywordFails";

    case TokenKind::KeywordTry:
      return "KeywordTry";
    }

    return "Unknown";
  }

  /**
   * @brief Returns the fixed source spelling of a token kind when one exists.
   *
   * Identifiers and literals do not have a fixed spelling and therefore return
   * an empty string. Invalid and EndOfFile also return an empty string.
   *
   * @param kind Token kind whose fixed spelling is requested.
   *
   * @return Fixed token spelling, or an empty string when the token kind has no
   *         single source spelling.
   */
  [[nodiscard]]
  constexpr std::string_view
  token_kind_spelling(TokenKind kind) noexcept
  {
    switch (kind)
    {
    case TokenKind::LeftParen:
      return "(";

    case TokenKind::RightParen:
      return ")";

    case TokenKind::LeftBrace:
      return "{";

    case TokenKind::RightBrace:
      return "}";

    case TokenKind::LeftBracket:
      return "[";

    case TokenKind::RightBracket:
      return "]";

    case TokenKind::Comma:
      return ",";

    case TokenKind::Semicolon:
      return ";";

    case TokenKind::Colon:
      return ":";

    case TokenKind::ColonColon:
      return "::";

    case TokenKind::Dot:
      return ".";

    case TokenKind::Arrow:
      return "->";

    case TokenKind::Question:
      return "?";

    case TokenKind::Equal:
      return "=";

    case TokenKind::EqualEqual:
      return "==";

    case TokenKind::Bang:
      return "!";

    case TokenKind::BangEqual:
      return "!=";

    case TokenKind::Less:
      return "<";

    case TokenKind::LessEqual:
      return "<=";

    case TokenKind::Greater:
      return ">";

    case TokenKind::GreaterEqual:
      return ">=";

    case TokenKind::Plus:
      return "+";

    case TokenKind::Minus:
      return "-";

    case TokenKind::Star:
      return "*";

    case TokenKind::Slash:
      return "/";

    case TokenKind::Percent:
      return "%";

    case TokenKind::Ampersand:
      return "&";

    case TokenKind::AmpersandAmpersand:
      return "&&";

    case TokenKind::Pipe:
      return "|";

    case TokenKind::PipePipe:
      return "||";

    case TokenKind::Caret:
      return "^";

    case TokenKind::Tilde:
      return "~";

    case TokenKind::KeywordFail:
      return "fail";

    case TokenKind::KeywordFails:
      return "fails";

    case TokenKind::KeywordTry:
      return "try";

    case TokenKind::Invalid:
    case TokenKind::EndOfFile:
    case TokenKind::Identifier:
    case TokenKind::IntegerLiteral:
    case TokenKind::FloatingLiteral:
    case TokenKind::StringLiteral:
    case TokenKind::CharacterLiteral:
      return "";
    }

    return "";
  }

  /**
   * @brief Reports whether a token kind is a VixC reserved word.
   *
   * @param kind Token kind to inspect.
   *
   * @return true when the token represents a VixC keyword, otherwise false.
   */
  [[nodiscard]]
  constexpr bool
  token_kind_is_keyword(TokenKind kind) noexcept
  {
    return kind == TokenKind::KeywordFail || kind == TokenKind::KeywordFails || kind == TokenKind::KeywordTry;
  }

  /**
   * @brief Reports whether a token kind represents a literal.
   *
   * @param kind Token kind to inspect.
   *
   * @return true for integer, floating-point, string, and character literals,
   *         otherwise false.
   */
  [[nodiscard]]
  constexpr bool
  token_kind_is_literal(TokenKind kind) noexcept
  {
    return kind == TokenKind::IntegerLiteral || kind == TokenKind::FloatingLiteral || kind == TokenKind::StringLiteral || kind == TokenKind::CharacterLiteral;
  }

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_TOKEN_KIND_HPP
