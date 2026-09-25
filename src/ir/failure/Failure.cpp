/**
 *
 *  @file Failure.cpp
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

#include "Failure.hpp"

#include <utility>

namespace vixc::ir::failure
{

  Failure::Failure(
      SourceRange range,
      SourceRange failure_type_range) noexcept
      : IrNode(
            IrKind::Failure,
            range),
        failure_type_range_(failure_type_range)
  {
  }

  Failure::Failure(
      SourceRange range,
      SourceRange failure_type_range,
      std::unique_ptr<IrNode> operand)
      : IrNode(
            IrKind::Failure,
            range),
        failure_type_range_(failure_type_range)
  {
    set_operand(
        std::move(operand));
  }

  IrNode *Failure::set_operand(
      std::unique_ptr<IrNode> operand)
  {
    if (!operand)
      return nullptr;

    if (child_count() != 0)
      return nullptr;

    return add_child(
        std::move(operand));
  }

  const IrNode *Failure::operand() const noexcept
  {
    return child(0);
  }

  IrNode *Failure::operand() noexcept
  {
    return child(0);
  }

  SourceRange
  Failure::failure_type_range() const noexcept
  {
    return failure_type_range_;
  }

  bool Failure::valid() const noexcept
  {
    if (!range().valid())
      return false;

    if (!failure_type_range_.valid())
      return false;

    if (failure_type_range_.empty())
      return false;

    if (
        range().source_id() != failure_type_range_.source_id())
    {
      return false;
    }

    if (child_count() != 1)
      return false;

    if (operand() == nullptr)
      return false;

    return true;
  }

  FailurePropagation::FailurePropagation(
      SourceRange range,
      SourceRange failure_type_range) noexcept
      : IrNode(
            IrKind::FailurePropagation,
            range),
        failure_type_range_(failure_type_range)
  {
  }

  FailurePropagation::FailurePropagation(
      SourceRange range,
      SourceRange failure_type_range,
      std::unique_ptr<IrNode> operand)
      : IrNode(
            IrKind::FailurePropagation,
            range),
        failure_type_range_(failure_type_range)
  {
    set_operand(
        std::move(operand));
  }

  IrNode *FailurePropagation::set_operand(
      std::unique_ptr<IrNode> operand)
  {
    if (!operand)
      return nullptr;

    if (child_count() != 0)
      return nullptr;

    return add_child(
        std::move(operand));
  }

  const IrNode *
  FailurePropagation::operand() const noexcept
  {
    return child(0);
  }

  IrNode *
  FailurePropagation::operand() noexcept
  {
    return child(0);
  }

  SourceRange
  FailurePropagation::failure_type_range() const noexcept
  {
    return failure_type_range_;
  }

  bool FailurePropagation::valid() const noexcept
  {
    if (!range().valid())
      return false;

    if (!failure_type_range_.valid())
      return false;

    if (failure_type_range_.empty())
      return false;

    if (
        range().source_id() != failure_type_range_.source_id())
    {
      return false;
    }

    if (child_count() != 1)
      return false;

    if (operand() == nullptr)
      return false;

    return true;
  }

} // namespace vixc::ir::failure
