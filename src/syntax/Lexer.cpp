/**
 *
 *  @file Lexer.cpp
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

#include "Lexer.hpp"

#include <vixc/SourceRange.hpp>

#include <cctype>
#include <string>
#include <utility>

namespace vixc::syntax
{
  Lexer::Lexer(
      SourceId source_id,
      std::string_view source,
      diagnostics::DiagnosticEngine &diagnostics) noexcept
      : source_id_(source_id),
        source_(source),
        diagnostics_(diagnostics)
  {
  }

  Token Lexer::next()
  {
    skip_trivia();

    if (at_end())
    {
      return make_token(
          TokenKind::EndOfFile,
          offset_,
          offset_);
    }

    const char value = current();

    if (is_identifier_start(value))
      return lex_identifier_or_keyword();

    if (is_digit(value))
      return lex_number();

    if (value == '.' && is_digit(peek()))
      return lex_number();

    if (value == '"')
      return lex_string_literal();

    if (value == '\'')
      return lex_character_literal();

    return lex_punctuation();
  }

  std::vector<Token> Lexer::lex_all()
  {
    std::vector<Token> tokens;

    while (true)
    {
      Token token = next();
      const bool finished = token.is_end_of_file();

      tokens.push_back(std::move(token));

      if (finished)
        break;
    }

    return tokens;
  }

  std::size_t Lexer::offset() const noexcept
  {
    return offset_;
  }

  bool Lexer::at_end() const noexcept
  {
    return offset_ >= source_.size();
  }

  char Lexer::current() const noexcept
  {
    if (at_end())
      return '\0';

    return source_[offset_];
  }

  char Lexer::peek(std::size_t distance) const noexcept
  {
    const std::size_t position = offset_ + distance;

    if (position >= source_.size())
      return '\0';

    return source_[position];
  }

  void Lexer::advance() noexcept
  {
    if (!at_end())
      ++offset_;
  }

  bool Lexer::match(char expected) noexcept
  {
    if (at_end() || current() != expected)
      return false;

    advance();
    return true;
  }

  void Lexer::skip_trivia()
  {
    while (!at_end())
    {
      const char value = current();

      if (std::isspace(static_cast<unsigned char>(value)) != 0)
      {
        advance();
        continue;
      }

      if (value == '/' && peek() == '/')
      {
        advance();
        advance();

        while (!at_end() && current() != '\n')
          advance();

        continue;
      }

      if (value == '/' && peek() == '*')
      {
        const std::size_t begin = offset_;

        advance();
        advance();

        bool terminated = false;

        while (!at_end())
        {
          if (current() == '*' && peek() == '/')
          {
            advance();
            advance();
            terminated = true;
            break;
          }

          advance();
        }

        if (!terminated)
        {
          report_unterminated(
              "unterminated block comment",
              begin,
              offset_);
        }

        continue;
      }

      break;
    }
  }

  Token Lexer::lex_identifier_or_keyword()
  {
    const std::size_t begin = offset_;

    advance();

    while (!at_end() && is_identifier_continue(current()))
      advance();

    const std::string_view text =
        source_.substr(begin, offset_ - begin);

    TokenKind kind = TokenKind::Identifier;

    if (text == "fail")
      kind = TokenKind::KeywordFail;
    else if (text == "fails")
      kind = TokenKind::KeywordFails;
    else if (text == "try")
      kind = TokenKind::KeywordTry;
    else if (text == "return")
      kind = TokenKind::KeywordReturn;

    return make_token(kind, begin, offset_);
  }

  Token Lexer::lex_number()
  {
    const std::size_t begin = offset_;

    bool is_hexadecimal = false;
    bool saw_dot = false;
    bool saw_exponent = false;

    if (current() == '.')
    {
      saw_dot = true;
      advance();
    }
    else if (
        current() == '0' && (peek() == 'x' || peek() == 'X'))
    {
      is_hexadecimal = true;
      advance();
      advance();
    }
    else
    {
      advance();
    }

    while (!at_end())
    {
      const char value = current();

      if (is_digit(value))
      {
        advance();
        continue;
      }

      if (
          (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || value == '_' || value == '\'')
      {
        const bool exponent_marker =
            (!is_hexadecimal && (value == 'e' || value == 'E')) || (is_hexadecimal && (value == 'p' || value == 'P'));

        if (exponent_marker)
        {
          saw_exponent = true;
          advance();

          if (current() == '+' || current() == '-')
            advance();

          continue;
        }

        advance();
        continue;
      }

      if (value == '.')
      {
        saw_dot = true;
        advance();
        continue;
      }

      break;
    }

    const TokenKind kind =
        saw_dot || saw_exponent
            ? TokenKind::FloatingLiteral
            : TokenKind::IntegerLiteral;

    return make_token(kind, begin, offset_);
  }

  Token Lexer::lex_string_literal()
  {
    const std::size_t begin = offset_;

    advance();

    while (!at_end())
    {
      const char value = current();

      if (value == '"')
      {
        advance();

        return make_token(
            TokenKind::StringLiteral,
            begin,
            offset_);
      }

      if (value == '\\')
      {
        advance();

        if (at_end())
          break;

        if (current() == '\r' && peek() == '\n')
        {
          advance();
          advance();
          continue;
        }

        advance();
        continue;
      }

      if (value == '\n' || value == '\r')
      {
        const std::size_t end = offset_;

        report_unterminated(
            "unterminated string literal",
            begin,
            end);

        return make_token(
            TokenKind::Invalid,
            begin,
            end);
      }

      advance();
    }

    report_unterminated(
        "unterminated string literal",
        begin,
        offset_);

    return make_token(
        TokenKind::Invalid,
        begin,
        offset_);
  }

  Token Lexer::lex_character_literal()
  {
    const std::size_t begin = offset_;

    advance();

    while (!at_end())
    {
      const char value = current();

      if (value == '\'')
      {
        advance();

        return make_token(
            TokenKind::CharacterLiteral,
            begin,
            offset_);
      }

      if (value == '\\')
      {
        advance();

        if (at_end())
          break;

        if (current() == '\r' && peek() == '\n')
        {
          advance();
          advance();
          continue;
        }

        advance();
        continue;
      }

      if (value == '\n' || value == '\r')
      {
        const std::size_t end = offset_;

        report_unterminated(
            "unterminated character literal",
            begin,
            end);

        return make_token(
            TokenKind::Invalid,
            begin,
            end);
      }

      advance();
    }

    report_unterminated(
        "unterminated character literal",
        begin,
        offset_);

    return make_token(
        TokenKind::Invalid,
        begin,
        offset_);
  }

  Token Lexer::lex_punctuation()
  {
    const std::size_t begin = offset_;
    const char value = current();

    advance();

    switch (value)
    {
    case '(':
      return make_token(
          TokenKind::LeftParen,
          begin,
          offset_);

    case ')':
      return make_token(
          TokenKind::RightParen,
          begin,
          offset_);

    case '{':
      return make_token(
          TokenKind::LeftBrace,
          begin,
          offset_);

    case '}':
      return make_token(
          TokenKind::RightBrace,
          begin,
          offset_);

    case '[':
      return make_token(
          TokenKind::LeftBracket,
          begin,
          offset_);

    case ']':
      return make_token(
          TokenKind::RightBracket,
          begin,
          offset_);

    case ',':
      return make_token(
          TokenKind::Comma,
          begin,
          offset_);

    case ';':
      return make_token(
          TokenKind::Semicolon,
          begin,
          offset_);

    case ':':
      if (match(':'))
      {
        return make_token(
            TokenKind::ColonColon,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Colon,
          begin,
          offset_);

    case '.':
      return make_token(
          TokenKind::Dot,
          begin,
          offset_);

    case '?':
      return make_token(
          TokenKind::Question,
          begin,
          offset_);

    case '=':
      if (match('='))
      {
        return make_token(
            TokenKind::EqualEqual,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Equal,
          begin,
          offset_);

    case '!':
      if (match('='))
      {
        return make_token(
            TokenKind::BangEqual,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Bang,
          begin,
          offset_);

    case '<':
      if (match('='))
      {
        return make_token(
            TokenKind::LessEqual,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Less,
          begin,
          offset_);

    case '>':
      if (match('='))
      {
        return make_token(
            TokenKind::GreaterEqual,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Greater,
          begin,
          offset_);

    case '+':
      return make_token(
          TokenKind::Plus,
          begin,
          offset_);

    case '-':
      if (match('>'))
      {
        return make_token(
            TokenKind::Arrow,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Minus,
          begin,
          offset_);

    case '*':
      return make_token(
          TokenKind::Star,
          begin,
          offset_);

    case '/':
      return make_token(
          TokenKind::Slash,
          begin,
          offset_);

    case '%':
      return make_token(
          TokenKind::Percent,
          begin,
          offset_);

    case '&':
      if (match('&'))
      {
        return make_token(
            TokenKind::AmpersandAmpersand,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Ampersand,
          begin,
          offset_);

    case '|':
      if (match('|'))
      {
        return make_token(
            TokenKind::PipePipe,
            begin,
            offset_);
      }

      return make_token(
          TokenKind::Pipe,
          begin,
          offset_);

    case '^':
      return make_token(
          TokenKind::Caret,
          begin,
          offset_);

    case '~':
      return make_token(
          TokenKind::Tilde,
          begin,
          offset_);

    default:
      return make_token(
          TokenKind::Invalid,
          begin,
          offset_);
    }
  }

  Token Lexer::make_token(
      TokenKind kind,
      std::size_t begin,
      std::size_t end) const noexcept
  {
    return Token{
        kind,
        SourceRange{
            source_id_,
            begin,
            end},
        source_.substr(
            begin,
            end - begin)};
  }

  void Lexer::report_unterminated(
      std::string_view message,
      std::size_t begin,
      std::size_t end)
  {
    diagnostics_.error(
        std::string{message},
        SourceRange{
            source_id_,
            begin,
            end});
  }

  bool Lexer::is_identifier_start(char value) noexcept
  {
    const unsigned char byte =
        static_cast<unsigned char>(value);

    return value == '_' || std::isalpha(byte) != 0;
  }

  bool Lexer::is_identifier_continue(char value) noexcept
  {
    const unsigned char byte =
        static_cast<unsigned char>(value);

    return value == '_' || std::isalnum(byte) != 0;
  }

  bool Lexer::is_digit(char value) noexcept
  {
    return value >= '0' && value <= '9';
  }

} // namespace vixc::syntax
