/**
 *
 *  @file SourceLocation.hpp
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

#if !defined(VIXC_SOURCE_LOCATION_HPP)
#define VIXC_SOURCE_LOCATION_HPP

#include <cstddef>
#include <limits>

namespace vixc
{
  /**
   * @brief Stable identifier for a source unit known by the frontend.
   *
   * Source identifiers are assigned by the source management layer and remain
   * stable for the lifetime of the frontend operation that created them.
   *
   * A SourceId identifies a source unit, not necessarily a physical file.
   * Sources may originate from files, editor buffers, generated input, tests,
   * or other in-memory representations.
   */
  using SourceId = std::size_t;

  /**
   * @brief Identifies one byte position in a source unit.
   *
   * SourceLocation is the fundamental source-position type used by VixC.
   * It combines a SourceId with a zero-based byte offset into that source.
   *
   * The location intentionally stores an offset rather than a precomputed line
   * and column. Line and column information can be derived by the source
   * management layer when diagnostics are rendered. Keeping the canonical
   * representation as a source identifier and byte offset avoids duplicating
   * derived information throughout syntax trees, semantic structures, and the
   * intermediate representation.
   *
   * A default-constructed SourceLocation is invalid. Invalid locations are
   * useful for frontend objects that do not originate directly from source
   * text or whose source association has not yet been established.
   *
   * Offsets are byte offsets. They are not Unicode code-point indexes, UTF code
   * unit indexes, or terminal display columns.
   */
  class SourceLocation final
  {
  public:
    /**
     * @brief Sentinel value used to represent an invalid source identifier.
     */
    static constexpr SourceId invalid_source_id =
        std::numeric_limits<SourceId>::max();

    /**
     * @brief Creates an invalid source location.
     *
     * The location remains invalid until replaced with a location constructed
     * from a valid source identifier and offset.
     */
    constexpr SourceLocation() noexcept = default;

    /**
     * @brief Creates a source location.
     *
     * SourceLocation does not verify that the source identifier exists or that
     * the offset lies within the corresponding source. Those checks belong to
     * the source management layer that owns the source text.
     *
     * @param source_id Identifier of the source unit.
     * @param offset Zero-based byte offset into the source.
     */
    constexpr SourceLocation(
        SourceId source_id,
        std::size_t offset) noexcept
        : source_id_(source_id),
          offset_(offset)
    {
    }

    /**
     * @brief Returns the identifier of the source containing this location.
     *
     * @return Source identifier, or invalid_source_id when the location is
     *         invalid.
     */
    [[nodiscard]]
    constexpr SourceId source_id() const noexcept
    {
      return source_id_;
    }

    /**
     * @brief Returns the byte offset within the source.
     *
     * The offset is meaningful only when valid() returns true.
     *
     * @return Zero-based byte offset into the source.
     */
    [[nodiscard]]
    constexpr std::size_t offset() const noexcept
    {
      return offset_;
    }

    /**
     * @brief Reports whether this location identifies a source position.
     *
     * Validity here means that the location contains a non-sentinel SourceId.
     * It does not prove that the identifier belongs to a particular
     * SourceManager or that the offset is within the corresponding source.
     *
     * @return true when the location contains a source identifier, otherwise
     *         false.
     */
    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
      return source_id_ != invalid_source_id;
    }

    /**
     * @brief Compares two source locations for exact identity.
     *
     * Two locations are equal when both their source identifiers and byte
     * offsets are equal.
     *
     * @param other Location to compare with this one.
     *
     * @return true when both locations identify the same source position.
     */
    [[nodiscard]]
    constexpr bool operator==(const SourceLocation &other) const noexcept
    {
      return source_id_ == other.source_id_ && offset_ == other.offset_;
    }

    /**
     * @brief Reports whether two source locations differ.
     *
     * @param other Location to compare with this one.
     *
     * @return true when the source identifier or byte offset differs.
     */
    [[nodiscard]]
    constexpr bool operator!=(const SourceLocation &other) const noexcept
    {
      return !(*this == other);
    }

  private:
    /// Source unit containing the location.
    SourceId source_id_{invalid_source_id};

    /// Zero-based byte offset within the source unit.
    std::size_t offset_{0};
  };

} // namespace vixc

#endif // VIXC_SOURCE_LOCATION_HPP
