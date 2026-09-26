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

#include <string>

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

    if (requires_outcome_support(program))
      emit_outcome_support();

    if (!emit_node(program))
      return false;

    return !diagnostics_.has_errors();
  }

  void CxxBackend::reset()
  {
    emitter_.reset();
    active_function_ = nullptr;
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
    {
      const auto *statement = dynamic_cast<const ir::failure::Return *>(&node);
      if (statement == nullptr)
        return report_error("VIXC4018", "IR node marked as Return has an incompatible concrete type", node.range());
      return emit_return(*statement);
    }

    case ir::IrKind::TryInitialization:
    {
      const auto *initialization = dynamic_cast<const ir::failure::TryInitialization *>(&node);
      if (initialization == nullptr)
        return report_error("VIXC4020", "IR node marked as TryInitialization has an incompatible concrete type", node.range());
      return emit_try_initialization(*initialization);
    }

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

    if (!function.is_lowered())
      return report_error("VIXC4013", "Failure-aware function requires declaration-level C++ lowering before emission", function.range());

    const SourceRange failure_type = function.outcome()->failure_type_range();
    emitter_.write("vixc_generated::Outcome<");
    emitter_.write(source_text(function.success_type_range()), function.success_type_range());
    emitter_.write(", ");
    emitter_.write(source_text(failure_type), failure_type);
    emitter_.write(">\n");
    emitter_.write(source_text(function.declarator_range()), function.declarator_range());

    active_function_ = &function;
    const bool emitted = emit_children(function);
    active_function_ = nullptr;
    return emitted;
  }

  bool CxxBackend::emit_return(const ir::failure::Return &statement)
  {
    if (active_function_ == nullptr || !statement.valid())
      return report_error("VIXC4021", "Return IR has no active lowered failure-aware function", statement.range());
    const ir::IrNode *operand = statement.operand();
    emitter_.write("return vixc_generated::Outcome<", statement.range());
    emitter_.write(source_text(active_function_->success_type_range()));
    emitter_.write(", ");
    emitter_.write(source_text(active_function_->outcome()->failure_type_range()));
    emitter_.write(">::success(");
    emitter_.write(source_text(operand->range()), operand->range());
    emitter_.write(");", statement.range());
    return true;
  }

  bool CxxBackend::emit_try_initialization(const ir::failure::TryInitialization &initialization)
  {
    if (active_function_ == nullptr || !initialization.valid() || !initialization.has_synthetic_id())
      return report_error("VIXC4022", "Try initialization has no active lowered failure-aware function", initialization.range());
    const ir::failure::FailurePropagation *propagation = initialization.propagation();
    const ir::IrNode *operand = propagation->operand();
    const std::string id = std::to_string(initialization.synthetic_id());
    const SourceRange failure_type = active_function_->outcome()->failure_type_range();
    emitter_.write("auto __vixc_outcome_" + id + " = ", initialization.range());
    emitter_.write(source_text(operand->range()), operand->range());
    emitter_.write(";\nif (!__vixc_outcome_" + id + ".has_value())\n{\n  return vixc_generated::Outcome<");
    emitter_.write(source_text(active_function_->success_type_range()));
    emitter_.write(", ");
    emitter_.write(source_text(failure_type));
    emitter_.write(">::failure(std::move(__vixc_outcome_" + id + ").take_error());\n}\n");
    emitter_.write(source_text(initialization.declaration_range()), initialization.declaration_range());
    emitter_.write(" = std::move(__vixc_outcome_" + id + ").take_value();", initialization.range());
    return true;
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

    if (active_function_ == nullptr || !active_function_->is_lowered())
      return report_error("VIXC4013", "Failure IR requires declaration-level C++ lowering before emission", failure.range());

    emitter_.write("return vixc_generated::Outcome<", failure.range());
    emitter_.write(source_text(active_function_->success_type_range()));
    emitter_.write(", ");
    emitter_.write(source_text(active_function_->outcome()->failure_type_range()));
    emitter_.write(">::failure(");
    emitter_.write(source_text(operand->range()), operand->range());
    emitter_.write(");", failure.range());
    return true;
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

  bool CxxBackend::requires_outcome_support(const ir::IrNode &node) const noexcept
  {
    if (node.kind() == ir::IrKind::FailureAwareFunction)
    {
      const auto *function = dynamic_cast<const ir::failure::FailureAwareFunction *>(&node);
      return function != nullptr && function->is_lowered();
    }

    for (const auto &child : node.children())
    {
      if (child != nullptr && requires_outcome_support(*child))
        return true;
    }

    return false;
  }

  void CxxBackend::emit_outcome_support()
  {
    emitter_.write(
        "#include <utility>\n"
        "#include <variant>\n\n"
        "namespace vixc_generated\n{\n"
        "template <typename T, typename E>\n"
        "class Outcome\n{\npublic:\n"
        "  static Outcome success(T value) { return Outcome{std::in_place_index<0>, std::move(value)}; }\n"
        "  static Outcome failure(E error) { return Outcome{std::in_place_index<1>, std::move(error)}; }\n"
        "  [[nodiscard]] bool has_value() const noexcept { return storage_.index() == 0; }\n"
        "  [[nodiscard]] T &&take_value() noexcept { return std::get<0>(std::move(storage_)); }\n"
        "  [[nodiscard]] E &&take_error() noexcept { return std::get<1>(std::move(storage_)); }\n"
        "private:\n"
        "  template <typename... Args> Outcome(std::in_place_index_t<0>, Args &&... args) : storage_(std::in_place_index<0>, std::forward<Args>(args)...) {}\n"
        "  template <typename... Args> Outcome(std::in_place_index_t<1>, Args &&... args) : storage_(std::in_place_index<1>, std::forward<Args>(args)...) {}\n"
        "  std::variant<T, E> storage_;\n};\n}\n\n");
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
