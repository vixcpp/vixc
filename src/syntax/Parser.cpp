/**
 *
 *  @file Parser.cpp
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

#include "Parser.hpp"

#include <vixc/DiagnosticSeverity.hpp>

#include <utility>

namespace vixc::syntax
{

  namespace
  {

    const Token &invalid_token() noexcept
    {
      static const Token token{};
      return token;
    }

  } // namespace

  Parser::Parser(
      std::vector<Token> tokens,
      diagnostics::DiagnosticEngine &diagnostics) noexcept
      : tokens_(std::move(tokens)),
        diagnostics_(diagnostics)
  {
  }

  SyntaxNode Parser::parse()
  {
    return parse_translation_unit();
  }

  std::size_t Parser::position() const noexcept
  {
    return position_;
  }

  bool Parser::at_end() const noexcept
  {
    return position_ >= tokens_.size() || current().is_end_of_file();
  }

  const Token &Parser::current() const noexcept
  {
    if (position_ >= tokens_.size())
      return invalid_token();

    return tokens_[position_];
  }

  const Token &Parser::previous() const noexcept
  {
    if (position_ == 0 || tokens_.empty())
      return invalid_token();

    return tokens_[position_ - 1];
  }

  const Token &Parser::peek(std::size_t distance) const noexcept
  {
    if (position_ >= tokens_.size())
      return invalid_token();

    if (distance >= tokens_.size() - position_)
      return invalid_token();

    return tokens_[position_ + distance];
  }

  bool Parser::check(TokenKind kind) const noexcept
  {
    return current().is(kind);
  }

  bool Parser::match(TokenKind kind) noexcept
  {
    if (!check(kind))
      return false;

    advance();
    return true;
  }

  const Token &Parser::advance() noexcept
  {
    if (at_end())
      return current();

    ++position_;
    return previous();
  }

  SyntaxNode Parser::parse_translation_unit()
  {
    SyntaxNode root{
        SyntaxKind::TranslationUnit,
        SourceRange{}};

    while (!at_end())
    {
      const std::size_t before = position_;

      SyntaxNode node = parse_next();

      if (!node.is_invalid())
        root.add_child(std::move(node));

      if (position_ == before)
        advance();
    }

    if (tokens_.empty())
      return root;

    std::size_t first_index = 0;
    std::size_t end_index = tokens_.size();

    if (tokens_.back().is_end_of_file())
      end_index = tokens_.size() - 1;

    if (end_index > first_index)
    {
      return SyntaxNode{
          SyntaxKind::TranslationUnit,
          range_from_tokens(first_index, end_index),
          root.children()};
    }

    if (tokens_.back().is_end_of_file())
    {
      return SyntaxNode{
          SyntaxKind::TranslationUnit,
          tokens_.back().range(),
          root.children()};
    }

    return root;
  }

  SyntaxNode Parser::parse_next()
  {
    if (check(TokenKind::KeywordFail))
      return parse_fail_statement();

    if (check(TokenKind::KeywordFails))
      return parse_failure_specification();

    if (
        check(TokenKind::KeywordTry) && !peek().is(TokenKind::LeftBrace))
    {
      return parse_try_expression();
    }

    return parse_cxx_region();
  }

  SyntaxNode Parser::parse_cxx_region()
  {
    const std::size_t begin = position_;

    while (!at_end())
    {
      if (starts_vixc_construct())
        break;

      advance();
    }

    return make_cxx_region(begin, position_);
  }

  SyntaxNode Parser::parse_fail_statement()
  {
    const std::size_t begin = position_;
    const Token fail_token = current();

    advance();

    SyntaxNode node{
        SyntaxKind::FailStatement,
        fail_token.range()};

    const std::size_t payload_begin = position_;

    if (
        check(TokenKind::KeywordTry) && !peek().is(TokenKind::LeftBrace))
    {
      node.add_child(parse_try_expression());
    }
    else
    {
      const std::size_t payload_end =
          scan_opaque_region(
              true,
              false,
              false,
              false,
              true);

      if (payload_end > payload_begin)
      {
        node.add_child(
            make_cxx_region(
                payload_begin,
                payload_end));
      }
    }

    if (position_ == payload_begin)
    {
      report_error(
          "VIXC1001",
          "expected a failure expression after 'fail'",
          fail_token.range());
    }

    bool has_semicolon = false;

    if (match(TokenKind::Semicolon))
      has_semicolon = true;

    if (!has_semicolon)
    {
      SourceRange range = fail_token.range();

      if (position_ > begin + 1)
        range = range_from_tokens(begin, position_);

      report_error(
          "VIXC1002",
          "expected ';' after failure statement",
          range);
    }

    const std::size_t end = position_;

    SourceRange range = fail_token.range();

    if (end > begin)
      range = range_from_tokens(begin, end);

    return SyntaxNode{
        SyntaxKind::FailStatement,
        range,
        node.children()};
  }

  SyntaxNode Parser::parse_failure_specification()
  {
    const std::size_t begin = position_;
    const Token fails_token = current();

    advance();

    const std::size_t type_begin = position_;

    std::size_t paren_depth = 0;
    std::size_t bracket_depth = 0;

    while (!at_end())
    {
      const TokenKind kind = current().kind();

      if (
          paren_depth == 0 && bracket_depth == 0 && (kind == TokenKind::LeftBrace || kind == TokenKind::Semicolon))
      {
        break;
      }

      if (kind == TokenKind::LeftParen)
      {
        ++paren_depth;
        advance();
        continue;
      }

      if (kind == TokenKind::RightParen)
      {
        if (paren_depth == 0)
          break;

        --paren_depth;
        advance();
        continue;
      }

      if (kind == TokenKind::LeftBracket)
      {
        ++bracket_depth;
        advance();
        continue;
      }

      if (kind == TokenKind::RightBracket)
      {
        if (bracket_depth == 0)
          break;

        --bracket_depth;
        advance();
        continue;
      }

      advance();
    }

    const std::size_t type_end = position_;

    SyntaxNode node{
        SyntaxKind::FailureSpecification,
        fails_token.range()};

    if (type_end == type_begin)
    {
      report_error(
          "VIXC1003",
          "expected a failure type after 'fails'",
          fails_token.range());
    }
    else
    {
      node.add_child(
          make_cxx_region(
              type_begin,
              type_end));
    }

    SourceRange range = fails_token.range();

    if (type_end > begin + 1)
      range = range_from_tokens(begin, type_end);

    return SyntaxNode{
        SyntaxKind::FailureSpecification,
        range,
        node.children()};
  }

  SyntaxNode Parser::parse_try_expression()
  {
    const std::size_t begin = position_;
    const Token try_token = current();

    advance();

    const std::size_t operand_begin = position_;

    const std::size_t operand_end =
        scan_opaque_region(
            true,
            true,
            true,
            true,
            true);

    SyntaxNode node{
        SyntaxKind::TryExpression,
        try_token.range()};

    if (operand_end == operand_begin)
    {
      report_error(
          "VIXC1004",
          "expected an expression after 'try'",
          try_token.range());

      return node;
    }

    node.add_child(
        make_cxx_region(
            operand_begin,
            operand_end));

    return SyntaxNode{
        SyntaxKind::TryExpression,
        range_from_tokens(begin, operand_end),
        node.children()};
  }

  bool Parser::starts_vixc_construct() const noexcept
  {
    if (check(TokenKind::KeywordFail))
      return true;

    if (check(TokenKind::KeywordFails))
      return true;

    if (check(TokenKind::KeywordTry))
    {
      /*
       * Native C++ exception syntax begins with `try {`.
       * That sequence remains ordinary C++ rather than becoming a VixC
       * propagation expression.
       */
      return !peek().is(TokenKind::LeftBrace);
    }

    return false;
  }

  std::size_t Parser::scan_opaque_region(
      bool stop_at_semicolon,
      bool stop_at_comma,
      bool stop_at_right_paren,
      bool stop_at_right_bracket,
      bool stop_at_right_brace)
  {
    std::size_t paren_depth = 0;
    std::size_t bracket_depth = 0;
    std::size_t brace_depth = 0;

    while (!at_end())
    {
      const TokenKind kind = current().kind();

      const bool outer =
          paren_depth == 0 && bracket_depth == 0 && brace_depth == 0;

      if (outer)
      {
        if (
            stop_at_semicolon && kind == TokenKind::Semicolon)
        {
          break;
        }

        if (
            stop_at_comma && kind == TokenKind::Comma)
        {
          break;
        }

        if (
            stop_at_right_paren && kind == TokenKind::RightParen)
        {
          break;
        }

        if (
            stop_at_right_bracket && kind == TokenKind::RightBracket)
        {
          break;
        }

        if (
            stop_at_right_brace && kind == TokenKind::RightBrace)
        {
          break;
        }
      }

      switch (kind)
      {
      case TokenKind::LeftParen:
        ++paren_depth;
        advance();
        break;

      case TokenKind::RightParen:
        if (paren_depth == 0)
          return position_;

        --paren_depth;
        advance();
        break;

      case TokenKind::LeftBracket:
        ++bracket_depth;
        advance();
        break;

      case TokenKind::RightBracket:
        if (bracket_depth == 0)
          return position_;

        --bracket_depth;
        advance();
        break;

      case TokenKind::LeftBrace:
        ++brace_depth;
        advance();
        break;

      case TokenKind::RightBrace:
        if (brace_depth == 0)
          return position_;

        --brace_depth;
        advance();
        break;

      default:
        advance();
        break;
      }
    }

    return position_;
  }

  SyntaxNode Parser::make_cxx_region(
      std::size_t begin_index,
      std::size_t end_index) const noexcept
  {
    if (
        begin_index >= end_index || begin_index >= tokens_.size())
    {
      return SyntaxNode{};
    }

    return SyntaxNode{
        SyntaxKind::CxxRegion,
        range_from_tokens(
            begin_index,
            end_index)};
  }

  SourceRange Parser::range_from_tokens(
      std::size_t begin_index,
      std::size_t end_index) const noexcept
  {
    if (
        begin_index >= end_index || begin_index >= tokens_.size() || end_index > tokens_.size())
    {
      return SourceRange{};
    }

    const Token &first = tokens_[begin_index];
    const Token &last = tokens_[end_index - 1];

    const SourceRange first_range = first.range();
    const SourceRange last_range = last.range();

    if (
        !first_range.valid() || !last_range.valid() || first_range.source_id() != last_range.source_id())
    {
      return SourceRange{};
    }

    return SourceRange{
        first_range.begin(),
        last_range.end()};
  }

  void Parser::report_error(
      const char *code,
      const char *message,
      SourceRange range)
  {
    diagnostics_.emit(
        DiagnosticSeverity::Error,
        code,
        message,
        range);
  }

} // namespace vixc::syntax
