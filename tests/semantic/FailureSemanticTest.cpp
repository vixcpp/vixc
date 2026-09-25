/**
 *
 *  @file FailureSemanticTest.cpp
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
#include "../../src/semantic/SemanticAnalyzer.hpp"
#include "../../src/semantic/SemanticContext.hpp"
#include "../../src/semantic/failure/FailureAnalyzer.hpp"
#include "../../src/semantic/failure/OutcomeModel.hpp"
#include "../../src/source/SourceManager.hpp"
#include "../../src/syntax/Lexer.hpp"
#include "../../src/syntax/Parser.hpp"
#include "../../src/syntax/SyntaxKind.hpp"
#include "../../src/syntax/SyntaxNode.hpp"
#include "../../src/syntax/Token.hpp"

#include <vixc/SourceRange.hpp>

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace
{
  using vixc::semantic::FailureContext;
  using vixc::semantic::failure::FailureAnalyzer;
  using vixc::semantic::failure::OutcomeKind;
  using vixc::semantic::failure::OutcomeModel;
  using vixc::syntax::SyntaxKind;
  using vixc::syntax::SyntaxNode;

  SyntaxNode parse(
      vixc::SourceId source_id,
      const std::string &source,
      vixc::diagnostics::DiagnosticEngine &diagnostics)
  {
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

  const SyntaxNode *find_first(
      const SyntaxNode &root,
      SyntaxKind kind)
  {
    if (root.kind() == kind)
      return &root;

    for (const SyntaxNode &child :
         root.children())
    {
      const SyntaxNode *result =
          find_first(
              child,
              kind);

      if (result != nullptr)
        return result;
    }

    return nullptr;
  }

  void test_default_outcome_model_is_success_only()
  {
    const OutcomeModel model;

    assert(model.valid());

    assert(
        model.allows(
            OutcomeKind::Success));

    assert(model.allows_success());

    assert(!model.allows_none());
    assert(!model.allows_failure());
    assert(!model.allows_stopped());

    assert(!model.has_failure_contract());

    assert(
        !model.failure_specification_range()
             .valid());

    assert(
        !model.failure_type_range()
             .valid());
  }

  void test_failure_outcome_model()
  {
    const vixc::SourceRange specification_range{
        0,
        10,
        21};

    const vixc::SourceRange type_range{
        0,
        16,
        21};

    const OutcomeModel model{
        specification_range,
        type_range};

    assert(model.valid());

    assert(model.allows_success());
    assert(model.allows_failure());

    assert(!model.allows_none());
    assert(!model.allows_stopped());

    assert(model.has_failure_contract());

    assert(
        model.failure_specification_range() == specification_range);

    assert(
        model.failure_type_range() == type_range);
  }

  void test_failure_outcome_requires_valid_ranges()
  {
    const vixc::SourceRange specification_range{
        0,
        10,
        21};

    const vixc::SourceRange invalid_type;

    const OutcomeModel model{
        specification_range,
        invalid_type};

    assert(!model.valid());
  }

  void test_failure_outcome_requires_non_empty_type()
  {
    const vixc::SourceRange specification_range{
        0,
        10,
        21};

    const vixc::SourceRange empty_type{
        0,
        16,
        16};

    const OutcomeModel model{
        specification_range,
        empty_type};

    assert(!model.valid());
  }

  void test_failure_outcome_requires_same_source()
  {
    const vixc::SourceRange specification_range{
        0,
        10,
        21};

    const vixc::SourceRange type_range{
        1,
        16,
        21};

    const OutcomeModel model{
        specification_range,
        type_range};

    assert(!model.valid());
  }

  void test_failure_outcome_type_must_be_inside_specification()
  {
    const vixc::SourceRange specification_range{
        0,
        10,
        21};

    const vixc::SourceRange type_range{
        0,
        22,
        27};

    const OutcomeModel model{
        specification_range,
        type_range};

    assert(!model.valid());
  }

  void test_failure_specification_semantics()
  {
    const std::string source =
        "fails Error {";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    const SyntaxNode *specification =
        find_first(
            root,
            SyntaxKind::FailureSpecification);

    assert(specification != nullptr);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    FailureAnalyzer analyzer{
        context};

    assert(
        analyzer.analyze(
            *specification));

    assert(!diagnostics.has_errors());
  }

  void test_fail_outside_failure_context_is_rejected()
  {
    const std::string source =
        "fail error;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    const SyntaxNode *failure =
        find_first(
            root,
            SyntaxKind::FailStatement);

    assert(failure != nullptr);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    FailureAnalyzer analyzer{
        context};

    assert(
        !analyzer.analyze(
            *failure));

    assert(diagnostics.has_errors());
  }

  void test_fail_inside_failure_context_is_accepted()
  {
    const std::string source =
        "Error fail_value; fail fail_value;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    const SyntaxNode *failure =
        find_first(
            root,
            SyntaxKind::FailStatement);

    assert(failure != nullptr);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    const vixc::SourceRange specification_range{
        source_id,
        0,
        5};

    const vixc::SourceRange type_range{
        source_id,
        0,
        5};

    context.push_failure_context(
        FailureContext{
            specification_range,
            type_range});

    assert(context.has_failure_context());
    assert(context.failure_context_depth() == 1);

    FailureAnalyzer analyzer{
        context};

    assert(
        analyzer.analyze(
            *failure));

    assert(!diagnostics.has_errors());

    context.pop_failure_context();

    assert(!context.has_failure_context());
    assert(context.failure_context_depth() == 0);
  }

  void test_try_outside_failure_context_is_rejected()
  {
    const std::string source =
        "try read();";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    const SyntaxNode *try_expression =
        find_first(
            root,
            SyntaxKind::TryExpression);

    assert(try_expression != nullptr);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    FailureAnalyzer analyzer{
        context};

    assert(
        !analyzer.analyze(
            *try_expression));

    assert(diagnostics.has_errors());
  }

  void test_try_inside_failure_context_is_accepted()
  {
    const std::string source =
        "try read();";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    const SyntaxNode *try_expression =
        find_first(
            root,
            SyntaxKind::TryExpression);

    assert(try_expression != nullptr);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    const vixc::SourceRange specification_range{
        source_id,
        0,
        source.size()};

    const vixc::SourceRange type_range{
        source_id,
        0,
        3};

    context.push_failure_context(
        FailureContext{
            specification_range,
            type_range});

    FailureAnalyzer analyzer{
        context};

    assert(
        analyzer.analyze(
            *try_expression));

    assert(!diagnostics.has_errors());

    context.pop_failure_context();
  }

  void test_failure_context_stack()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    static_cast<void>(
        sources.add_source(
            "failure.vix",
            "Error"));

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    assert(!context.has_failure_context());
    assert(context.failure_context_depth() == 0);
    assert(context.current_failure_context() == nullptr);

    const FailureContext first{
        vixc::SourceRange{
            0,
            0,
            5},
        vixc::SourceRange{
            0,
            0,
            5}};

    const FailureContext second{
        vixc::SourceRange{
            0,
            0,
            5},
        vixc::SourceRange{
            0,
            1,
            4}};

    context.push_failure_context(first);

    assert(context.has_failure_context());
    assert(context.failure_context_depth() == 1);

    const FailureContext *current =
        context.current_failure_context();

    assert(current != nullptr);

    assert(
        current->specification_range == first.specification_range);

    assert(
        current->type_range == first.type_range);

    context.push_failure_context(second);

    assert(context.failure_context_depth() == 2);

    current =
        context.current_failure_context();

    assert(current != nullptr);

    assert(
        current->specification_range == second.specification_range);

    assert(
        current->type_range == second.type_range);

    context.pop_failure_context();

    assert(context.failure_context_depth() == 1);

    current =
        context.current_failure_context();

    assert(current != nullptr);

    assert(
        current->type_range == first.type_range);

    context.clear_failure_contexts();

    assert(!context.has_failure_context());
    assert(context.failure_context_depth() == 0);
    assert(context.current_failure_context() == nullptr);
  }

  void test_failure_context_pop_on_empty_stack_is_safe()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    assert(!context.has_failure_context());

    context.pop_failure_context();

    assert(!context.has_failure_context());
    assert(context.failure_context_depth() == 0);
  }

  void test_semantic_context_recovers_source_text()
  {
    const std::string source =
        "fails Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    const vixc::SourceRange range{
        source_id,
        6,
        11};

    const auto text =
        context.source_text(range);

    assert(text.has_value());
    assert(*text == "Error");
  }

  void test_semantic_context_rejects_invalid_source_range()
  {
    const std::string source =
        "Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    const vixc::SourceRange outside{
        source_id,
        0,
        100};

    assert(
        !context.source_text(
                    outside)
             .has_value());
  }

  void test_failure_analyzer_rejects_unrelated_syntax()
  {
    const std::string source =
        "int value = 42;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "plain.cpp",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());
    assert(root.child_count() == 1);

    const SyntaxNode *region =
        root.child(0);

    assert(region != nullptr);

    assert(
        region->kind() == SyntaxKind::CxxRegion);

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    FailureAnalyzer analyzer{
        context};

    assert(
        !analyzer.analyze(
            *region));

    assert(diagnostics.has_errors());
  }

  void test_semantic_analyzer_accepts_plain_cpp()
  {
    const std::string source =
        "int main() { return 0; }";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    vixc::semantic::SemanticAnalyzer analyzer{
        context};

    assert(
        analyzer.analyze(
            root));

    assert(!diagnostics.has_errors());
  }

  void test_semantic_analyzer_rejects_fail_without_context()
  {
    const std::string source =
        "fail error;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const SyntaxNode root =
        parse(
            source_id,
            source,
            diagnostics);

    assert(!diagnostics.has_errors());

    vixc::semantic::SemanticContext context{
        sources,
        diagnostics};

    vixc::semantic::SemanticAnalyzer analyzer{
        context};

    assert(
        !analyzer.analyze(
            root));

    assert(diagnostics.has_errors());
  }

} // namespace

int main()
{
  test_default_outcome_model_is_success_only();

  test_failure_outcome_model();
  test_failure_outcome_requires_valid_ranges();
  test_failure_outcome_requires_non_empty_type();
  test_failure_outcome_requires_same_source();
  test_failure_outcome_type_must_be_inside_specification();

  test_failure_specification_semantics();

  test_fail_outside_failure_context_is_rejected();
  test_fail_inside_failure_context_is_accepted();

  test_try_outside_failure_context_is_rejected();
  test_try_inside_failure_context_is_accepted();

  test_failure_context_stack();
  test_failure_context_pop_on_empty_stack_is_safe();

  test_semantic_context_recovers_source_text();
  test_semantic_context_rejects_invalid_source_range();

  test_failure_analyzer_rejects_unrelated_syntax();

  test_semantic_analyzer_accepts_plain_cpp();
  test_semantic_analyzer_rejects_fail_without_context();

  return 0;
}
