/**
 *
 *  @file FailureLowering.cpp
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

#include "FailureLowering.hpp"

#include "../../diagnostics/DiagnosticEngine.hpp"
#include "../../ir/IrKind.hpp"
#include "../../ir/IrNode.hpp"
#include "../../ir/failure/Failure.hpp"
#include "../../ir/failure/Outcome.hpp"

#include <vixc/DiagnosticSeverity.hpp>

namespace vixc::lowering::failure
{

  FailureLowering::FailureLowering(
      LoweringContext &context) noexcept
      : context_(context)
  {
  }

  bool FailureLowering::lower(
      ir::IrNode &node)
  {
    if (context_.has_fatal())
      return false;

    switch (node.kind())
    {
    case ir::IrKind::Outcome:
    {
      auto *outcome =
          dynamic_cast<ir::failure::Outcome *>(&node);

      if (outcome == nullptr)
      {
        return report_error(
            "VIXC3101",
            "IR node marked as Outcome has an incompatible concrete type",
            node.range());
      }

      return lower_outcome(*outcome);
    }

    case ir::IrKind::Failure:
    {
      auto *failure =
          dynamic_cast<ir::failure::Failure *>(&node);

      if (failure == nullptr)
      {
        return report_error(
            "VIXC3102",
            "IR node marked as Failure has an incompatible concrete type",
            node.range());
      }

      return lower_failure(*failure);
    }

    case ir::IrKind::FailurePropagation:
    {
      auto *propagation =
          dynamic_cast<ir::failure::FailurePropagation *>(&node);

      if (propagation == nullptr)
      {
        return report_error(
            "VIXC3103",
            "IR node marked as FailurePropagation has an incompatible concrete type",
            node.range());
      }

      return lower_failure_propagation(
          *propagation);
    }

    default:
      return report_error(
          "VIXC3104",
          "failure lowering received a non-failure IR node",
          node.range());
    }
  }

  bool FailureLowering::lower_outcome(
      ir::failure::Outcome &outcome)
  {
    if (!outcome.valid())
    {
      return report_error(
          "VIXC3105",
          "invalid Outcome IR reached failure lowering",
          outcome.range());
    }

    if (!outcome.allows_success())
    {
      return report_error(
          "VIXC3106",
          "Outcome IR must permit successful completion",
          outcome.range());
    }

    if (outcome.allows_failure())
    {
      const SourceRange failure_type =
          outcome.failure_type_range();

      if (!failure_type.valid())
      {
        return report_error(
            "VIXC3107",
            "failure-aware Outcome has no valid failure type",
            outcome.range());
      }

      if (failure_type.empty())
      {
        return report_error(
            "VIXC3108",
            "failure-aware Outcome has an empty failure type",
            failure_type);
      }

      if (!context_.source_text(failure_type).has_value())
      {
        return report_error(
            "VIXC3109",
            "unable to recover the failure type from source",
            failure_type);
      }
    }

    return true;
  }

  bool FailureLowering::lower_failure(
      ir::failure::Failure &failure)
  {
    if (!failure.valid())
    {
      return report_error(
          "VIXC3110",
          "invalid Failure IR reached failure lowering",
          failure.range());
    }

    const SourceRange failure_type =
        failure.failure_type_range();

    if (!context_.source_text(failure_type).has_value())
    {
      return report_error(
          "VIXC3111",
          "unable to recover the declared failure type",
          failure_type);
    }

    ir::IrNode *operand =
        failure.operand();

    if (operand == nullptr)
    {
      return report_error(
          "VIXC3112",
          "Failure IR has no failure value operand",
          failure.range());
    }

    return lower_operand(*operand);
  }

  bool FailureLowering::lower_failure_propagation(
      ir::failure::FailurePropagation &propagation)
  {
    if (!propagation.valid())
    {
      return report_error(
          "VIXC3113",
          "invalid FailurePropagation IR reached failure lowering",
          propagation.range());
    }

    const SourceRange failure_type =
        propagation.failure_type_range();

    if (!context_.source_text(failure_type).has_value())
    {
      return report_error(
          "VIXC3114",
          "unable to recover the enclosing failure type",
          failure_type);
    }

    ir::IrNode *operand =
        propagation.operand();

    if (operand == nullptr)
    {
      return report_error(
          "VIXC3115",
          "FailurePropagation IR has no operand",
          propagation.range());
    }

    return lower_operand(*operand);
  }

  bool FailureLowering::lower_operand(
      ir::IrNode &operand)
  {
    if (context_.has_fatal())
      return false;

    switch (operand.kind())
    {
    case ir::IrKind::Invalid:
      return report_error(
          "VIXC3116",
          "invalid operand reached failure lowering",
          operand.range());

    case ir::IrKind::CxxRegion:
      return true;

    case ir::IrKind::Outcome:
    case ir::IrKind::Failure:
    case ir::IrKind::FailurePropagation:
      return lower(operand);

    case ir::IrKind::Program:
      return report_error(
          "VIXC3117",
          "Program IR cannot be used as a failure operand",
          operand.range());
    }

    return report_error(
        "VIXC3118",
        "unsupported operand reached failure lowering",
        operand.range());
  }

  bool FailureLowering::report_error(
      const char *code,
      const char *message,
      SourceRange range)
  {
    context_.diagnostics().emit(
        DiagnosticSeverity::Error,
        code,
        message,
        range);

    return false;
  }

} // namespace vixc::lowering::failure
