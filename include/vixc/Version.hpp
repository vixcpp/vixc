/**
 *
 *  @file Version.hpp
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

#if !defined(VIXC_VERSION_HPP)
#define VIXC_VERSION_HPP

#include <cstdint>
#include <string_view>

namespace vixc::version
{
  /**
   * @brief Major component of the VixC version.
   *
   * The major version changes when the public frontend contract introduces
   * incompatible changes that require callers to update their integration.
   */
  inline constexpr std::uint32_t major = 0;

  /**
   * @brief Minor component of the VixC version.
   *
   * The minor version changes when compatible frontend capabilities, language
   * semantics, or public APIs are introduced.
   */
  inline constexpr std::uint32_t minor = 1;

  /**
   * @brief Patch component of the VixC version.
   *
   * The patch version changes for compatible fixes and implementation
   * corrections that do not alter the public contract.
   */
  inline constexpr std::uint32_t patch = 0;

  /**
   * @brief Complete textual VixC version.
   *
   * The value follows semantic-version component ordering and is suitable for
   * diagnostics, command-line output, generated metadata, and embedding tools.
   */
  inline constexpr std::string_view string = "0.1.0";

  /**
   * @brief Represents a VixC version as numeric components.
   *
   * Version values can be compared without parsing the textual version string.
   * Comparison follows major, minor, then patch ordering.
   */
  struct Version final
  {
    /**
     * @brief Major version component.
     */
    std::uint32_t major{0};

    /**
     * @brief Minor version component.
     */
    std::uint32_t minor{0};

    /**
     * @brief Patch version component.
     */
    std::uint32_t patch{0};

    /**
     * @brief Compares two versions for equality.
     *
     * @param other Version to compare with this value.
     *
     * @return true when all version components are equal.
     */
    [[nodiscard]]
    constexpr bool operator==(const Version &other) const noexcept
    {
      return major == other.major && minor == other.minor && patch == other.patch;
    }

    /**
     * @brief Reports whether two versions differ.
     *
     * @param other Version to compare with this value.
     *
     * @return true when at least one component differs.
     */
    [[nodiscard]]
    constexpr bool operator!=(const Version &other) const noexcept
    {
      return !(*this == other);
    }

    /**
     * @brief Reports whether this version precedes another version.
     *
     * Comparison is performed lexicographically by major, minor, and patch.
     *
     * @param other Version to compare with this value.
     *
     * @return true when this version is older than other.
     */
    [[nodiscard]]
    constexpr bool operator<(const Version &other) const noexcept
    {
      if (major != other.major)
        return major < other.major;

      if (minor != other.minor)
        return minor < other.minor;

      return patch < other.patch;
    }

    /**
     * @brief Reports whether this version is newer than another version.
     *
     * @param other Version to compare with this value.
     *
     * @return true when this version is newer than other.
     */
    [[nodiscard]]
    constexpr bool operator>(const Version &other) const noexcept
    {
      return other < *this;
    }

    /**
     * @brief Reports whether this version is older than or equal to another.
     *
     * @param other Version to compare with this value.
     *
     * @return true when this version does not exceed other.
     */
    [[nodiscard]]
    constexpr bool operator<=(const Version &other) const noexcept
    {
      return !(*this > other);
    }

    /**
     * @brief Reports whether this version is newer than or equal to another.
     *
     * @param other Version to compare with this value.
     *
     * @return true when this version is not older than other.
     */
    [[nodiscard]]
    constexpr bool operator>=(const Version &other) const noexcept
    {
      return !(*this < other);
    }
  };

  /**
   * @brief Current VixC version as numeric components.
   */
  inline constexpr Version current{
      major,
      minor,
      patch};

  /**
   * @brief Reports whether the current VixC version satisfies a minimum
   * version requirement.
   *
   * @param required_major Required major version.
   * @param required_minor Required minor version.
   * @param required_patch Required patch version.
   *
   * @return true when the current version is greater than or equal to the
   *         requested version.
   */
  [[nodiscard]]
  constexpr bool at_least(
      std::uint32_t required_major,
      std::uint32_t required_minor = 0,
      std::uint32_t required_patch = 0) noexcept
  {
    return current >= Version{
                          required_major,
                          required_minor,
                          required_patch};
  }

} // namespace vixc::version

#endif // VIXC_VERSION_HPP
