/**
 *
 *  @file LoweringContext.cpp
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

#include "LoweringContext.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"
#include "../source/SourceFile.hpp"
#include "../source/SourceManager.hpp"

namespace vixc::lowering
{

  LoweringContext::LoweringContext(
      source::SourceManager &sources,
      diagnostics::DiagnosticEngine &diagnostics) noexcept
      : sources_(sources),
        diagnostics_(diagnostics)
  {
  }

  source::SourceManager &
  LoweringContext::sources() noexcept
  {
    return sources_;
  }

  const source::SourceManager &
  LoweringContext::sources() const noexcept
  {
    return sources_;
  }

  diagnostics::DiagnosticEngine &
  LoweringContext::diagnostics() noexcept
  {
    return diagnostics_;
  }

  const diagnostics::DiagnosticEngine &
  LoweringContext::diagnostics() const noexcept
  {
    return diagnostics_;
  }

  std::optional<std::string_view>
  LoweringContext::source_text(
      SourceRange range) const noexcept
  {
    if (!range.valid())
      return std::nullopt;

    const source::SourceFile *file =
        sources_.get(range.source_id());

    if (file == nullptr)
      return std::nullopt;

    const std::size_t begin =
        range.begin_offset();

    const std::size_t end =
        range.end_offset();

    if (begin > file->size())
      return std::nullopt;

    if (end > file->size())
      return std::nullopt;

    if (end < begin)
      return std::nullopt;

    return file->contents().substr(
        begin,
        end - begin);
  }

  std::size_t
  LoweringContext::next_synthetic_id() noexcept
  {
    const std::size_t id =
        next_synthetic_id_;

    ++next_synthetic_id_;

    return id;
  }

  std::size_t
  LoweringContext::synthetic_count() const noexcept
  {
    return next_synthetic_id_;
  }

  bool LoweringContext::has_errors() const noexcept
  {
    return diagnostics_.has_errors();
  }

  bool LoweringContext::has_fatal() const noexcept
  {
    return diagnostics_.has_fatal();
  }

} // namespace vixc::lowering
