/**
 *
 *  @file CxxSourceMap.cpp
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

#include "CxxSourceMap.hpp"

#include <algorithm>

namespace vixc::backends::cxx
{

  bool CxxSourceMap::add(
      std::size_t generated_begin,
      std::size_t generated_end,
      SourceRange original_range)
  {
    return add(
        CxxSourceMapEntry{
            generated_begin,
            generated_end,
            original_range});
  }

  bool CxxSourceMap::add(
      CxxSourceMapEntry entry)
  {
    if (!entry.valid())
      return false;

    if (!entries_.empty())
    {
      const CxxSourceMapEntry &previous =
          entries_.back();

      if (
          entry.generated_begin() < previous.generated_end())
      {
        return false;
      }
    }

    entries_.push_back(entry);
    return true;
  }

  const CxxSourceMapEntry *
  CxxSourceMap::find(
      std::size_t generated_offset) const noexcept
  {
    if (entries_.empty())
      return nullptr;

    const auto it =
        std::upper_bound(
            entries_.begin(),
            entries_.end(),
            generated_offset,
            [](std::size_t offset,
               const CxxSourceMapEntry &entry)
            {
              return offset < entry.generated_begin();
            });

    if (it == entries_.begin())
      return nullptr;

    const CxxSourceMapEntry &candidate =
        *(it - 1);

    if (!candidate.contains(generated_offset))
      return nullptr;

    return &candidate;
  }

  std::optional<SourceRange>
  CxxSourceMap::original_range_for(
      std::size_t generated_offset) const noexcept
  {
    const CxxSourceMapEntry *entry =
        find(generated_offset);

    if (entry == nullptr)
      return std::nullopt;

    return entry->original_range();
  }

  const std::vector<CxxSourceMapEntry> &
  CxxSourceMap::entries() const noexcept
  {
    return entries_;
  }

  std::size_t
  CxxSourceMap::size() const noexcept
  {
    return entries_.size();
  }

  bool CxxSourceMap::empty() const noexcept
  {
    return entries_.empty();
  }

  void CxxSourceMap::clear() noexcept
  {
    entries_.clear();
  }

} // namespace vixc::backends::cxx
