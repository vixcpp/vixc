/**
 *
 *  @file Version.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vixc
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix++
 *
 */
#ifndef VIXC_VERSION_HPP
#define VIXC_VERSION_HPP

#include <string_view>

namespace vixc
{
  /**
   * @brief Returns the current Vix++ version.
   *
   * The value is provided by the build system through the VIXC_VERSION
   * compile definition. If the definition is not available, the version
   * falls back to "dev".
   *
   * @return The Vix++ version string.
   */
  inline constexpr std::string_view version() noexcept
  {
#ifdef VIXC_VERSION
    return VIXC_VERSION;
#else
    return "dev";
#endif
  }

  /**
   * @brief Returns the Vix++ product name.
   *
   * @return The stable product name used in CLI output and diagnostics.
   */
  inline constexpr std::string_view product_name() noexcept
  {
    return "Vix++";
  }

  /**
   * @brief Returns the Vix++ repository URL.
   *
   * @return The official Vix++ source repository URL.
   */
  inline constexpr std::string_view repository_url() noexcept
  {
    return "https://github.com/vixcpp/vixc";
  }
}

#endif
