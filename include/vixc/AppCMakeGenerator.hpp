/**
 *
 *  @file AppCMakeGenerator.hpp
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
#ifndef VIXC_APP_CMAKE_GENERATOR_HPP
#define VIXC_APP_CMAKE_GENERATOR_HPP

#include <filesystem>
#include <string>

#include <vixc/AppManifest.hpp>
#include <vixc/Diagnostic.hpp>

namespace vixc
{
  /**
   * @brief Result returned after generating an internal CMake project.
   */
  struct AppCMakeGenerateResult
  {
    /**
     * @brief Directory containing the generated CMakeLists.txt.
     */
    std::filesystem::path source_dir{};

    /**
     * @brief Path to the generated CMakeLists.txt file.
     */
    std::filesystem::path cmake_lists_path{};

    /**
     * @brief Error message when generation failed.
     */
    std::string error{};

    /**
     * @brief Checks whether the generation succeeded.
     *
     * @return True if the generated CMake project is ready.
     */
    [[nodiscard]] bool success() const;
  };

  /**
   * @brief Builds the generated CMakeLists.txt content for a VixC app manifest.
   *
   * @param manifest Parsed vix.app manifest.
   * @param project_dir Original user project directory.
   * @return Generated CMakeLists.txt content.
   */
  [[nodiscard]] std::string generate_app_cmake_lists_content(
      const AppManifest &manifest,
      const std::filesystem::path &project_dir);

  /**
   * @brief Generates an internal CMake project for a VixC app.
   *
   * The generated project is written under:
   *
   * .vix/generated/vixc/app/CMakeLists.txt
   *
   * @param manifest Parsed vix.app manifest.
   * @param project_dir Original user project directory.
   * @param diagnostics Diagnostic collection used for reporting errors.
   * @return Generation result.
   */
  [[nodiscard]] AppCMakeGenerateResult generate_app_cmake_project(
      const AppManifest &manifest,
      const std::filesystem::path &project_dir,
      DiagnosticBag &diagnostics);
}

#endif
