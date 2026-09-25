/**
 *
 *  @file DiagnosticEngine.cpp
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

#include "DiagnosticEngine.hpp"

#include <utility>

namespace vixc::diagnostics
{
  void DiagnosticEngine::emit(Diagnostic diagnostic)
  {
    if (diagnostic.is_error())
      ++error_count_;

    if (diagnostic.is_fatal())
      ++fatal_count_;

    diagnostics_.push_back(std::move(diagnostic));
  }

  void DiagnosticEngine::emit(
      DiagnosticSeverity severity,
      std::string message)
  {
    emit(Diagnostic{
        severity,
        std::move(message)});
  }

  void DiagnosticEngine::emit(
      DiagnosticSeverity severity,
      std::string message,
      SourceRange range)
  {
    emit(Diagnostic{
        severity,
        std::move(message),
        range});
  }

  void DiagnosticEngine::emit(
      DiagnosticSeverity severity,
      std::string code,
      std::string message,
      SourceRange range)
  {
    emit(Diagnostic{
        severity,
        std::move(code),
        std::move(message),
        range});
  }

  void DiagnosticEngine::note(
      std::string message,
      SourceRange range)
  {
    emit(
        DiagnosticSeverity::Note,
        std::move(message),
        range);
  }

  void DiagnosticEngine::warning(
      std::string message,
      SourceRange range)
  {
    emit(
        DiagnosticSeverity::Warning,
        std::move(message),
        range);
  }

  void DiagnosticEngine::error(
      std::string message,
      SourceRange range)
  {
    emit(
        DiagnosticSeverity::Error,
        std::move(message),
        range);
  }

  void DiagnosticEngine::fatal(
      std::string message,
      SourceRange range)
  {
    emit(
        DiagnosticSeverity::Fatal,
        std::move(message),
        range);
  }

  const std::vector<Diagnostic> &
  DiagnosticEngine::diagnostics() const noexcept
  {
    return diagnostics_;
  }

  std::size_t DiagnosticEngine::size() const noexcept
  {
    return diagnostics_.size();
  }

  bool DiagnosticEngine::empty() const noexcept
  {
    return diagnostics_.empty();
  }

  bool DiagnosticEngine::has_errors() const noexcept
  {
    return error_count_ != 0;
  }

  bool DiagnosticEngine::has_fatal() const noexcept
  {
    return fatal_count_ != 0;
  }

  std::size_t
  DiagnosticEngine::count(DiagnosticSeverity severity) const noexcept
  {
    std::size_t result = 0;

    for (const Diagnostic &diagnostic : diagnostics_)
    {
      if (diagnostic.severity() == severity)
        ++result;
    }

    return result;
  }

  void DiagnosticEngine::clear() noexcept
  {
    diagnostics_.clear();
    error_count_ = 0;
    fatal_count_ = 0;
  }

} // namespace vixc::diagnostics
