/**
 *
 *  @file AppManifest.cpp
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

#include <vixc/AppManifest.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace vixc
{
  namespace
  {
    [[nodiscard]] std::string trim_copy(const std::string &value)
    {
      std::size_t begin = 0;

      while (begin < value.size() &&
             std::isspace(static_cast<unsigned char>(value[begin])) != 0)
      {
        ++begin;
      }

      std::size_t end = value.size();

      while (end > begin &&
             std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
      {
        --end;
      }

      return value.substr(begin, end - begin);
    }

    [[nodiscard]] std::string lower_copy(std::string value)
    {
      for (char &c : value)
      {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
      }

      return value;
    }

    [[nodiscard]] bool starts_with(const std::string &value,
                                   const std::string &prefix)
    {
      return value.rfind(prefix, 0) == 0;
    }

    [[nodiscard]] bool ends_with(const std::string &value,
                                 const std::string &suffix)
    {
      if (suffix.size() > value.size())
      {
        return false;
      }

      return std::equal(
          suffix.rbegin(),
          suffix.rend(),
          value.rbegin());
    }

    [[nodiscard]] std::string strip_quotes(const std::string &value)
    {
      const std::string trimmed = trim_copy(value);

      if (trimmed.size() >= 2 &&
          ((trimmed.front() == '"' && trimmed.back() == '"') ||
           (trimmed.front() == '\'' && trimmed.back() == '\'')))
      {
        return trimmed.substr(1, trimmed.size() - 2);
      }

      return trimmed;
    }

    [[nodiscard]] std::string strip_inline_comment(const std::string &line)
    {
      bool in_single_quote = false;
      bool in_double_quote = false;
      bool escaped = false;

      for (std::size_t i = 0; i < line.size(); ++i)
      {
        const char c = line[i];

        if (escaped)
        {
          escaped = false;
          continue;
        }

        if (c == '\\')
        {
          escaped = true;
          continue;
        }

        if (c == '\'' && !in_double_quote)
        {
          in_single_quote = !in_single_quote;
          continue;
        }

        if (c == '"' && !in_single_quote)
        {
          in_double_quote = !in_double_quote;
          continue;
        }

        if (c == '#' && !in_single_quote && !in_double_quote)
        {
          return line.substr(0, i);
        }
      }

      return line;
    }

    [[nodiscard]] bool is_array_start(const std::string &value)
    {
      return trim_copy(value) == "[";
    }

    [[nodiscard]] bool is_array_end(const std::string &value)
    {
      return trim_copy(value) == "]";
    }

    [[nodiscard]] std::vector<std::string> parse_inline_array(
        const std::string &value)
    {
      std::vector<std::string> result{};

      std::string text = trim_copy(value);

      if (text.size() < 2 || text.front() != '[' || text.back() != ']')
      {
        return result;
      }

      text = text.substr(1, text.size() - 2);

      std::string current{};
      bool in_single_quote = false;
      bool in_double_quote = false;
      bool escaped = false;

      for (const char c : text)
      {
        if (escaped)
        {
          current.push_back(c);
          escaped = false;
          continue;
        }

        if (c == '\\')
        {
          escaped = true;
          current.push_back(c);
          continue;
        }

        if (c == '\'' && !in_double_quote)
        {
          in_single_quote = !in_single_quote;
          current.push_back(c);
          continue;
        }

        if (c == '"' && !in_single_quote)
        {
          in_double_quote = !in_double_quote;
          current.push_back(c);
          continue;
        }

        if (c == ',' && !in_single_quote && !in_double_quote)
        {
          const std::string item = strip_quotes(trim_copy(current));

          if (!item.empty())
          {
            result.push_back(item);
          }

          current.clear();
          continue;
        }

        current.push_back(c);
      }

      const std::string item = strip_quotes(trim_copy(current));

      if (!item.empty())
      {
        result.push_back(item);
      }

      return result;
    }

    bool assign_scalar(AppManifest &manifest,
                       const std::string &key,
                       const std::string &value,
                       std::string &error)
    {
      const std::string normalized_key = lower_copy(trim_copy(key));
      const std::string normalized_value = strip_quotes(value);

      if (normalized_key == "name")
      {
        manifest.name = normalized_value;
        return true;
      }

      if (normalized_key == "type")
      {
        const auto parsed = app_target_type_from_string(normalized_value);

        if (!parsed.has_value())
        {
          error = "Invalid vix.app target type: " + normalized_value;
          return false;
        }

        manifest.type = *parsed;
        return true;
      }

      if (normalized_key == "standard")
      {
        manifest.standard = normalized_value;
        return true;
      }

      error = "Unknown scalar field in vix.app: " + key;
      return false;
    }

    bool assign_array(AppManifest &manifest,
                      const std::string &key,
                      const std::vector<std::string> &values,
                      std::string &error)
    {
      const std::string normalized_key = lower_copy(trim_copy(key));

      if (normalized_key == "sources")
      {
        manifest.sources = values;
        return true;
      }

      if (normalized_key == "include_dirs" ||
          normalized_key == "includedirs" ||
          normalized_key == "includes")
      {
        manifest.include_dirs = values;
        return true;
      }

      if (normalized_key == "defines")
      {
        manifest.defines = values;
        return true;
      }

      if (normalized_key == "links" ||
          normalized_key == "libraries" ||
          normalized_key == "libs")
      {
        manifest.links = values;
        return true;
      }

      error = "Unknown array field in vix.app: " + key;
      return false;
    }

    bool parse_manifest_text(const std::string &text,
                             AppManifest &manifest,
                             std::string &error)
    {
      std::istringstream input(text);
      std::string line{};

      std::string active_array_key{};
      std::vector<std::string> active_array_values{};

      std::size_t line_number = 0;

      while (std::getline(input, line))
      {
        ++line_number;

        line = trim_copy(strip_inline_comment(line));

        if (line.empty())
        {
          continue;
        }

        if (!active_array_key.empty())
        {
          if (is_array_end(line))
          {
            if (!assign_array(
                    manifest,
                    active_array_key,
                    active_array_values,
                    error))
            {
              return false;
            }

            active_array_key.clear();
            active_array_values.clear();
            continue;
          }

          if (ends_with(line, ","))
          {
            line.pop_back();
          }

          const std::string item = strip_quotes(trim_copy(line));

          if (!item.empty())
          {
            active_array_values.push_back(item);
          }

          continue;
        }

        const auto position = line.find('=');

        if (position == std::string::npos)
        {
          error = "Invalid vix.app syntax at line " +
                  std::to_string(line_number) +
                  ": expected key = value";
          return false;
        }

        const std::string key = trim_copy(line.substr(0, position));
        const std::string value = trim_copy(line.substr(position + 1));

        if (key.empty())
        {
          error = "Invalid vix.app syntax at line " +
                  std::to_string(line_number) +
                  ": empty key";
          return false;
        }

        if (is_array_start(value))
        {
          active_array_key = key;
          active_array_values.clear();
          continue;
        }

        if (starts_with(value, "["))
        {
          if (!assign_array(
                  manifest,
                  key,
                  parse_inline_array(value),
                  error))
          {
            return false;
          }

          continue;
        }

        if (!assign_scalar(manifest, key, value, error))
        {
          return false;
        }
      }

      if (!active_array_key.empty())
      {
        error = "Invalid vix.app syntax: missing closing ] for field " +
                active_array_key;
        return false;
      }

      if (manifest.name.empty())
      {
        error = "Invalid vix.app: missing required field 'name'";
        return false;
      }

      if (manifest.sources.empty())
      {
        error = "Invalid vix.app: missing required field 'sources'";
        return false;
      }

      if (manifest.standard.empty())
      {
        manifest.standard = "c++20";
      }

      return true;
    }

    [[nodiscard]] std::string read_text_file_or_empty(
        const std::filesystem::path &path)
    {
      std::ifstream input(path, std::ios::binary);

      if (!input)
      {
        return {};
      }

      std::ostringstream output;
      output << input.rdbuf();

      return output.str();
    }
  }

  std::string to_string(AppTargetType type)
  {
    switch (type)
    {
    case AppTargetType::Executable:
      return "executable";
    case AppTargetType::StaticLibrary:
      return "static-library";
    case AppTargetType::SharedLibrary:
      return "shared-library";
    }

    return "executable";
  }

  std::optional<AppTargetType> app_target_type_from_string(
      const std::string &value)
  {
    const std::string type = lower_copy(trim_copy(value));

    if (type == "executable")
    {
      return AppTargetType::Executable;
    }

    if (type == "static" ||
        type == "static-library")
    {
      return AppTargetType::StaticLibrary;
    }

    if (type == "shared" ||
        type == "shared-library")
    {
      return AppTargetType::SharedLibrary;
    }

    if (type == "library")
    {
      return AppTargetType::StaticLibrary;
    }

    return std::nullopt;
  }

  bool AppManifest::valid() const
  {
    return !name.empty() && !sources.empty();
  }

  bool AppManifestLoadResult::success() const
  {
    return error.empty() && manifest.valid();
  }

  AppManifestLoadResult load_app_manifest(
      const std::filesystem::path &path,
      DiagnosticBag &diagnostics)
  {
    AppManifestLoadResult result{};

    const std::string text = read_text_file_or_empty(path);

    if (text.empty())
    {
      result.error = "failed to read vix.app";

      diagnostics.error(
          result.error,
          path.string(),
          {},
          "check that the vix.app file exists and is readable");

      return result;
    }

    std::string error{};

    if (!parse_manifest_text(text, result.manifest, error))
    {
      result.error = error;

      diagnostics.error(
          error,
          path.string(),
          {},
          "check the vix.app syntax and required fields");

      return result;
    }

    return result;
  }
}
