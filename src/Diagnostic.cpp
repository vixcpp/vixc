/**
 *
 *  @file Diagnostic.cpp
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

#include <vixpp/Diagnostic.hpp>

#include <algorithm>
#include <ostream>
#include <utility>

namespace vixpp
{
  bool SourcePosition::valid() const noexcept
  {
    return line > 0 && column > 0;
  }

  bool SourceRange::valid() const noexcept
  {
    return begin.valid();
  }

  bool Diagnostic::has_file() const noexcept
  {
    return !file.empty();
  }

  bool Diagnostic::has_location() const noexcept
  {
    return range.valid();
  }

  void DiagnosticBag::add(Diagnostic diagnostic)
  {
    diagnostics_.push_back(std::move(diagnostic));
  }

  void DiagnosticBag::error(std::string message,
                            std::string file,
                            SourceRange range,
                            std::optional<std::string> hint)
  {
    add(Diagnostic{
        DiagnosticSeverity::Error,
        std::move(message),
        std::move(file),
        range,
        std::move(hint)});
  }

  void DiagnosticBag::warning(std::string message,
                              std::string file,
                              SourceRange range,
                              std::optional<std::string> hint)
  {
    add(Diagnostic{
        DiagnosticSeverity::Warning,
        std::move(message),
        std::move(file),
        range,
        std::move(hint)});
  }

  void DiagnosticBag::note(std::string message,
                           std::string file,
                           SourceRange range,
                           std::optional<std::string> hint)
  {
    add(Diagnostic{
        DiagnosticSeverity::Note,
        std::move(message),
        std::move(file),
        range,
        std::move(hint)});
  }

  bool DiagnosticBag::has_errors() const noexcept
  {
    return std::any_of(
        diagnostics_.begin(),
        diagnostics_.end(),
        [](const Diagnostic &diagnostic)
        {
          return diagnostic.severity == DiagnosticSeverity::Error;
        });
  }

  bool DiagnosticBag::empty() const noexcept
  {
    return diagnostics_.empty();
  }

  std::size_t DiagnosticBag::size() const noexcept
  {
    return diagnostics_.size();
  }

  const std::vector<Diagnostic> &DiagnosticBag::items() const noexcept
  {
    return diagnostics_;
  }

  void DiagnosticBag::clear() noexcept
  {
    diagnostics_.clear();
  }

  std::string_view to_string(DiagnosticSeverity severity) noexcept
  {
    switch (severity)
    {
    case DiagnosticSeverity::Note:
      return "note";
    case DiagnosticSeverity::Warning:
      return "warning";
    case DiagnosticSeverity::Error:
      return "error";
    }

    return "error";
  }

  void print_diagnostic(std::ostream &out, const Diagnostic &diagnostic)
  {
    if (diagnostic.has_file())
    {
      out << diagnostic.file;

      if (diagnostic.has_location())
      {
        out << ':'
            << diagnostic.range.begin.line
            << ':'
            << diagnostic.range.begin.column;
      }

      out << ": ";
    }

    out << to_string(diagnostic.severity)
        << ": "
        << diagnostic.message
        << '\n';

    if (diagnostic.hint.has_value() && !diagnostic.hint->empty())
    {
      out << "hint: " << *diagnostic.hint << '\n';
    }
  }

  void print_diagnostics(std::ostream &out, const DiagnosticBag &bag)
  {
    for (const auto &diagnostic : bag.items())
    {
      print_diagnostic(out, diagnostic);
    }
  }
}
