/**
 *
 *  @file FailureAwareFunction.cpp
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

#include "FailureAwareFunction.hpp"

#include <utility>

namespace vixc::ir::failure
{
  FailureAwareFunction::FailureAwareFunction(
      SourceRange range,
      SourceRange success_type_range,
      SourceRange declarator_range,
      SourceRange body_range,
      std::unique_ptr<Outcome> outcome)
      : IrNode(
            IrKind::FailureAwareFunction,
            range),
        success_type_range_(success_type_range),
        declarator_range_(declarator_range),
        body_range_(body_range),
        outcome_(std::move(outcome))
  {
  }

  SourceRange
  FailureAwareFunction::success_type_range() const noexcept
  {
    return success_type_range_;
  }

  SourceRange
  FailureAwareFunction::declarator_range() const noexcept
  {
    return declarator_range_;
  }

  SourceRange
  FailureAwareFunction::body_range() const noexcept
  {
    return body_range_;
  }

  const Outcome *FailureAwareFunction::outcome() const noexcept
  {
    return outcome_.get();
  }

  Outcome *FailureAwareFunction::outcome() noexcept
  {
    return outcome_.get();
  }

  bool FailureAwareFunction::valid() const noexcept
  {
    if (!range().valid() ||
        !success_type_range_.valid() || success_type_range_.empty() ||
        !declarator_range_.valid() || declarator_range_.empty() ||
        !body_range_.valid() || body_range_.empty() ||
        outcome_ == nullptr || !outcome_->valid())
    {
      return false;
    }

    if (range().source_id() != success_type_range_.source_id() ||
        range().source_id() != declarator_range_.source_id() ||
        range().source_id() != body_range_.source_id() ||
        range().source_id() != outcome_->range().source_id())
    {
      return false;
    }

    return range().begin_offset() <= success_type_range_.begin_offset() &&
           success_type_range_.end_offset() <= range().end_offset() &&
           range().begin_offset() <= declarator_range_.begin_offset() &&
           declarator_range_.end_offset() <= range().end_offset() &&
           range().begin_offset() <= body_range_.begin_offset() &&
           body_range_.end_offset() <= range().end_offset();
  }

  void FailureAwareFunction::mark_lowered() noexcept
  {
    lowered_ = true;
  }

  bool FailureAwareFunction::is_lowered() const noexcept
  {
    return lowered_;
  }

  Return::Return(
      SourceRange range,
      std::unique_ptr<IrNode> operand)
      : IrNode(
            IrKind::Return,
            range)
  {
    add_child(std::move(operand));
  }

  const IrNode *Return::operand() const noexcept
  {
    return child(0);
  }

  IrNode *Return::operand() noexcept
  {
    return child(0);
  }

  bool Return::valid() const noexcept
  {
    if (!range().valid() || child_count() != 1 || operand() == nullptr)
      return false;

    const SourceRange operand_range = operand()->range();
    if (!operand_range.valid() ||
        operand_range.empty() ||
        operand_range.source_id() != range().source_id())
    {
      return false;
    }

    return range().begin_offset() <= operand_range.begin_offset() &&
           operand_range.end_offset() <= range().end_offset();
  }

  TryInitialization::TryInitialization(
      SourceRange range,
      SourceRange declaration_range,
      std::unique_ptr<FailurePropagation> propagation)
      : IrNode(IrKind::TryInitialization, range),
        declaration_range_(declaration_range)
  {
    add_child(std::move(propagation));
  }

  SourceRange TryInitialization::declaration_range() const noexcept { return declaration_range_; }
  const FailurePropagation *TryInitialization::propagation() const noexcept { return dynamic_cast<const FailurePropagation *>(child(0)); }
  FailurePropagation *TryInitialization::propagation() noexcept { return dynamic_cast<FailurePropagation *>(child(0)); }
  void TryInitialization::set_synthetic_id(std::size_t value) noexcept { synthetic_id_ = value; has_synthetic_id_ = true; }
  std::size_t TryInitialization::synthetic_id() const noexcept { return synthetic_id_; }
  bool TryInitialization::has_synthetic_id() const noexcept { return has_synthetic_id_; }
  bool TryInitialization::valid() const noexcept
  {
    return range().valid() && declaration_range_.valid() && !declaration_range_.empty() &&
           declaration_range_.source_id() == range().source_id() && propagation() != nullptr && propagation()->valid();
  }

} // namespace vixc::ir::failure
