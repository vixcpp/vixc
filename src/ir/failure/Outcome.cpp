/**
 *
 *  @file Outcome.cpp
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

#include "Outcome.hpp"

namespace vixc::ir::failure
{

  std::string_view
  outcome_state_name(OutcomeState state) noexcept
  {
    switch (state)
    {
    case OutcomeState::Success:
      return "success";

    case OutcomeState::None:
      return "none";

    case OutcomeState::Failure:
      return "failure";

    case OutcomeState::Stopped:
      return "stopped";
    }

    return "unknown";
  }

  Outcome::Outcome(SourceRange range) noexcept
      : IrNode(
            IrKind::Outcome,
            range)
  {
  }

  Outcome::Outcome(
      SourceRange range,
      SourceRange failure_type_range) noexcept
      : IrNode(
            IrKind::Outcome,
            range),
        states_(
            static_cast<std::uint8_t>(
                success_bit | failure_bit)),
        failure_type_range_(failure_type_range)
  {
  }

  bool Outcome::valid() const noexcept
  {
    if (!allows_success())
      return false;

    if (!allows_failure())
      return !failure_type_range_.valid();

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

    if (
        failure_type_range_.begin_offset() < range().begin_offset())
    {
      return false;
    }

    if (
        failure_type_range_.end_offset() > range().end_offset())
    {
      return false;
    }

    return true;
  }

  bool Outcome::allows(OutcomeState state) const noexcept
  {
    return (states_ & bit_for(state)) != 0;
  }

  bool Outcome::allows_success() const noexcept
  {
    return allows(OutcomeState::Success);
  }

  bool Outcome::allows_none() const noexcept
  {
    return allows(OutcomeState::None);
  }

  bool Outcome::allows_failure() const noexcept
  {
    return allows(OutcomeState::Failure);
  }

  bool Outcome::allows_stopped() const noexcept
  {
    return allows(OutcomeState::Stopped);
  }

  bool Outcome::has_failure_contract() const noexcept
  {
    return allows_failure();
  }

  SourceRange
  Outcome::failure_type_range() const noexcept
  {
    return failure_type_range_;
  }

  std::uint8_t
  Outcome::bit_for(OutcomeState state) noexcept
  {
    switch (state)
    {
    case OutcomeState::Success:
      return success_bit;

    case OutcomeState::None:
      return none_bit;

    case OutcomeState::Failure:
      return failure_bit;

    case OutcomeState::Stopped:
      return stopped_bit;
    }

    return 0;
  }

} // namespace vixc::ir::failure
