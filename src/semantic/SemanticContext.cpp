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

  void SemanticContext::clear_failure_declarations() noexcept
  {
    failure_functions_.clear();
    propagation_resolutions_.clear();
  }

  ir::FailureFunctionId
  SemanticContext::register_failure_function(
      std::string name,
      SourceRange function_name_range,
      SourceRange declaration_range,
      SourceRange success_type_range,
      SourceRange declarator_range,
      SourceRange failure_type_range)
  {
    const ir::FailureFunctionId id{failure_functions_.size()};
    failure_functions_.push_back(
        FailureFunctionDeclaration{
            id,
            std::move(name),
            function_name_range,
            declaration_range,
            success_type_range,
            declarator_range,
            failure_type_range});
    return id;
  }

  FailureFunctionLookup
  SemanticContext::lookup_failure_function(
      std::string_view name) const noexcept
  {
    const FailureFunctionDeclaration *candidate = nullptr;

    for (const FailureFunctionDeclaration &function : failure_functions_)
    {
      if (function.name != name)
        continue;

      if (candidate != nullptr)
      {
        return FailureFunctionLookup{
            FailureFunctionLookupKind::Ambiguous};
      }

      candidate = &function;
    }

    if (candidate == nullptr)
      return FailureFunctionLookup{};

    return FailureFunctionLookup{
        FailureFunctionLookupKind::Resolved,
        candidate->id,
        candidate->declaration_range,
        candidate->failure_type_range};
  }

  void SemanticContext::record_failure_propagation_resolution(
      FailurePropagationResolution resolution)
  {
    for (FailurePropagationResolution &existing : propagation_resolutions_)
    {
      if (existing.try_range == resolution.try_range)
      {
        existing = resolution;
        return;
      }
    }

    propagation_resolutions_.push_back(std::move(resolution));
  }

  std::optional<FailurePropagationResolution>
  SemanticContext::failure_propagation_resolution(
      SourceRange try_range) const noexcept
  {
    for (const FailurePropagationResolution &resolution :
         propagation_resolutions_)
    {
      if (resolution.try_range == try_range)
        return resolution;
    }

    return std::nullopt;
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
