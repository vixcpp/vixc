/**
 *
 *  @file ParserTest.cpp
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

#include "../../src/diagnostics/DiagnosticEngine.hpp"
#include "../../src/syntax/Lexer.hpp"
#include "../../src/syntax/Parser.hpp"
#include "../../src/syntax/SyntaxKind.hpp"
#include "../../src/syntax/SyntaxNode.hpp"
#include "../../src/syntax/Token.hpp"

#include <vixc/SourceLocation.hpp>

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  vixc::syntax::SyntaxNode parse(
      const std::string &source,
      vixc::diagnostics::DiagnosticEngine &diagnostics)
  {
    constexpr vixc::SourceId source_id = 0;

    vixc::syntax::Lexer lexer{
        source_id,
        source,
        diagnostics};

    std::vector<vixc::syntax::Token> tokens =
        lexer.lex_all();

    vixc::syntax::Parser parser{
        std::move(tokens),
        diagnostics};

    return parser.parse();
  }

  std::string_view source_text(
      const std::string &source,
      vixc::SourceRange range)
  {
    assert(range.valid());
    assert(range.end_offset() <= source.size());

    return std::string_view{source}.substr(
        range.begin_offset(),
        range.size());
  }

  void test_empty_translation_unit()
  {
    const std::string source;

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(
        root.kind() == vixc::syntax::SyntaxKind::TranslationUnit);

    assert(root.child_count() == 0);
  }

  void test_plain_cpp_becomes_cxx_region()
  {
    const std::string source =
        "int main() { return 0; }";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(
        root.kind() == vixc::syntax::SyntaxKind::TranslationUnit);

    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *region =
        root.child(0);

    assert(region != nullptr);

    assert(
        region->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        source_text(
            source,
            region->range()) == source);
  }

  void test_multiple_plain_cpp_tokens_remain_one_region()
  {
    const std::string source =
        "int value = 42; value += 1;";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *region =
        root.child(0);

    assert(region != nullptr);

    assert(
        region->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        source_text(
            source,
            region->range()) == source);
  }

  void test_fail_statement()
  {
    const std::string source =
        "fail error;";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *failure =
        root.child(0);

    assert(failure != nullptr);

    assert(
        failure->kind() == vixc::syntax::SyntaxKind::FailStatement);

    assert(failure->child_count() == 1);

    assert(
        source_text(
            source,
            failure->range()) == "fail error;");

    const vixc::syntax::SyntaxNode *operand =
        failure->child(0);

    assert(operand != nullptr);

    assert(
        source_text(
            source,
            operand->range()) == "error");
  }

  void test_fail_statement_with_expression()
  {
    const std::string source =
        "fail make_error(code);";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *failure =
        root.child(0);

    assert(failure != nullptr);

    assert(
        failure->kind() == vixc::syntax::SyntaxKind::FailStatement);

    assert(failure->child_count() == 1);

    const vixc::syntax::SyntaxNode *operand =
        failure->child(0);

    assert(operand != nullptr);

    assert(
        source_text(
            source,
            operand->range()) == "make_error(code)");

    assert(
        source_text(
            source,
            failure->range()) == "fail make_error(code);");
  }

  void test_failure_specification()
  {
    const std::string source =
        "fails Error {";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(root.child_count() >= 1);

    const vixc::syntax::SyntaxNode *specification =
        root.child(0);

    assert(specification != nullptr);

    assert(
        specification->kind() == vixc::syntax::SyntaxKind::FailureSpecification);

    assert(specification->child_count() == 1);

    const vixc::syntax::SyntaxNode *type =
        specification->child(0);

    assert(type != nullptr);

    assert(
        source_text(
            source,
            type->range()) == "Error");

    assert(
        source_text(
            source,
            specification->range()) == "fails Error");
  }

  void test_failure_specification_with_qualified_type()
  {
    const std::string source =
        "fails errors::NetworkError {";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() >= 1);

    const vixc::syntax::SyntaxNode *specification =
        root.child(0);

    assert(specification != nullptr);

    assert(
        specification->kind() == vixc::syntax::SyntaxKind::FailureSpecification);

    assert(specification->child_count() == 1);

    const vixc::syntax::SyntaxNode *type =
        specification->child(0);

    assert(type != nullptr);

    assert(
        source_text(
            source,
            type->range()) == "errors::NetworkError");
  }

  void test_failure_specification_with_template_type()
  {
    const std::string source =
        "fails Result<Error> {";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() >= 1);

    const vixc::syntax::SyntaxNode *specification =
        root.child(0);

    assert(specification != nullptr);
    assert(specification->child_count() == 1);

    const vixc::syntax::SyntaxNode *type =
        specification->child(0);

    assert(type != nullptr);

    assert(
        source_text(
            source,
            type->range()) == "Result<Error>");
  }

  void test_try_expression()
  {
    const std::string source =
        "try read();";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 2);

    const vixc::syntax::SyntaxNode *try_expression =
        root.child(0);

    assert(try_expression != nullptr);

    assert(
        try_expression->kind() == vixc::syntax::SyntaxKind::TryExpression);

    assert(try_expression->child_count() == 1);

    const vixc::syntax::SyntaxNode *operand =
        try_expression->child(0);

    assert(operand != nullptr);

    assert(
        source_text(
            source,
            operand->range()) == "read()");

    assert(
        source_text(
            source,
            try_expression->range()) == "try read()");

    const vixc::syntax::SyntaxNode *suffix =
        root.child(1);

    assert(suffix != nullptr);
    assert(suffix->kind() == vixc::syntax::SyntaxKind::CxxRegion);
    assert(source_text(source, suffix->range()) == ";");
  }

  void test_try_expression_inside_cpp_region()
  {
    const std::string source =
        "auto value = try read();";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(root.child_count() == 3);

    const vixc::syntax::SyntaxNode *prefix =
        root.child(0);

    const vixc::syntax::SyntaxNode *try_expression =
        root.child(1);

    const vixc::syntax::SyntaxNode *suffix =
        root.child(2);

    assert(prefix != nullptr);
    assert(try_expression != nullptr);
    assert(suffix != nullptr);

    assert(
        prefix->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        try_expression->kind() == vixc::syntax::SyntaxKind::TryExpression);

    assert(
        suffix->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        source_text(
            source,
            prefix->range()) == "auto value = ");

    assert(
        source_text(
            source,
            try_expression->range()) == "try read()");

    assert(
        source_text(
            source,
            suffix->range()) == ";");
  }

  void test_native_cpp_try_block_is_not_vixc_try_expression()
  {
    const std::string source =
        "try { work(); }";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *region =
        root.child(0);

    assert(region != nullptr);

    assert(
        region->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        source_text(
            source,
            region->range()) == source);
  }

  void test_complete_failure_example_is_structurally_parsed()
  {
    const std::string source =
        "Result load() fails Error {\n"
        "  auto value = try read();\n"
        "  fail error;\n"
        "}";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(
        root.kind() == vixc::syntax::SyntaxKind::TranslationUnit);

    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *function =
        root.child(0);

    assert(function != nullptr);

    assert(
        function->kind() ==
        vixc::syntax::SyntaxKind::FunctionDeclaration);

    bool found_specification = false;
    bool found_try_initialization = false;
    bool found_fail = false;

    for (const vixc::syntax::SyntaxNode &child :
         function->children())
    {
      switch (child.kind())
      {
      case vixc::syntax::SyntaxKind::FailureSpecification:
        found_specification = true;
        break;

      case vixc::syntax::SyntaxKind::TryInitialization:
      {
        assert(child.child_count() == 2);

        const vixc::syntax::SyntaxNode *declaration =
            child.child(0);
        const vixc::syntax::SyntaxNode *propagation =
            child.child(1);

        assert(declaration != nullptr);
        assert(propagation != nullptr);
        assert(
            declaration->kind() ==
            vixc::syntax::SyntaxKind::CxxRegion);
        assert(
            source_text(
                source,
                declaration->range()) == "auto value");
        assert(
            propagation->kind() ==
            vixc::syntax::SyntaxKind::TryExpression);
        assert(propagation->child_count() == 1);

        const vixc::syntax::SyntaxNode *operand =
            propagation->child(0);
        assert(operand != nullptr);
        assert(
            source_text(
                source,
                operand->range()) == "read()");

        found_try_initialization = true;
        break;
      }

      case vixc::syntax::SyntaxKind::FailStatement:
        found_fail = true;
        break;

      default:
        break;
      }
    }

    assert(found_specification);
    assert(found_try_initialization);
    assert(found_fail);
  }

  void test_cpp_before_and_after_fail_statement()
  {
    const std::string source =
        "before(); fail error; after();";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    assert(root.child_count() == 3);

    const vixc::syntax::SyntaxNode *before =
        root.child(0);

    const vixc::syntax::SyntaxNode *failure =
        root.child(1);

    const vixc::syntax::SyntaxNode *after =
        root.child(2);

    assert(before != nullptr);
    assert(failure != nullptr);
    assert(after != nullptr);

    assert(
        before->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        failure->kind() == vixc::syntax::SyntaxKind::FailStatement);

    assert(
        after->kind() == vixc::syntax::SyntaxKind::CxxRegion);

    assert(
        source_text(
            source,
            before->range()) == "before(); ");

    assert(
        source_text(
            source,
            failure->range()) == "fail error;");

    assert(
        source_text(
            source,
            after->range()) == " after();");
  }

  void test_fail_requires_operand()
  {
    const std::string source =
        "fail;";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    (void)root;

    assert(diagnostics.has_errors());
    assert(!diagnostics.empty());
  }

  void test_fail_requires_semicolon()
  {
    const std::string source =
        "fail error";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    (void)root;

    assert(diagnostics.has_errors());
    assert(!diagnostics.empty());
  }

  void test_fails_requires_type()
  {
    const std::string source =
        "fails {";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    (void)root;

    assert(diagnostics.has_errors());
    assert(!diagnostics.empty());
  }

  void test_try_requires_operand()
  {
    const std::string source =
        "try;";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    (void)root;

    assert(diagnostics.has_errors());
    assert(!diagnostics.empty());
  }

  void test_ranges_use_original_source_offsets()
  {
    const std::string source =
        "  fail error;\n";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    const vixc::syntax::SyntaxNode root =
        parse(
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() >= 1);

    const vixc::syntax::SyntaxNode *failure =
        nullptr;

    for (const vixc::syntax::SyntaxNode &child :
         root.children())
    {
      if (
          child.kind() == vixc::syntax::SyntaxKind::FailStatement)
      {
        failure = &child;
        break;
      }
    }

    assert(failure != nullptr);

    assert(failure->range().source_id() == 0);

    assert(
        source_text(
            source,
            failure->range()) == "fail error;");
  }

  void test_parser_preserves_source_identity()
  {
    const std::string source =
        "fail error;";

    vixc::diagnostics::DiagnosticEngine diagnostics;

    constexpr vixc::SourceId source_id = 27;

    vixc::syntax::Lexer lexer{
        source_id,
        source,
        diagnostics};

    std::vector<vixc::syntax::Token> tokens =
        lexer.lex_all();

    vixc::syntax::Parser parser{
        std::move(tokens),
        diagnostics};

    const vixc::syntax::SyntaxNode root =
        parser.parse();

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *failure =
        root.child(0);

    assert(failure != nullptr);

    assert(
        failure->range().source_id() == source_id);

    assert(failure->child_count() == 1);

    const vixc::syntax::SyntaxNode *operand =
        failure->child(0);

    assert(operand != nullptr);

    assert(
        operand->range().source_id() == source_id);
  }

  void test_failure_aware_function_retains_return_structure()
  {
    const std::string source =
        "int divide(int a, int b) fails MathError\n"
        "{\n"
        "  return a / b;\n"
        "}\n";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *function = root.child(0);
    assert(function != nullptr);
    assert(function->kind() == vixc::syntax::SyntaxKind::FunctionDeclaration);

    const vixc::syntax::SyntaxNode *return_type = nullptr;
    const vixc::syntax::SyntaxNode *function_name = nullptr;
    const vixc::syntax::SyntaxNode *declarator = nullptr;
    const vixc::syntax::SyntaxNode *specification = nullptr;
    const vixc::syntax::SyntaxNode *statement = nullptr;

    for (const vixc::syntax::SyntaxNode &child : function->children())
    {
      if (child.kind() == vixc::syntax::SyntaxKind::FunctionReturnType)
        return_type = &child;
      else if (child.kind() == vixc::syntax::SyntaxKind::FunctionName)
        function_name = &child;
      else if (child.kind() == vixc::syntax::SyntaxKind::FunctionDeclarator)
        declarator = &child;
      else if (child.kind() == vixc::syntax::SyntaxKind::FailureSpecification)
        specification = &child;
      else if (child.kind() == vixc::syntax::SyntaxKind::ReturnStatement)
        statement = &child;
    }

    assert(return_type != nullptr);
    assert(function_name != nullptr);
    assert(declarator != nullptr);
    assert(specification != nullptr);
    assert(statement != nullptr);

    assert(source_text(source, return_type->range()) == "int");
    assert(source_text(source, function_name->range()) == "divide");
    assert(source_text(source, declarator->range()) == "divide(int a, int b)");
    assert(source_text(source, statement->range()) == "return a / b;");

    assert(statement->child_count() == 1);
    const vixc::syntax::SyntaxNode *operand = statement->child(0);
    assert(operand != nullptr);
    assert(source_text(source, operand->range()) == "a / b");
  }

  void test_failure_aware_function_bodies_remain_separate()
  {
    const std::string source =
        "int first() fails int { return 1; }\n"
        "int second() fails int { return 2; }\n";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 2);

    for (std::size_t index = 0; index < root.child_count(); ++index)
    {
      const vixc::syntax::SyntaxNode *function = root.child(index);
      assert(function != nullptr);
      assert(function->kind() == vixc::syntax::SyntaxKind::FunctionDeclaration);

      std::size_t return_count = 0;
      for (const vixc::syntax::SyntaxNode &child : function->children())
      {
        if (child.kind() == vixc::syntax::SyntaxKind::ReturnStatement)
          ++return_count;
      }

      assert(return_count == 1);
    }
  }

  void test_failure_aware_function_retains_multi_token_success_type()
  {
    const std::string source =
        "std::string name() fails Error { return \"x\"; }\n";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const vixc::syntax::SyntaxNode *function = root.child(0);
    assert(function != nullptr);

    const vixc::syntax::SyntaxNode *return_type = nullptr;
    const vixc::syntax::SyntaxNode *declarator = nullptr;
    for (const vixc::syntax::SyntaxNode &child : function->children())
    {
      if (child.kind() == vixc::syntax::SyntaxKind::FunctionReturnType)
        return_type = &child;
      else if (child.kind() == vixc::syntax::SyntaxKind::FunctionDeclarator)
        declarator = &child;
    }

    assert(return_type != nullptr);
    assert(declarator != nullptr);
    assert(source_text(source, return_type->range()) == "std::string");
    assert(source_text(source, declarator->range()) == "name()");
  }

  void test_try_initialization_retains_direct_call()
  {
    const std::string source =
        "int wrapper() fails Error { auto value = try source(1, 2); return value; }";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());

    const vixc::syntax::SyntaxNode *initialization = nullptr;
    for (const vixc::syntax::SyntaxNode &child : root.child(0)->children())
    {
      if (child.kind() == vixc::syntax::SyntaxKind::TryInitialization)
        initialization = &child;
    }

    assert(initialization != nullptr);
    assert(initialization->child_count() == 2);
    const vixc::syntax::SyntaxNode *try_expression = initialization->child(1);
    assert(try_expression != nullptr);
    assert(try_expression->child_count() == 1);
    const vixc::syntax::SyntaxNode *call = try_expression->child(0);
    assert(call != nullptr);
    assert(call->kind() == vixc::syntax::SyntaxKind::DirectCallExpression);
    assert(source_text(source, call->range()) == "source(1, 2)");
    assert(call->child_count() == 1);
    assert(source_text(source, call->child(0)->range()) == "source");
  }

  void test_non_direct_try_operand_is_not_a_direct_call()
  {
    const std::string source =
        "int wrapper() fails Error { auto value = try ns::source(); return value; }";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());

    const vixc::syntax::SyntaxNode *try_expression = nullptr;
    for (const vixc::syntax::SyntaxNode &child : root.child(0)->children())
    {
      if (child.kind() == vixc::syntax::SyntaxKind::TryInitialization)
        try_expression = child.child(1);
    }

    assert(try_expression != nullptr);
    assert(try_expression->child_count() == 1);
    assert(
        try_expression->child(0)->kind() ==
        vixc::syntax::SyntaxKind::CxxRegion);
  }

  void test_failure_aware_functions_following_cpp_preserve_signatures()
  {
    const std::string source =
        "#include <iostream>\n"
        "#include <utility>\n"
        "\n"
        "enum class MathError\n"
        "{\n"
        "    DivisionByZero\n"
        "};\n"
        "\n"
        "int divide(int a, int b) fails MathError\n"
        "{\n"
        "    if (b == 0)\n"
        "    {\n"
        "        fail MathError::DivisionByZero;\n"
        "    }\n"
        "\n"
        "    return a / b;\n"
        "}\n"
        "\n"
        "int calculate() fails MathError\n"
        "{\n"
        "    auto value = try divide(10, 2);\n"
        "    return value;\n"
        "}\n"
        "\n"
        "int main()\n"
        "{\n"
        "    return 0;\n"
        "}\n";

    vixc::diagnostics::DiagnosticEngine diagnostics;
    const vixc::syntax::SyntaxNode root = parse(source, diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 4);

    const vixc::syntax::SyntaxNode *prelude = root.child(0);
    const vixc::syntax::SyntaxNode *divide = root.child(1);
    const vixc::syntax::SyntaxNode *calculate = root.child(2);
    const vixc::syntax::SyntaxNode *main = root.child(3);

    assert(prelude != nullptr);
    assert(divide != nullptr);
    assert(calculate != nullptr);
    assert(main != nullptr);
    assert(prelude->kind() == vixc::syntax::SyntaxKind::CxxRegion);
    assert(divide->kind() == vixc::syntax::SyntaxKind::FunctionDeclaration);
    assert(calculate->kind() == vixc::syntax::SyntaxKind::FunctionDeclaration);
    assert(main->kind() == vixc::syntax::SyntaxKind::CxxRegion);
    assert(source_text(source, prelude->range()).find("MathError") != std::string_view::npos);
    assert(source_text(source, main->range()).find("int main") != std::string_view::npos);

    const auto assert_signature =
        [&source](
            const vixc::syntax::SyntaxNode &function,
            std::string_view declarator_text)
        {
          const vixc::syntax::SyntaxNode *return_type = nullptr;
          const vixc::syntax::SyntaxNode *declarator = nullptr;
          const vixc::syntax::SyntaxNode *specification = nullptr;

          for (const vixc::syntax::SyntaxNode &child : function.children())
          {
            if (child.kind() == vixc::syntax::SyntaxKind::FunctionReturnType)
              return_type = &child;
            else if (child.kind() == vixc::syntax::SyntaxKind::FunctionDeclarator)
              declarator = &child;
            else if (child.kind() == vixc::syntax::SyntaxKind::FailureSpecification)
              specification = &child;
          }

          assert(return_type != nullptr);
          assert(declarator != nullptr);
          assert(specification != nullptr);
          assert(source_text(source, return_type->range()) == "int");
          assert(source_text(source, declarator->range()) == declarator_text);
          assert(specification->child_count() == 1);

          const vixc::syntax::SyntaxNode *failure_type =
              specification->child(0);
          assert(failure_type != nullptr);
          assert(source_text(source, failure_type->range()) == "MathError");
        };

    assert_signature(*divide, "divide(int a, int b)");
    assert_signature(*calculate, "calculate()");

    bool divide_has_failure = false;
    bool divide_has_return = false;
    for (const vixc::syntax::SyntaxNode &child : divide->children())
    {
      divide_has_failure =
          divide_has_failure ||
          child.kind() == vixc::syntax::SyntaxKind::FailStatement;
      divide_has_return =
          divide_has_return ||
          child.kind() == vixc::syntax::SyntaxKind::ReturnStatement;
    }

    assert(divide_has_failure);
    assert(divide_has_return);

    bool calculate_has_try_initialization = false;
    bool calculate_has_return = false;
    for (const vixc::syntax::SyntaxNode &child : calculate->children())
    {
      if (child.kind() == vixc::syntax::SyntaxKind::TryInitialization)
      {
        assert(child.child_count() == 2);
        const vixc::syntax::SyntaxNode *propagation = child.child(1);
        assert(propagation != nullptr);
        assert(propagation->kind() == vixc::syntax::SyntaxKind::TryExpression);
        calculate_has_try_initialization = true;
      }

      calculate_has_return =
          calculate_has_return ||
          child.kind() == vixc::syntax::SyntaxKind::ReturnStatement;
    }

    assert(calculate_has_try_initialization);
    assert(calculate_has_return);
  }

} // namespace

int main()
{
  test_empty_translation_unit();

  test_plain_cpp_becomes_cxx_region();
  test_multiple_plain_cpp_tokens_remain_one_region();

  test_fail_statement();
  test_fail_statement_with_expression();

  test_failure_specification();
  test_failure_specification_with_qualified_type();
  test_failure_specification_with_template_type();

  test_try_expression();
  test_try_expression_inside_cpp_region();
  test_native_cpp_try_block_is_not_vixc_try_expression();

  test_complete_failure_example_is_structurally_parsed();
  test_cpp_before_and_after_fail_statement();

  test_fail_requires_operand();
  test_fail_requires_semicolon();
  test_fails_requires_type();
  test_try_requires_operand();

  test_ranges_use_original_source_offsets();
  test_parser_preserves_source_identity();
  test_failure_aware_function_retains_return_structure();
  test_failure_aware_function_bodies_remain_separate();
  test_failure_aware_function_retains_multi_token_success_type();
  test_try_initialization_retains_direct_call();
  test_non_direct_try_operand_is_not_a_direct_call();
  test_failure_aware_functions_following_cpp_preserve_signatures();

  return 0;
}
