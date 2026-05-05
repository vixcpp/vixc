/**
 *
 *  @file SourceFile.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vixpp
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix++
 *
 */

#include <vixpp/SourceFile.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace vixpp
{
  SourceFile::SourceFile(std::filesystem::path path, std::string content)
      : path_(std::move(path)),
        content_(std::move(content)),
        line_offsets_{}
  {
    rebuild_line_offsets();
  }

  std::optional<SourceFile> SourceFile::load(
      const std::filesystem::path &path,
      DiagnosticBag &diagnostics)
  {
    std::ifstream input(path, std::ios::binary);

    if (!input)
    {
      diagnostics.error(
          "failed to open source file",
          path.string(),
          {},
          "check that the file exists and that you have permission to read it");

      return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (input.bad())
    {
      diagnostics.error(
          "failed to read source file",
          path.string(),
          {},
          "the file could not be read completely");

      return std::nullopt;
    }

    return SourceFile(path, buffer.str());
  }

  const std::filesystem::path &SourceFile::path() const noexcept
  {
    return path_;
  }

  std::string SourceFile::path_string() const
  {
    return path_.string();
  }

  std::string_view SourceFile::content() const noexcept
  {
    return content_;
  }

  bool SourceFile::empty() const noexcept
  {
    return content_.empty();
  }

  std::size_t SourceFile::size() const noexcept
  {
    return content_.size();
  }

  SourcePosition SourceFile::position_at(std::size_t offset) const noexcept
  {
    if (line_offsets_.empty())
    {
      return SourcePosition{1, 1};
    }

    if (offset > content_.size())
    {
      offset = content_.size();
    }

    const auto it = std::upper_bound(
        line_offsets_.begin(),
        line_offsets_.end(),
        offset);

    const std::size_t line_index =
        it == line_offsets_.begin()
            ? 0
            : static_cast<std::size_t>(std::distance(line_offsets_.begin(), it) - 1);

    const std::size_t line_start = line_offsets_[line_index];

    return SourcePosition{
        line_index + 1,
        offset - line_start + 1};
  }

  SourceRange SourceFile::range_from_offsets(std::size_t begin,
                                             std::size_t end) const noexcept
  {
    if (begin > content_.size())
    {
      begin = content_.size();
    }

    if (end > content_.size())
    {
      end = content_.size();
    }

    if (end < begin)
    {
      end = begin;
    }

    return SourceRange{
        position_at(begin),
        position_at(end)};
  }

  std::vector<std::string_view> SourceFile::lines() const
  {
    std::vector<std::string_view> result{};

    if (content_.empty())
    {
      result.emplace_back(std::string_view{});
      return result;
    }

    std::size_t line_start = 0;

    while (line_start <= content_.size())
    {
      std::size_t line_end = content_.find('\n', line_start);

      if (line_end == std::string::npos)
      {
        line_end = content_.size();
      }

      std::size_t view_end = line_end;

      if (view_end > line_start && content_[view_end - 1] == '\r')
      {
        --view_end;
      }

      result.emplace_back(
          content_.data() + line_start,
          view_end - line_start);

      if (line_end == content_.size())
      {
        break;
      }

      line_start = line_end + 1;
    }

    return result;
  }

  void SourceFile::rebuild_line_offsets()
  {
    line_offsets_.clear();
    line_offsets_.push_back(0);

    for (std::size_t i = 0; i < content_.size(); ++i)
    {
      if (content_[i] == '\n')
      {
        const std::size_t next_line_offset = i + 1;

        if (next_line_offset <= content_.size())
        {
          line_offsets_.push_back(next_line_offset);
        }
      }
    }
  }
}
