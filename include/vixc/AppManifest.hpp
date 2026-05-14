/**
 *
 *  @file AppManifest.hpp
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
#ifndef VIXC_APP_MANIFEST_HPP
#define VIXC_APP_MANIFEST_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <vixc/Diagnostic.hpp>

namespace vixc
{
  /**
   * @brief Describes the target type declared by a VixC app manifest.
   */
  enum class AppTargetType
  {
    Executable,
    StaticLibrary,
    SharedLibrary
  };

  /**
   * @brief Converts an app target type to a stable text value.
   *
   * @param type Target type to convert.
   * @return Stable target type string.
   */
  [[nodiscard]] std::string to_string(AppTargetType type);

  /**
   * @brief Parses an app target type from a text value.
   *
   * Supported values are:
   *
   * - executable
   * - static
   * - static-library
   * - shared
   * - shared-library
   * - library
   *
   * @param value Raw target type value.
   * @return Parsed target type on success, std::nullopt on failure.
   */
  [[nodiscard]] std::optional<AppTargetType> app_target_type_from_string(
      const std::string &value);

  /**
   * @brief Simple VixC application manifest.
   *
   * AppManifest represents the content of a vix.app file used by VixC.
   * It is intentionally small and describes one generated C++ target.
   *
   * VixC accepts .vix sources in this manifest. These sources are transpiled
   * to generated C++ files before the internal CMake project is created.
   */
  struct AppManifest
  {
    /**
     * @brief Application or target name.
     */
    std::string name{};

    /**
     * @brief Target type.
     */
    AppTargetType type{AppTargetType::Executable};

    /**
     * @brief C++ standard value.
     *
     * Example values:
     *
     * - c++17
     * - c++20
     * - c++23
     */
    std::string standard{"c++20"};

    /**
     * @brief Source files relative to the project directory.
     *
     * VixC source files normally use the .vix extension.
     */
    std::vector<std::string> sources{};

    /**
     * @brief Include directories relative to the project directory.
     */
    std::vector<std::string> include_dirs{};

    /**
     * @brief Preprocessor definitions.
     */
    std::vector<std::string> defines{};

    /**
     * @brief CMake targets or libraries linked by the generated target.
     */
    std::vector<std::string> links{};

    /**
     * @brief Checks whether the manifest has the minimum valid fields.
     *
     * @return True if the manifest can be used.
     */
    [[nodiscard]] bool valid() const;
  };

  /**
   * @brief Result returned when loading a vix.app file.
   */
  struct AppManifestLoadResult
  {
    /**
     * @brief Parsed manifest.
     */
    AppManifest manifest{};

    /**
     * @brief Error message when loading failed.
     */
    std::string error{};

    /**
     * @brief Checks whether the load operation succeeded.
     *
     * @return True if the manifest was loaded successfully.
     */
    [[nodiscard]] bool success() const;
  };

  /**
   * @brief Loads a VixC app manifest from disk.
   *
   * @param path Path to the vix.app file.
   * @param diagnostics Diagnostic collection used for reporting errors.
   * @return Load result containing either a manifest or an error.
   */
  [[nodiscard]] AppManifestLoadResult load_app_manifest(
      const std::filesystem::path &path,
      DiagnosticBag &diagnostics);
}

#endif
