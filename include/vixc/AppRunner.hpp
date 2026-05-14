/**
 *
 *  @file AppRunner.hpp
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
#ifndef VIXC_APP_RUNNER_HPP
#define VIXC_APP_RUNNER_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <vixc/AppCMakeGenerator.hpp>
#include <vixc/AppManifest.hpp>
#include <vixc/Diagnostic.hpp>
#include <vixc/Transpiler.hpp>

namespace vixc
{
  /**
   * @brief Result returned after preparing a VixC app project.
   */
  struct AppRunnerResult
  {
    /**
     * @brief Original project directory.
     */
    std::filesystem::path project_dir{};

    /**
     * @brief Path to the vix.app manifest.
     */
    std::filesystem::path manifest_path{};

    /**
     * @brief Directory containing generated VixC sources.
     */
    std::filesystem::path generated_source_dir{};

    /**
     * @brief Directory containing the generated CMakeLists.txt.
     */
    std::filesystem::path cmake_source_dir{};

    /**
     * @brief Path to the generated CMakeLists.txt file.
     */
    std::filesystem::path cmake_lists_path{};

    /**
     * @brief Final manifest after replacing .vix sources with generated C++ files.
     */
    AppManifest manifest{};

    /**
     * @brief Generated C++ source files.
     */
    std::vector<std::filesystem::path> generated_files{};

    /**
     * @brief Error message when preparation failed.
     */
    std::string error{};

    /**
     * @brief Checks whether the app project preparation succeeded.
     *
     * @return True if the project is ready for build or run.
     */
    [[nodiscard]] bool success() const;
  };

  /**
   * @brief Prepares a VixC app project from a vix.app manifest.
   *
   * This function loads vix.app, transpiles every .vix source into a generated
   * C++ file, replaces the manifest sources with generated C++ paths, then
   * generates the internal CMake project.
   *
   * @param project_dir Project directory containing vix.app.
   * @param transpiler Transpiler used to convert .vix sources to C++.
   * @param diagnostics Diagnostic collection used for reporting errors.
   * @return App runner result.
   */
  [[nodiscard]] AppRunnerResult prepare_app_project(
      const std::filesystem::path &project_dir,
      const Transpiler &transpiler,
      DiagnosticBag &diagnostics);
}

#endif
