/**
 *
 *  @file LexerTest.cpp
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

#include "../../src/diagnostics/DiagnosticEngine.hpp"
#include "../../src/syntax/Lexer.hpp"
#include "../../src/syntax/TokenKind.hpp"

#include <vixc/SourceLocation.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace
{
  void test_keywords_punctuation_and_trivia()
  {
    const std::string source =
        " // ignored\nfail value;";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    vixc::syntax::Lexer lexer{
        vixc::SourceId{0},
        source,
        diagnostics};

    const std::vector<vixc::syntax::Token> tokens = lexer.lex_all();

    assert(!diagnostics.has_errors());
    assert(tokens.size() == 4);
    assert(tokens[0].kind() == vixc::syntax::TokenKind::KeywordFail);
    assert(tokens[0].text() == "fail");
    assert(tokens[0].range().begin_offset() == 12);
    assert(tokens[1].kind() == vixc::syntax::TokenKind::Identifier);
    assert(tokens[1].text() == "value");
    assert(tokens[2].kind() == vixc::syntax::TokenKind::Semicolon);
    assert(tokens[3].is_end_of_file());
    assert(tokens[3].range().begin_offset() == source.size());
  }
} // namespace

int main()
{
  test_keywords_punctuation_and_trivia();
  return 0;
}
