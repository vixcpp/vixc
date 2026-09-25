/**
 *
 *  @file Token.hpp
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

#if !defined(VIXC_SYNTAX_TOKEN_HPP)
#define VIXC_SYNTAX_TOKEN_HPP

#include "TokenKind.hpp"

#include <vixc/SourceRange.hpp>

#include <string_view>

namespace vixc::syntax
{
  /**
   * @brief Represents one lexical token in VixC source.
   *
   * Token combines a lexical category, its exact source spelling, and the
   * SourceRange from which that spelling originated.
   *
   * Tokens are intentionally lightweight. They do not own source text.
   * text() returns a std::string_view that normally refers directly into the
   * immutable contents of a SourceFile managed by the frontend.
   *
   * The underlying source storage must therefore outlive every Token that
   * refers to it. This matches the normal VixC frontend lifetime, where source
   * files are owned by SourceManager for the complete parsing and semantic
   * analysis operation.
   *
   * SourceRange is the canonical source position of the token. The stored text
   * exists so the parser and later syntax processing can inspect identifiers,
   * literals, and other variable spellings without repeatedly slicing the
   * source buffer.
   *
   * Token does not perform semantic interpretation. An Identifier token is only
   * an identifier at this stage, and literal values remain source text until a
   * later frontend stage decides how they should be interpreted.
   */
  class Token final
  {
  public:
    /**
     * @brief Creates an invalid token.
     *
     * The token has TokenKind::Invalid, an invalid SourceRange, and an empty
     * source spelling.
     */
    constexpr Token() noexcept = default;

    /**
     * @brief Creates a token from its kind, source range, and source spelling.
     *
     * The text view is not copied. The caller must ensure that the referenced
     * character storage remains valid for the lifetime of this token.
     *
     * Token does not verify that the size of text matches the size of range.
     * Maintaining that relationship is the responsibility of the lexer that
     * constructs the token.
     *
     * @param kind Lexical category of the token.
     * @param range Source range occupied by the token.
     * @param text Exact source spelling of the token.
     */
    constexpr Token(
        TokenKind kind,
        SourceRange range,
        std::string_view text) noexcept
        : kind_(kind),
          range_(range),
          text_(text)
    {
    }

    /**
     * @brief Returns the lexical category of the token.
     *
     * @return Token kind assigned by the lexer.
     */
    [[nodiscard]]
    constexpr TokenKind kind() const noexcept
    {
      return kind_;
    }

    /**
     * @brief Returns the source range occupied by the token.
     *
     * The range follows SourceRange half-open semantics. For ordinary tokens,
     * begin() identifies the first source byte and end() identifies the position
     * immediately following the final source byte.
     *
     * EndOfFile normally uses a valid zero-length range at the end-of-file
     * position.
     *
     * @return Source range associated with the token.
     */
    [[nodiscard]]
    constexpr SourceRange range() const noexcept
    {
      return range_;
    }

    /**
     * @brief Returns the exact source spelling of the token.
     *
     * For identifiers and literals, this is the original spelling written in
     * the source. For punctuation and keywords, it is the exact matched text.
     *
     * EndOfFile normally has an empty spelling.
     *
     * The returned view does not own its character storage.
     *
     * @return Non-owning view of the token source text.
     */
    [[nodiscard]]
    constexpr std::string_view text() const noexcept
    {
      return text_;
    }

    /**
     * @brief Returns the number of source bytes in the token spelling.
     *
     * @return Size of text() in bytes.
     */
    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      return text_.size();
    }

    /**
     * @brief Reports whether the token has an empty source spelling.
     *
     * EndOfFile normally returns true. Ordinary lexical tokens normally return
     * false.
     *
     * @return true when text() is empty, otherwise false.
     */
    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
      return text_.empty();
    }

    /**
     * @brief Reports whether the token has the requested kind.
     *
     * @param expected Token kind to compare against.
     *
     * @return true when kind() equals expected, otherwise false.
     */
    [[nodiscard]]
    constexpr bool is(TokenKind expected) const noexcept
    {
      return kind_ == expected;
    }

    /**
     * @brief Reports whether the token is a VixC reserved word.
     *
     * @return true when kind() is one of the VixC keyword token kinds.
     */
    [[nodiscard]]
    constexpr bool is_keyword() const noexcept
    {
      return token_kind_is_keyword(kind_);
    }

    /**
     * @brief Reports whether the token represents a literal.
     *
     * @return true when kind() represents an integer, floating-point, string,
     *         or character literal.
     */
    [[nodiscard]]
    constexpr bool is_literal() const noexcept
    {
      return token_kind_is_literal(kind_);
    }

    /**
     * @brief Reports whether this token is the end-of-file marker.
     *
     * @return true when kind() is TokenKind::EndOfFile.
     */
    [[nodiscard]]
    constexpr bool is_end_of_file() const noexcept
    {
      return kind_ == TokenKind::EndOfFile;
    }

    /**
     * @brief Reports whether the token represents invalid source text.
     *
     * @return true when kind() is TokenKind::Invalid.
     */
    [[nodiscard]]
    constexpr bool is_invalid() const noexcept
    {
      return kind_ == TokenKind::Invalid;
    }

    /**
     * @brief Compares the token spelling with a source string.
     *
     * This operation is useful for identifiers and other tokens whose spelling
     * carries information beyond their TokenKind.
     *
     * @param spelling Source spelling to compare with text().
     *
     * @return true when both spellings are identical.
     */
    [[nodiscard]]
    constexpr bool text_is(std::string_view spelling) const noexcept
    {
      return text_ == spelling;
    }

  private:
    /// Lexical category assigned to the token.
    TokenKind kind_{TokenKind::Invalid};

    /// Original source region occupied by the token.
    SourceRange range_{};

    /// Non-owning view of the token spelling in the source buffer.
    std::string_view text_{};
  };

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_TOKEN_HPP
