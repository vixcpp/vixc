/**
 *
 *  @file FailureFunctionId.hpp
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

#if !defined(VIXC_IR_FAILURE_FUNCTION_ID_HPP)
#define VIXC_IR_FAILURE_FUNCTION_ID_HPP

#include <cstddef>
#include <limits>

namespace vixc::ir
{
  /**
   * @brief Opaque identity of a failure-aware declaration in one analysis.
   *
   * Values are assigned by semantic declaration collection. They are stable
   * only for the frontend operation that created them and intentionally do not
   * encode a source spelling or object address.
   */
  class FailureFunctionId final
  {
  public:
    /** @brief Creates an invalid declaration identity. */
    constexpr FailureFunctionId() noexcept = default;

    /** @brief Creates an identity assigned by semantic declaration collection. */
    explicit constexpr FailureFunctionId(std::size_t value) noexcept
        : value_(value)
    {
    }

    /** @brief Reports whether this identity names a collected declaration. */
    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
      return value_ != invalid_value;
    }

    [[nodiscard]]
    friend constexpr bool operator==(
        FailureFunctionId left,
        FailureFunctionId right) noexcept
    {
      return left.value_ == right.value_;
    }

  private:
    static constexpr std::size_t invalid_value =
        std::numeric_limits<std::size_t>::max();

    std::size_t value_{invalid_value};
  };

} // namespace vixc::ir

#endif // VIXC_IR_FAILURE_FUNCTION_ID_HPP
