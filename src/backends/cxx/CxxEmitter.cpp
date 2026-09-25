/**
 *
 *  @file CxxEmitter.cpp
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

#include "CxxEmitter.hpp"

namespace vixc::backends::cxx
{
  void CxxEmitter::write(std::string_view text)
  {
    output_.append(
        text.data(),
        text.size());
  }

  void CxxEmitter::write(
      std::string_view text,
      SourceRange original_range)
  {
    const std::size_t begin =
        output_.size();

    write(text);

    const std::size_t end =
        output_.size();

    map(
        begin,
        end,
        original_range);
  }

  void CxxEmitter::write_indent()
  {
    constexpr std::string_view indentation = "  ";

    for (std::size_t level = 0;
         level < indentation_level_;
         ++level)
    {
      write(indentation);
    }
  }

  void CxxEmitter::newline()
  {
    output_.push_back('\n');
  }

  void CxxEmitter::write_line(
      std::string_view text)
  {
    write(text);
    newline();
  }

  void CxxEmitter::write_line(
      std::string_view text,
      SourceRange original_range)
  {
    write(
        text,
        original_range);

    newline();
  }

  void CxxEmitter::write_indented(
      std::string_view text)
  {
    write_indent();
    write(text);
  }

  void CxxEmitter::write_indented(
      std::string_view text,
      SourceRange original_range)
  {
    write_indent();

    write(
        text,
        original_range);
  }

  void CxxEmitter::write_indented_line(
      std::string_view text)
  {
    write_indented(text);
    newline();
  }

  void CxxEmitter::write_indented_line(
      std::string_view text,
      SourceRange original_range)
  {
    write_indented(
        text,
        original_range);

    newline();
  }

  void CxxEmitter::indent() noexcept
  {
    ++indentation_level_;
  }

  void CxxEmitter::dedent() noexcept
  {
    if (indentation_level_ == 0)
      return;

    --indentation_level_;
  }

  std::size_t
  CxxEmitter::indentation_level() const noexcept
  {
    return indentation_level_;
  }

  std::size_t CxxEmitter::offset() const noexcept
  {
    return output_.size();
  }

  const std::string &
  CxxEmitter::output() const noexcept
  {
    return output_;
  }

  const CxxSourceMap &
  CxxEmitter::source_map() const noexcept
  {
    return source_map_;
  }

  bool CxxEmitter::empty() const noexcept
  {
    return output_.empty();
  }

  void CxxEmitter::reset() noexcept
  {
    output_.clear();
    source_map_.clear();
    indentation_level_ = 0;
  }

  void CxxEmitter::map(
      std::size_t generated_begin,
      std::size_t generated_end,
      SourceRange original_range)
  {
    if (!original_range.valid())
      return;

    if (generated_begin == generated_end)
      return;

    source_map_.add(
        generated_begin,
        generated_end,
        original_range);
  }

} // namespace vixc::backends::cxx
