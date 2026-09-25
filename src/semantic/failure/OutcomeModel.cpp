/**
 *
 *  @file OutcomeModel.cpp
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

#include "OutcomeModel.hpp"

namespace vixc::semantic::failure
{
  std::string_view
  outcome_kind_name(OutcomeKind kind) noexcept
  {
    switch (kind)
    {
    case OutcomeKind::Success:
      return "success";

    case OutcomeKind::None:
      return "none";

    case OutcomeKind::Failure:
      return "failure";

    case OutcomeKind::Stopped:
      return "stopped";
    }

    return "unknown";
  }

  OutcomeModel::OutcomeModel() noexcept = default;

  OutcomeModel::OutcomeModel(
      SourceRange specification_range,
      SourceRange failure_type_range) noexcept
      : outcomes_(
            static_cast<std::uint8_t>(
                success_bit | failure_bit)),
        failure_specification_range_(specification_range),
        failure_type_range_(failure_type_range)
  {
  }

  bool OutcomeModel::valid() const noexcept
  {
    if (!allows_failure())
    {
      return !failure_specification_range_.valid() && !failure_type_range_.valid();
    }

    if (
        !failure_specification_range_.valid() || !failure_type_range_.valid())
    {
      return false;
    }

    if (failure_type_range_.empty())
      return false;

    if (
        failure_specification_range_.source_id() != failure_type_range_.source_id())
    {
      return false;
    }

    if (
        failure_type_range_.begin_offset() < failure_specification_range_.begin_offset())
    {
      return false;
    }

    if (
        failure_type_range_.end_offset() > failure_specification_range_.end_offset())
    {
      return false;
    }

    return true;
  }

  bool OutcomeModel::allows(OutcomeKind kind) const noexcept
  {
    return (outcomes_ & bit_for(kind)) != 0;
  }

  bool OutcomeModel::allows_success() const noexcept
  {
    return allows(OutcomeKind::Success);
  }

  bool OutcomeModel::allows_none() const noexcept
  {
    return allows(OutcomeKind::None);
  }

  bool OutcomeModel::allows_failure() const noexcept
  {
    return allows(OutcomeKind::Failure);
  }

  bool OutcomeModel::allows_stopped() const noexcept
  {
    return allows(OutcomeKind::Stopped);
  }

  bool OutcomeModel::has_failure_contract() const noexcept
  {
    return allows_failure();
  }

  SourceRange
  OutcomeModel::failure_specification_range() const noexcept
  {
    return failure_specification_range_;
  }

  SourceRange
  OutcomeModel::failure_type_range() const noexcept
  {
    return failure_type_range_;
  }

  std::uint8_t
  OutcomeModel::bit_for(OutcomeKind kind) noexcept
  {
    switch (kind)
    {
    case OutcomeKind::Success:
      return success_bit;

    case OutcomeKind::None:
      return none_bit;

    case OutcomeKind::Failure:
      return failure_bit;

    case OutcomeKind::Stopped:
      return stopped_bit;
    }

    return 0;
  }

} // namespace vixc::semantic::failure
