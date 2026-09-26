/**
 *
 *  @file LoweringPipeline.cpp
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

#include "LoweringPipeline.hpp"

#include "failure/FailureLowering.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"
#include "../ir/IrKind.hpp"
#include "../ir/IrNode.hpp"
#include "../ir/Program.hpp"
#include "../ir/failure/FailureAwareFunction.hpp"

#include <vixc/DiagnosticSeverity.hpp>

namespace vixc::lowering
{

  LoweringPipeline::LoweringPipeline(
      LoweringContext &context) noexcept
      : context_(context)
  {
  }

  bool LoweringPipeline::lower(
      ir::Program &program)
  {
    if (context_.has_errors())
      return false;

    if (!lower_node(program))
      return false;

    return !context_.has_errors();
  }

  bool LoweringPipeline::lower_node(
      ir::IrNode &node)
  {
    if (context_.has_fatal())
      return false;

    switch (node.kind())
    {
    case ir::IrKind::Invalid:
      return report_error(
          "VIXC3001",
          "invalid IR node reached the lowering pipeline",
          node.range());

    case ir::IrKind::Program:
      return lower_children(node);

    case ir::IrKind::CxxRegion:
      return true;

    case ir::IrKind::FailureAwareFunction:
    {
      auto *function = dynamic_cast<ir::failure::FailureAwareFunction *>(&node);
      if (function == nullptr || !function->valid())
      {
        return report_error(
            "VIXC3004",
            "invalid failure-aware function reached the lowering pipeline",
            node.range());
      }

      ir::failure::Outcome *outcome = function->outcome();
      if (outcome == nullptr)
      {
        return report_error(
            "VIXC3005",
            "failure-aware function has no Outcome contract",
            node.range());
      }

      failure::FailureLowering lowering{context_};
      if (!lowering.lower(*outcome))
        return false;

      return lower_children(*function);
    }

    case ir::IrKind::Return:
    {
      auto *statement = dynamic_cast<ir::failure::Return *>(&node);
      if (statement == nullptr || !statement->valid())
      {
        return report_error(
            "VIXC3006",
            "invalid Return IR reached the lowering pipeline",
            node.range());
      }

      return lower_children(*statement);
    }

    case ir::IrKind::Outcome:
    case ir::IrKind::Failure:
    case ir::IrKind::FailurePropagation:
    {
      failure::FailureLowering lowering{
          context_};

      return lowering.lower(node);
    }
    }

    return report_error(
        "VIXC3002",
        "unsupported IR node reached the lowering pipeline",
        node.range());
  }

  bool LoweringPipeline::lower_children(
      ir::IrNode &node)
  {
    bool successful = true;

    for (std::size_t index = 0;
         index < node.child_count();
         ++index)
    {
      if (context_.has_fatal())
        return false;

      ir::IrNode *child =
          node.child(index);

      if (child == nullptr)
      {
        successful = false;

        report_error(
            "VIXC3003",
            "IR node contains an invalid child",
            node.range());

        continue;
      }

      if (!lower_node(*child))
        successful = false;
    }

    return successful;
  }

  bool LoweringPipeline::report_error(
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

} // namespace vixc::lowering
