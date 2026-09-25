/**
 *
 *  @file SourceManager.cpp
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

#include "SourceManager.hpp"

#include "SourceFile.hpp"

#include <memory>
#include <utility>

namespace vixc::source
{

  SourceManager::~SourceManager() = default;

  SourceId
  SourceManager::add_source(std::string path, std::string contents)
  {
    const SourceId source_id = sources_.size();

    sources_.push_back(
        std::make_unique<SourceFile>(
            std::move(path),
            std::move(contents)));

    return source_id;
  }

  const SourceFile *
  SourceManager::get(SourceId source_id) const noexcept
  {
    if (source_id >= sources_.size())
      return nullptr;

    return sources_[source_id].get();
  }

  bool SourceManager::contains(SourceId source_id) const noexcept
  {
    return source_id < sources_.size();
  }

  std::size_t SourceManager::source_count() const noexcept
  {
    return sources_.size();
  }

  bool SourceManager::empty() const noexcept
  {
    return sources_.empty();
  }

} // namespace vixc::source
