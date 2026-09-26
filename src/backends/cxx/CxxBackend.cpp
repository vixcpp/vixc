/**
 *
 *  @file CxxBackend.cpp
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

#include "CxxBackend.hpp"

#include "../../diagnostics/DiagnosticEngine.hpp"
#include "../../ir/IrKind.hpp"
#include "../../ir/IrNode.hpp"
#include "../../ir/Program.hpp"
#include "../../ir/failure/Failure.hpp"
#include "../../ir/failure/FailureAwareFunction.hpp"
#include "../../ir/failure/Outcome.hpp"
#include "../../source/SourceFile.hpp"
#include "../../source/SourceManager.hpp"

#include <vixc/DiagnosticSeverity.hpp>

namespace vixc::backends::cxx
{
  CxxBackend::CxxBackend(
      source::SourceManager &sources,
      diagnostics::DiagnosticEngine &diagnostics) noexcept
      : sources_(sources),
        diagnostics_(diagnostics)
  {
  }

  std::string_view
  CxxBackend::name() const noexcept
  {
    return "cxx";
  }

  bool CxxBackend::generate(
      const ir::Program &program)
  {
    reset();

    if (diagnostics_.has_errors())
      return false;

    if (!emit_node(program))
      return false;

    return !diagnostics_.has_errors();
  }

  void CxxBackend::reset()
  {
    emitter_.reset();
  }

  const std::string &
  CxxBackend::output() const noexcept
  {
    return emitter_.output();
  }

  const CxxSourceMap &
  CxxBackend::source_map() const noexcept
  {
    return emitter_.source_map();
  }

  bool CxxBackend::empty() const noexcept
  {
    return emitter_.empty();
  }

  bool CxxBackend::emit_node(
      const ir::IrNode &node)
  {
    if (diagnostics_.has_fatal())
      return false;

    switch (node.kind())
    {
    case ir::IrKind::Invalid:
      return report_error(
          "VIXC4001",
          "invalid IR node reached the C++ backend",
          node.range());

    case ir::IrKind::Program:
      return emit_children(node);

    case ir::IrKind::CxxRegion:
      return emit_cxx_region(node);

    case ir::IrKind::FailureAwareFunction:
    {
      const auto *function = dynamic_cast<
          const ir::failure::FailureAwareFunction *>(&node);
      if (function == nullptr)
      {
        return report_error(
            "VIXC4017",
            "IR node marked as FailureAwareFunction has an incompatible concrete type",
            node.range());
      }

      return emit_failure_aware_function(*function);
    }

    case ir::IrKind::Return:
      return report_error(
          "VIXC4018",
          "Return IR requires declaration-level C++ lowering before emission",
          node.range());

    case ir::IrKind::Outcome:
    {
      const auto *outcome =
          dynamic_cast<
              const ir::failure::Outcome *>(
              &node);

      if (outcome == nullptr)
      {
        return report_error(
            "VIXC4002",
            "IR node marked as Outcome has an incompatible concrete type",
            node.range());
      }

      return emit_outcome(*outcome);
    }

    case ir::IrKind::Failure:
    {
      const auto *failure =
          dynamic_cast<
              const ir::failure::Failure *>(
              &node);

      if (failure == nullptr)
      {
        return report_error(
            "VIXC4003",
            "IR node marked as Failure has an incompatible concrete type",
            node.range());
      }

      return emit_failure(*failure);
    }

    case ir::IrKind::FailurePropagation:
    {
      const auto *propagation =
          dynamic_cast<
              const ir::failure::FailurePropagation *>(
              &node);

      if (propagation == nullptr)
      {
        return report_error(
            "VIXC4004",
            "IR node marked as FailurePropagation has an incompatible concrete type",
            node.range());
      }

      return emit_failure_propagation(
          *propagation);
    }
    }

    return report_error(
        "VIXC4005",
        "unsupported IR node reached the C++ backend",
        node.range());
  }

  bool CxxBackend::emit_children(
      const ir::IrNode &node)
  {
    bool successful = true;

    for (std::size_t index = 0;
         index < node.child_count();
         ++index)
    {
      if (diagnostics_.has_fatal())
        return false;

      const ir::IrNode *child =
          node.child(index);

      if (child == nullptr)
      {
        successful = false;

        report_error(
            "VIXC4006",
            "IR node contains an invalid child",
            node.range());

        continue;
      }

      if (!emit_node(*child))
        successful = false;
    }

    return successful;
  }

  bool CxxBackend::emit_cxx_region(
      const ir::IrNode &node)
  {
    const SourceRange range =
        node.range();

    if (!range.valid())
    {
      return report_error(
          "VIXC4007",
          "CxxRegion has no valid source range",
          range);
    }

    const std::string_view text =
        source_text(range);

    if (
        text.empty() && !range.empty())
    {
      return report_error(
          "VIXC4008",
          "unable to recover source text for CxxRegion",
          range);
    }

    emitter_.write(
        text,
        range);

    return true;
  }

  bool CxxBackend::emit_outcome(
      const ir::failure::Outcome &outcome)
  {
    if (!outcome.valid())
    {
      return report_error(
          "VIXC4009",
          "invalid Outcome IR reached the C++ backend",
          outcome.range());
    }

    /*
     * Outcome is semantic contract metadata.
     *
     * The node itself does not correspond to an independent C++ statement or
     * expression. Concrete C++ representation is introduced where the
     * surrounding declaration is lowered.
     *
     * Keeping this node emission-free prevents the backend from inventing a
     * standalone runtime object that is not present in the VixC program.
     */
    if (!outcome.allows_failure())
      return true;

    const SourceRange failure_type =
        outcome.failure_type_range();

    const std::string_view type_text =
        source_text(failure_type);

    if (
        type_text.empty() && !failure_type.empty())
    {
      return report_error(
          "VIXC4010",
          "unable to recover the declared failure type",
          failure_type);
    }

    return true;
  }

  bool CxxBackend::emit_failure_aware_function(
      const ir::failure::FailureAwareFunction &function)
  {
    if (!function.valid())
    {
      return report_error(
          "VIXC4019",
          "invalid FailureAwareFunction IR reached the C++ backend",
          function.range());
    }

    return report_error(
        "VIXC4013",
        "Failure-aware function requires declaration-level C++ lowering before emission",
        function.range());
  }

  bool CxxBackend::emit_failure(
      const ir::failure::Failure &failure)
  {
    if (!failure.valid())
    {
      return report_error(
          "VIXC4011",
          "invalid Failure IR reached the C++ backend",
          failure.range());
    }

    const ir::IrNode *operand =
        failure.operand();

    if (operand == nullptr)
    {
      return report_error(
          "VIXC4012",
          "Failure IR has no failure value operand",
          failure.range());
    }

    /*
     * A recoverable failure cannot be emitted correctly until the surrounding
     * failure-aware declaration has been lowered to a concrete C++ return
     * representation.
     *
     * Emitting `return`, throwing an exception, or choosing std::expected here
     * would make a backend representation decision without the declaration
     * context required to preserve the VixC contract.
     */
    return report_error(
        "VIXC4013",
        "Failure IR requires declaration-level C++ lowering before emission",
        failure.range());
  }

  bool CxxBackend::emit_failure_propagation(
      const ir::failure::FailurePropagation &propagation)
  {
    if (!propagation.valid())
    {
      return report_error(
          "VIXC4014",
          "invalid FailurePropagation IR reached the C++ backend",
          propagation.range());
    }

    const ir::IrNode *operand =
        propagation.operand();

    if (operand == nullptr)
    {
      return report_error(
          "VIXC4015",
          "FailurePropagation IR has no operand",
          propagation.range());
    }

    /*
     * Propagation requires control over the enclosing computation because a
     * failed operand must terminate that computation while a successful operand
     * contributes its value.
     *
     * This cannot be represented portably as an isolated C++ expression.
     * Declaration and control-flow lowering must therefore resolve propagation
     * before the node reaches final C++ emission.
     */
    return report_error(
        "VIXC4016",
        "FailurePropagation IR requires control-flow lowering before C++ emission",
        propagation.range());
  }

  std::string_view
  CxxBackend::source_text(
      SourceRange range) const noexcept
  {
    if (!range.valid())
      return {};

    const source::SourceFile *file =
        sources_.get(
            range.source_id());

    if (file == nullptr)
      return {};

    const std::size_t begin =
        range.begin_offset();

    const std::size_t end =
        range.end_offset();

    if (begin > file->size())
      return {};

    if (end > file->size())
      return {};

    if (end < begin)
      return {};

    return file->contents().substr(
        begin,
        end - begin);
  }

  bool CxxBackend::report_error(
      const char *code,
      const char *message,
      SourceRange range)
  {
    diagnostics_.emit(
        DiagnosticSeverity::Error,
        code,
        message,
        range);

    return false;
  }

} // namespace vixc::backends::cxx
