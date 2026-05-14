/**
 *
 *  @file AppRunner.cpp
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

#include <vixc/AppRunner.hpp>

#include <fstream>
#include <sstream>
#include <system_error>

namespace vixc
{
  namespace
  {
    [[nodiscard]] std::filesystem::path normalize_absolute(
        const std::filesystem::path &path)
    {
      std::error_code ec{};
      std::filesystem::path result = std::filesystem::absolute(path, ec);

      if (ec)
      {
        result = path;
      }

      return result.lexically_normal();
    }

    [[nodiscard]] bool is_vix_source(const std::filesystem::path &path)
    {
      return path.extension() == ".vix";
    }

    [[nodiscard]] bool is_cpp_source(const std::filesystem::path &path)
    {
      const std::filesystem::path extension = path.extension();

      return extension == ".cpp" ||
             extension == ".cc" ||
             extension == ".cxx" ||
             extension == ".c++";
    }

    [[nodiscard]] std::filesystem::path resolve_project_path(
        const std::filesystem::path &project_dir,
        const std::filesystem::path &source)
    {
      if (source.is_absolute())
      {
        return source.lexically_normal();
      }

      return (project_dir / source).lexically_normal();
    }

    [[nodiscard]] std::filesystem::path make_generated_source_path(
        const std::filesystem::path &project_dir,
        const std::filesystem::path &source)
    {
      std::filesystem::path relative_source = source;

      if (source.is_absolute())
      {
        std::error_code ec{};
        relative_source = std::filesystem::relative(source, project_dir, ec);

        if (ec)
        {
          relative_source = source.filename();
        }
      }

      std::filesystem::path output =
          project_dir / ".vix" / "generated" / "vixc" / "src" / relative_source;

      output.replace_extension(".generated.cpp");

      return output.lexically_normal();
    }

    [[nodiscard]] bool write_text_file(
        const std::filesystem::path &path,
        const std::string &content,
        DiagnosticBag &diagnostics)
    {
      const std::filesystem::path parent = path.parent_path();

      if (!parent.empty())
      {
        std::error_code ec{};
        std::filesystem::create_directories(parent, ec);

        if (ec)
        {
          diagnostics.error(
              "failed to create generated source directory",
              parent.string(),
              {},
              ec.message());

          return false;
        }
      }

      std::ofstream output(path, std::ios::binary | std::ios::trunc);

      if (!output)
      {
        diagnostics.error(
            "failed to create generated C++ source file",
            path.string(),
            {},
            "check that the generated VixC directory is writable");

        return false;
      }

      output << content;

      if (!output)
      {
        diagnostics.error(
            "failed to write generated C++ source file",
            path.string(),
            {},
            "the generated source file could not be written completely");

        return false;
      }

      return true;
    }

    [[nodiscard]] std::string path_for_manifest(
        const std::filesystem::path &project_dir,
        const std::filesystem::path &path)
    {
      std::error_code ec{};
      std::filesystem::path relative =
          std::filesystem::relative(path, project_dir, ec);

      if (!ec && !relative.empty())
      {
        return relative.generic_string();
      }

      return path.lexically_normal().generic_string();
    }
  }

  bool AppRunnerResult::success() const
  {
    return error.empty() &&
           !project_dir.empty() &&
           !manifest_path.empty() &&
           !cmake_source_dir.empty() &&
           !cmake_lists_path.empty() &&
           manifest.valid();
  }

  AppRunnerResult prepare_app_project(
      const std::filesystem::path &project_dir,
      const Transpiler &transpiler,
      DiagnosticBag &diagnostics)
  {
    AppRunnerResult result{};

    result.project_dir = normalize_absolute(project_dir);
    result.manifest_path = result.project_dir / "vix.app";
    result.generated_source_dir =
        result.project_dir / ".vix" / "generated" / "vixc" / "src";

    const AppManifestLoadResult load_result =
        load_app_manifest(result.manifest_path, diagnostics);

    if (!load_result.success())
    {
      result.error = load_result.error.empty()
                         ? "failed to load vix.app"
                         : load_result.error;

      return result;
    }

    AppManifest prepared_manifest = load_result.manifest;
    prepared_manifest.sources.clear();

    for (const std::string &source_entry : load_result.manifest.sources)
    {
      const std::filesystem::path source_path(source_entry);
      const std::filesystem::path absolute_source =
          resolve_project_path(result.project_dir, source_path);

      if (is_vix_source(source_path))
      {
        auto source_file = SourceFile::load(absolute_source, diagnostics);

        if (!source_file.has_value())
        {
          result.error = "failed to load VixC source file";
          return result;
        }

        const TranspileResult transpiled =
            transpiler.transpile(*source_file, diagnostics);

        if (diagnostics.has_errors())
        {
          result.error = "failed to transpile VixC source file";
          return result;
        }

        const std::filesystem::path generated_file =
            make_generated_source_path(
                result.project_dir,
                absolute_source);

        if (!write_text_file(generated_file, transpiled.code, diagnostics))
        {
          result.error = "failed to write generated C++ source file";
          return result;
        }

        result.generated_files.push_back(generated_file);

        prepared_manifest.sources.push_back(
            path_for_manifest(result.project_dir, generated_file));

        continue;
      }

      if (is_cpp_source(source_path))
      {
        prepared_manifest.sources.push_back(source_entry);
        continue;
      }

      diagnostics.error(
          "unsupported source file in vix.app",
          source_entry,
          {},
          "VixC app sources must use .vix, .cpp, .cc, .cxx, or .c++");

      result.error = "unsupported source file in vix.app";
      return result;
    }

    result.manifest = prepared_manifest;

    const AppCMakeGenerateResult cmake_result =
        generate_app_cmake_project(
            result.manifest,
            result.project_dir,
            diagnostics);

    if (!cmake_result.success())
    {
      result.error = cmake_result.error.empty()
                         ? "failed to generate internal CMake project"
                         : cmake_result.error;

      return result;
    }

    result.cmake_source_dir = cmake_result.source_dir;
    result.cmake_lists_path = cmake_result.cmake_lists_path;

    return result;
  }
}
