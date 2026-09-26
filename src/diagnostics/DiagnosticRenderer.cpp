/**
 *
 *  @file DiagnosticRenderer.cpp
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

#include <vixc/DiagnosticRenderer.hpp>

#include "../source/SourceFile.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace vixc
{

  namespace
  {

    void append_line(
        std::string &output,
        std::size_t line_number,
        std::string_view text)
    {
      output += std::to_string(line_number);
      output += " | ";
      output += text;
      output += '\n';
    }

    std::string underline_prefix(
        std::string_view line,
        std::size_t column_offset)
    {
      std::string prefix;
      prefix.reserve(column_offset);

      for (std::size_t index = 0;
           index < column_offset;
           ++index)
      {
        prefix += line[index] == '\t'
            ? '\t'
            : ' ';
      }

      return prefix;
    }

    std::size_t underline_width(
        SourceRange range,
        std::size_t line_start,
        std::string_view line)
    {
      const std::size_t begin = range.begin_offset();
      const std::size_t line_end = line_start + line.size();

      if (range.end_offset() <= begin)
        return 1;

      const std::size_t end =
          std::min(
              range.end_offset(),
              line_end);

      if (end <= begin)
        return 1;

      return end - begin;
    }

  } // namespace

  std::string DiagnosticRenderer::render(
      const Diagnostic &diagnostic,
      std::string_view source_name,
      std::string_view source_text) const
  {
    std::string output;

    output += diagnostic_severity_name(diagnostic.severity());

    if (diagnostic.has_code())
    {
      output += " [";
      output += diagnostic.code();
      output += ']';
    }

    output += ": ";
    output += diagnostic.message();
    output += '\n';

    const SourceRange range = diagnostic.range();
    if (range.valid() && range.begin_offset() <= source_text.size())
    {
      source::SourceFile source_file{
          std::string{source_name},
          std::string{source_text}};

      const std::optional<std::size_t> line_index =
          source_file.line_index_for_offset(
              range.begin_offset());

      if (line_index.has_value())
      {
        const std::optional<std::size_t> line_start =
            source_file.line_start_offset(*line_index);
        const std::optional<std::string_view> line =
            source_file.line(*line_index);

        if (line_start.has_value() && line.has_value())
        {
          const std::size_t line_number = *line_index + 1;
          const std::size_t column_offset =
              range.begin_offset() - *line_start;
          const std::size_t underline_offset =
              std::min(
                  column_offset,
                  line->size());

          output += "--> ";
          output += source_file.path();
          output += ':';
          output += std::to_string(line_number);
          output += ':';
          output += std::to_string(column_offset + 1);
          output += '\n';

          if (*line_index > 0)
          {
            const std::optional<std::string_view> previous =
                source_file.line(*line_index - 1);
            if (previous.has_value())
              append_line(output, *line_index, *previous);
          }

          append_line(output, line_number, *line);

          output += "  | ";
          output += underline_prefix(*line, underline_offset);
          output.append(
              underline_width(
                  range,
                  *line_start,
                  *line),
              '^');
          output += '\n';

          if (*line_index + 1 < source_file.line_count())
          {
            const std::optional<std::string_view> next =
                source_file.line(*line_index + 1);
            if (next.has_value())
              append_line(output, line_number + 1, *next);
          }

          output += "  |\n";
        }
      }
    }

    if (diagnostic.has_hint())
    {
      output += "hint: ";
      output += diagnostic.hint();
      output += '\n';
    }

    return output;
  }

} // namespace vixc
