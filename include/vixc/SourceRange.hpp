/**
 *
 *  @file SourceRange.hpp
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

#if !defined(VIXC_SOURCE_RANGE_HPP)
#define VIXC_SOURCE_RANGE_HPP

#include <vixc/SourceLocation.hpp>

#include <cstddef>

namespace vixc
{
  /**
   * @brief Represents a half-open range of source text.
   *
   * SourceRange identifies a contiguous region within one source unit using a
   * begin location and an end location.
   *
   * The range follows half-open interval semantics: the begin position is
   * included and the end position is excluded. A range covering bytes 4
   * through 7 therefore has begin offset 4 and end offset 8.
   *
   * Half-open ranges compose naturally with source slicing and allow an empty
   * range to be represented by identical begin and end locations.
   *
   * A valid range requires both locations to be valid, both locations to refer
   * to the same source, and the begin offset to be less than or equal to the
   * end offset.
   *
   * SourceRange does not verify that the offsets exist inside the corresponding
   * SourceFile. Bounds validation belongs to the source management layer.
   */
  class SourceRange final
  {
  public:
    /**
     * @brief Creates an invalid source range.
     *
     * Both endpoints are default-constructed invalid SourceLocation values.
     */
    constexpr SourceRange() noexcept = default;

    /**
     * @brief Creates a range from two source locations.
     *
     * The constructor preserves the supplied locations exactly. Use valid() to
     * determine whether they form a well-defined range.
     *
     * @param begin Inclusive beginning of the source range.
     * @param end Exclusive end of the source range.
     */
    constexpr SourceRange(
        SourceLocation begin,
        SourceLocation end) noexcept
        : begin_(begin),
          end_(end)
    {
    }

    /**
     * @brief Creates a source range from one source identifier and two offsets.
     *
     * This is equivalent to constructing SourceLocation values for the begin
     * and end positions and passing them to the primary constructor.
     *
     * @param source_id Identifier of the source containing the range.
     * @param begin_offset Inclusive zero-based byte offset.
     * @param end_offset Exclusive zero-based byte offset.
     */
    constexpr SourceRange(
        SourceId source_id,
        std::size_t begin_offset,
        std::size_t end_offset) noexcept
        : begin_(source_id, begin_offset),
          end_(source_id, end_offset)
    {
    }

    /**
     * @brief Returns the inclusive beginning of the range.
     *
     * @return Beginning source location.
     */
    [[nodiscard]]
    constexpr SourceLocation begin() const noexcept
    {
      return begin_;
    }

    /**
     * @brief Returns the exclusive end of the range.
     *
     * @return Ending source location.
     */
    [[nodiscard]]
    constexpr SourceLocation end() const noexcept
    {
      return end_;
    }

    /**
     * @brief Returns the source identifier associated with the range.
     *
     * The result is meaningful when valid() returns true.
     *
     * @return Source identifier stored by the beginning location.
     */
    [[nodiscard]]
    constexpr SourceId source_id() const noexcept
    {
      return begin_.source_id();
    }

    /**
     * @brief Returns the inclusive beginning byte offset.
     *
     * @return Zero-based byte offset of the beginning location.
     */
    [[nodiscard]]
    constexpr std::size_t begin_offset() const noexcept
    {
      return begin_.offset();
    }

    /**
     * @brief Returns the exclusive ending byte offset.
     *
     * @return Zero-based byte offset of the ending location.
     */
    [[nodiscard]]
    constexpr std::size_t end_offset() const noexcept
    {
      return end_.offset();
    }

    /**
     * @brief Returns the length of the range in bytes.
     *
     * Invalid ranges have a length of zero.
     *
     * @return Number of bytes covered by the range.
     */
    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      if (!valid())
        return 0;

      return end_.offset() - begin_.offset();
    }

    /**
     * @brief Reports whether the range contains no source bytes.
     *
     * A valid range is empty when its begin and end offsets are equal.
     * Invalid ranges are not considered empty.
     *
     * @return true when the range is valid and has zero length, otherwise
     *         false.
     */
    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
      return valid() && begin_.offset() == end_.offset();
    }

    /**
     * @brief Reports whether the endpoints form a valid source range.
     *
     * A range is structurally valid when both endpoints are valid, both refer
     * to the same source, and the begin offset does not exceed the end offset.
     *
     * This function does not verify source bounds against a SourceFile.
     *
     * @return true when the range is structurally valid, otherwise false.
     */
    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
      return begin_.valid() && end_.valid() && begin_.source_id() == end_.source_id() && begin_.offset() <= end_.offset();
    }

    /**
     * @brief Reports whether a location lies inside the range.
     *
     * Because SourceRange uses half-open semantics, a location at begin() is
     * contained while a location exactly equal to end() is not.
     *
     * Invalid ranges and locations are never considered to contain a position.
     *
     * @param location Source location to test.
     *
     * @return true when the location belongs to this range.
     */
    [[nodiscard]]
    constexpr bool contains(SourceLocation location) const noexcept
    {
      return valid() && location.valid() && location.source_id() == source_id() && location.offset() >= begin_offset() && location.offset() < end_offset();
    }

    /**
     * @brief Compares two source ranges for exact identity.
     *
     * @param other Range to compare with this one.
     *
     * @return true when both ranges have identical begin and end locations.
     */
    [[nodiscard]]
    constexpr bool operator==(const SourceRange &other) const noexcept
    {
      return begin_ == other.begin_ && end_ == other.end_;
    }

    /**
     * @brief Reports whether two source ranges differ.
     *
     * @param other Range to compare with this one.
     *
     * @return true when either endpoint differs.
     */
    [[nodiscard]]
    constexpr bool operator!=(const SourceRange &other) const noexcept
    {
      return !(*this == other);
    }

  private:
    /// Inclusive beginning of the source range.
    SourceLocation begin_{};

    /// Exclusive end of the source range.
    SourceLocation end_{};
  };

} // namespace vixc

#endif // VIXC_SOURCE_RANGE_HPP
