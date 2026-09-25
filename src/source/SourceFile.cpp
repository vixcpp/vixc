/**
 *
 *  @file SourceFile.cpp
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

#include "SourceFile.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace vixc::source
{

  SourceFile::SourceFile(std::string path, std::string contents)
      : path_(std::move(path)),
        contents_(std::move(contents))
  {
    build_line_index();
  }

  std::string_view SourceFile::path() const noexcept
  {
    return path_;
  }

  std::string_view SourceFile::contents() const noexcept
  {
    return contents_;
  }

  std::size_t SourceFile::size() const noexcept
  {
    return contents_.size();
  }

  bool SourceFile::empty() const noexcept
  {
    return contents_.empty();
  }

  std::size_t SourceFile::line_count() const noexcept
  {
    return line_starts_.size();
  }

  std::optional<std::size_t>
  SourceFile::line_index_for_offset(std::size_t offset) const noexcept
  {
    if (offset > contents_.size())
      return std::nullopt;

    const auto it =
        std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);

    if (it == line_starts_.begin())
      return std::size_t{0};

    return static_cast<std::size_t>(
        std::distance(line_starts_.begin(), it) - 1);
  }

  std::optional<std::size_t>
  SourceFile::line_start_offset(std::size_t line_index) const noexcept
  {
    if (line_index >= line_starts_.size())
      return std::nullopt;

    return line_starts_[line_index];
  }

  std::optional<std::string_view>
  SourceFile::line(std::size_t line_index) const noexcept
  {
    if (line_index >= line_starts_.size())
      return std::nullopt;

    const std::size_t start = line_starts_[line_index];

    std::size_t end =
        line_index + 1 < line_starts_.size()
            ? line_starts_[line_index + 1]
            : contents_.size();

    if (end > start && contents_[end - 1] == '\n')
      --end;

    if (end > start && contents_[end - 1] == '\r')
      --end;

    return std::string_view{
        contents_.data() + start,
        end - start};
  }

  void SourceFile::build_line_index()
  {
    line_starts_.clear();
    line_starts_.push_back(0);

    for (std::size_t offset = 0; offset < contents_.size(); ++offset)
    {
      if (contents_[offset] == '\n')
        line_starts_.push_back(offset + 1);
    }
  }

} // namespace vixc::source
