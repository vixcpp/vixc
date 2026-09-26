/**
 *
 *  @file FailureAnalyzer.cpp
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

#include "FailureAnalyzer.hpp"

#include "../../diagnostics/DiagnosticEngine.hpp"
#include "../../syntax/SyntaxKind.hpp"
#include "../../syntax/SyntaxNode.hpp"

#include <vixc/DiagnosticSeverity.hpp>

#include <utility>

namespace vixc::semantic::failure
{
  FailureAnalyzer::FailureAnalyzer(
      SemanticContext &context) noexcept
      : context_(context)
  {
  }

  bool FailureAnalyzer::analyze(
      const syntax::SyntaxNode &node)
  {
    switch (node.kind())
    {
    case syntax::SyntaxKind::FailureSpecification:
      return analyze_failure_specification(node);

    case syntax::SyntaxKind::FailStatement:
      return analyze_fail_statement(node);

    case syntax::SyntaxKind::TryExpression:
      return analyze_try_expression(node);

    default:
      return report_error(
          "VIXC2001",
          "expected a failure-related syntax node",
          node.range());
    }
  }

  bool FailureAnalyzer::analyze_failure_specification(
      const syntax::SyntaxNode &node)
  {
    const std::optional<OutcomeModel> model =
        build_outcome_model(node);

    if (!model.has_value())
    {
      return report_error(
          "VIXC2002",
          "invalid failure specification",
          node.range());
    }

    if (!model->valid())
    {
      return report_error(
          "VIXC2003",
          "failure specification does not form a valid outcome contract",
          node.range());
    }

    const std::optional<std::string_view> failure_type =
        context_.source_text(
            model->failure_type_range());

    if (!failure_type.has_value())
    {
      return report_error(
          "VIXC2004",
          "unable to read the declared failure type",
          model->failure_type_range());
    }

    if (failure_type->empty())
    {
      return report_error(
          "VIXC2005",
          "failure type cannot be empty",
          model->failure_type_range());
    }

    return true;
  }

  bool FailureAnalyzer::analyze_fail_statement(
      const syntax::SyntaxNode &node)
  {
    if (!context_.has_failure_context())
    {
      return report_error(
          "VIXC2006",
          "'fail' can only be used inside a failure-aware computation",
          node.range(),
          "declare the containing function with `fails <ErrorType>`");
    }

    if (node.child_count() != 1)
    {
      return report_error(
          "VIXC2007",
          "'fail' requires exactly one failure expression",
          node.range());
    }

    const syntax::SyntaxNode *expression =
        node.child(0);

    if (expression == nullptr)
    {
      return report_error(
          "VIXC2008",
          "missing failure expression",
          node.range());
    }

    if (!expression->range().valid())
    {
      return report_error(
          "VIXC2009",
          "failure expression has no valid source range",
          node.range());
    }

    const std::optional<std::string_view> expression_text =
        context_.source_text(
            expression->range());

    if (!expression_text.has_value())
    {
      return report_error(
          "VIXC2010",
          "unable to read the failure expression",
          expression->range());
    }

    if (expression_text->empty())
    {
      return report_error(
          "VIXC2011",
          "failure expression cannot be empty",
          expression->range());
    }

    const FailureContext *failure_context =
        context_.current_failure_context();

    if (failure_context == nullptr)
    {
      return report_error(
          "VIXC2012",
          "missing active failure contract",
          node.range());
    }

    if (
        !failure_context->specification_range.valid() || !failure_context->type_range.valid())
    {
      return report_error(
          "VIXC2013",
          "active failure contract is invalid",
          node.range());
    }

    return true;
  }

  bool FailureAnalyzer::analyze_try_expression(
      const syntax::SyntaxNode &node)
  {
    if (!context_.has_failure_context())
    {
      return report_error(
          "VIXC2014",
          "'try' can only propagate failure inside a failure-aware computation",
          node.range(),
          "declare the containing function with `fails <ErrorType>`");
    }

    if (node.child_count() != 1)
    {
      return report_error(
          "VIXC2015",
          "'try' requires exactly one operand",
          node.range());
    }

    const syntax::SyntaxNode *operand =
        node.child(0);

    if (operand == nullptr)
    {
      return report_error(
          "VIXC2016",
          "missing operand for 'try'",
          node.range());
    }

    if (!operand->range().valid())
    {
      return report_error(
          "VIXC2017",
          "'try' operand has no valid source range",
          node.range());
    }

    const std::optional<std::string_view> operand_text =
        context_.source_text(
            operand->range());

    if (!operand_text.has_value())
    {
      return report_error(
          "VIXC2018",
          "unable to read the operand of 'try'",
          operand->range());
    }

    if (operand_text->empty())
    {
      return report_error(
          "VIXC2019",
          "'try' operand cannot be empty",
          operand->range());
    }

    const FailureContext *failure_context =
        context_.current_failure_context();

    if (failure_context == nullptr)
    {
      return report_error(
          "VIXC2020",
          "missing failure contract for propagation",
          node.range());
    }

    if (
        !failure_context->specification_range.valid() || !failure_context->type_range.valid())
    {
      return report_error(
          "VIXC2021",
          "active failure contract is invalid",
          node.range());
    }

    return true;
  }

  std::optional<OutcomeModel>
  FailureAnalyzer::build_outcome_model(
      const syntax::SyntaxNode &node) const
  {
    if (
        !node.is(
            syntax::SyntaxKind::FailureSpecification))
    {
      return std::nullopt;
    }

    if (!node.range().valid())
      return std::nullopt;

    if (node.child_count() != 1)
      return std::nullopt;

    const syntax::SyntaxNode *type_node =
        node.child(0);

    if (type_node == nullptr)
      return std::nullopt;

    if (!type_node->range().valid())
      return std::nullopt;

    OutcomeModel model{
        node.range(),
        type_node->range()};

    if (!model.valid())
      return std::nullopt;

    return model;
  }

  bool FailureAnalyzer::report_error(
      const char *code,
      const char *message,
      SourceRange range,
      std::string hint)
  {
    context_.diagnostics().emit(
        DiagnosticSeverity::Error,
        code,
        message,
        range,
        std::move(hint));

    return false;
  }

} // namespace vixc::semantic::failure
