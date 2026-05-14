/**
 *
 *  @file AppCMakeGenerator.cpp
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

#include <vixc/AppCMakeGenerator.hpp>

#include <fstream>
#include <sstream>
#include <system_error>

namespace vixc
{
  namespace
  {
    [[nodiscard]] std::string cmake_quote(const std::string &value)
    {
      std::string result{};
      result.reserve(value.size() + 8);
      result.push_back('"');

      for (const char c : value)
      {
        if (c == '\\')
        {
          result += "\\\\";
        }
        else if (c == '"')
        {
          result += "\\\"";
        }
        else
        {
          result.push_back(c);
        }
      }

      result.push_back('"');

      return result;
    }

    [[nodiscard]] bool needs_vix_package(const std::vector<std::string> &links)
    {
      for (const std::string &link : links)
      {
        if (link.rfind("vix::", 0) == 0)
        {
          return true;
        }
      }

      return false;
    }

    [[nodiscard]] std::string cmake_path(const std::filesystem::path &path)
    {
      return path.lexically_normal().generic_string();
    }

    [[nodiscard]] std::string cmake_quoted_path(
        const std::filesystem::path &path)
    {
      return cmake_quote(cmake_path(path));
    }

    [[nodiscard]] std::filesystem::path absolute_project_path(
        const std::filesystem::path &project_dir,
        const std::string &relative_path)
    {
      const std::filesystem::path path(relative_path);

      if (path.is_absolute())
      {
        return path.lexically_normal();
      }

      return (project_dir / path).lexically_normal();
    }

    [[nodiscard]] int cpp_standard_number(const std::string &standard)
    {
      if (standard == "c++11" || standard == "cpp11" || standard == "11")
      {
        return 11;
      }

      if (standard == "c++14" || standard == "cpp14" || standard == "14")
      {
        return 14;
      }

      if (standard == "c++17" || standard == "cpp17" || standard == "17")
      {
        return 17;
      }

      if (standard == "c++20" || standard == "cpp20" || standard == "20")
      {
        return 20;
      }

      if (standard == "c++23" || standard == "cpp23" || standard == "23")
      {
        return 23;
      }

      if (standard == "c++26" || standard == "cpp26" || standard == "26")
      {
        return 26;
      }

      return 20;
    }

    [[nodiscard]] bool write_text_file_atomic(
        const std::filesystem::path &path,
        const std::string &content)
    {
      const std::filesystem::path parent = path.parent_path();

      if (!parent.empty())
      {
        std::error_code ec{};
        std::filesystem::create_directories(parent, ec);

        if (ec)
        {
          return false;
        }
      }

      const std::filesystem::path temporary_path = path.string() + ".tmp";

      {
        std::ofstream output(
            temporary_path,
            std::ios::binary | std::ios::trunc);

        if (!output)
        {
          return false;
        }

        output << content;

        if (!output)
        {
          return false;
        }
      }

      std::error_code ec{};
      std::filesystem::rename(temporary_path, path, ec);

      if (!ec)
      {
        return true;
      }

      std::filesystem::remove(path, ec);
      ec.clear();

      std::filesystem::rename(temporary_path, path, ec);

      return !ec;
    }

    [[nodiscard]] std::string cmake_target_command(AppTargetType type)
    {
      switch (type)
      {
      case AppTargetType::Executable:
        return "add_executable";
      case AppTargetType::StaticLibrary:
        return "add_library";
      case AppTargetType::SharedLibrary:
        return "add_library";
      }

      return "add_executable";
    }

    [[nodiscard]] std::string cmake_library_kind(AppTargetType type)
    {
      switch (type)
      {
      case AppTargetType::StaticLibrary:
        return " STATIC";
      case AppTargetType::SharedLibrary:
        return " SHARED";
      case AppTargetType::Executable:
        return "";
      }

      return "";
    }

    void write_cmake_list(std::ostringstream &output,
                          const std::string &command,
                          const std::string &target_name,
                          const std::vector<std::string> &values)
    {
      if (values.empty())
      {
        return;
      }

      output << command << "(" << target_name << " PRIVATE\n";

      for (const std::string &value : values)
      {
        output << "  " << cmake_quote(value) << "\n";
      }

      output << ")\n\n";
    }

    void write_cmake_path_list(std::ostringstream &output,
                               const std::string &command,
                               const std::string &target_name,
                               const std::filesystem::path &project_dir,
                               const std::vector<std::string> &values)
    {
      if (values.empty())
      {
        return;
      }

      output << command << "(" << target_name << " PRIVATE\n";

      for (const std::string &value : values)
      {
        const std::filesystem::path resolved =
            absolute_project_path(project_dir, value);

        output << "  " << cmake_quoted_path(resolved) << "\n";
      }

      output << ")\n\n";
    }
  }

  bool AppCMakeGenerateResult::success() const
  {
    return error.empty() &&
           !source_dir.empty() &&
           !cmake_lists_path.empty();
  }

  std::string generate_app_cmake_lists_content(
      const AppManifest &manifest,
      const std::filesystem::path &project_dir)
  {
    const std::string target_name = manifest.name;
    const int standard = cpp_standard_number(manifest.standard);

    std::ostringstream output{};

    output << "# Auto-generated by VixC from vix.app\n";
    output << "# Do not edit this file directly.\n\n";

    output << "cmake_minimum_required(VERSION 3.24)\n\n";

    output << "project("
           << target_name
           << " LANGUAGES CXX)\n\n";

    output << "set(CMAKE_CXX_STANDARD "
           << standard
           << ")\n";

    output << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
    output << "set(CMAKE_CXX_EXTENSIONS OFF)\n\n";

    if (needs_vix_package(manifest.links))
    {
      output << "find_package(vix CONFIG REQUIRED)\n\n";
    }

    output << cmake_target_command(manifest.type)
           << "("
           << target_name
           << cmake_library_kind(manifest.type)
           << "\n";

    for (const std::string &source : manifest.sources)
    {
      const std::filesystem::path resolved =
          absolute_project_path(project_dir, source);

      output << "  " << cmake_quoted_path(resolved) << "\n";
    }

    output << ")\n\n";

    write_cmake_path_list(
        output,
        "target_include_directories",
        target_name,
        project_dir,
        manifest.include_dirs);

    write_cmake_list(
        output,
        "target_compile_definitions",
        target_name,
        manifest.defines);

    write_cmake_list(
        output,
        "target_link_libraries",
        target_name,
        manifest.links);

    return output.str();
  }

  AppCMakeGenerateResult generate_app_cmake_project(
      const AppManifest &manifest,
      const std::filesystem::path &project_dir,
      DiagnosticBag &diagnostics)
  {
    AppCMakeGenerateResult result{};

    if (!manifest.valid())
    {
      result.error = "invalid vix.app manifest";

      diagnostics.error(
          result.error,
          {},
          {},
          "vix.app must contain at least name and sources");

      return result;
    }

    std::error_code ec{};
    const std::filesystem::path normalized_project_dir =
        std::filesystem::absolute(project_dir, ec).lexically_normal();

    if (ec)
    {
      result.error = "failed to resolve project directory";

      diagnostics.error(
          result.error,
          project_dir.string(),
          {},
          ec.message());

      return result;
    }

    result.source_dir =
        normalized_project_dir / ".vix" / "generated" / "vixc" / manifest.name;

    result.cmake_lists_path = result.source_dir / "CMakeLists.txt";

    const std::string content =
        generate_app_cmake_lists_content(
            manifest,
            normalized_project_dir);

    if (!write_text_file_atomic(result.cmake_lists_path, content))
    {
      result.error =
          "failed to write generated CMakeLists.txt: " +
          result.cmake_lists_path.string();

      diagnostics.error(
          result.error,
          result.cmake_lists_path.string(),
          {},
          "check that the generated VixC directory is writable");

      return result;
    }

    return result;
  }
}
