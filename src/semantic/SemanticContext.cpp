/**
 *
 *  @file SemanticContext.cpp
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

#include "SemanticContext.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"
#include "../source/SourceFile.hpp"
#include "../source/SourceManager.hpp"

#include <utility>

namespace vixc::semantic
{

  SemanticContext::SemanticContext(
      source::SourceManager &sources,
      diagnostics::DiagnosticEngine &diagnostics) noexcept
      : sources_(sources),
        diagnostics_(diagnostics)
  {
  }

  source::SourceManager &
  SemanticContext::sources() noexcept
  {
    return sources_;
  }

  const source::SourceManager &
  SemanticContext::sources() const noexcept
  {
    return sources_;
  }

  diagnostics::DiagnosticEngine &
  SemanticContext::diagnostics() noexcept
  {
    return diagnostics_;
  }

  const diagnostics::DiagnosticEngine &
  SemanticContext::diagnostics() const noexcept
  {
    return diagnostics_;
  }

  std::optional<std::string_view>
  SemanticContext::source_text(SourceRange range) const noexcept
  {
    if (!range.valid())
      return std::nullopt;

    const source::SourceFile *file =
        sources_.get(range.source_id());

    if (file == nullptr)
      return std::nullopt;

    const std::size_t begin = range.begin_offset();
    const std::size_t end = range.end_offset();

    if (begin > file->size())
      return std::nullopt;

    if (end > file->size())
      return std::nullopt;

    if (end < begin)
      return std::nullopt;

    const std::string_view contents =
        file->contents();

    return contents.substr(
        begin,
        end - begin);
  }

  void SemanticContext::push_failure_context(
      FailureContext context)
  {
    failure_contexts_.push_back(
        std::move(context));
  }

  void SemanticContext::pop_failure_context() noexcept
  {
    if (failure_contexts_.empty())
      return;

    failure_contexts_.pop_back();
  }

  bool SemanticContext::has_failure_context() const noexcept
  {
    return !failure_contexts_.empty();
  }

  const FailureContext *
  SemanticContext::current_failure_context() const noexcept
  {
    if (failure_contexts_.empty())
      return nullptr;

    return &failure_contexts_.back();
  }

  std::size_t
  SemanticContext::failure_context_depth() const noexcept
  {
    return failure_contexts_.size();
  }

  void SemanticContext::clear_failure_contexts() noexcept
  {
    failure_contexts_.clear();
  }

} // namespace vixc::semantic
