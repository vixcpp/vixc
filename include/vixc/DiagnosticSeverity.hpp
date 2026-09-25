/**
 *
 *  @file DiagnosticSeverity.hpp
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

#if !defined(VIXC_DIAGNOSTIC_SEVERITY_HPP)
#define VIXC_DIAGNOSTIC_SEVERITY_HPP

#include <string_view>

namespace vixc
{
  /**
   * @brief Describes the severity of a frontend diagnostic.
   *
   * DiagnosticSeverity classifies diagnostics independently from how they are
   * eventually presented to a user. The frontend produces structured
   * diagnostics and leaves formatting, colors, terminal output, editor
   * integration, and other presentation decisions to the caller.
   *
   * Severity also communicates whether processing may continue. A Note adds
   * contextual information, a Warning reports a suspicious but accepted
   * construct, an Error reports an invalid program, and Fatal reports a
   * condition that prevents the current frontend operation from continuing
   * meaningfully.
   */
  enum class DiagnosticSeverity
  {
    /**
     * @brief Additional information associated with another diagnostic or
     * frontend event.
     *
     * Notes do not make a program invalid by themselves.
     */
    Note,

    /**
     * @brief Reports a suspicious construct that does not invalidate the
     * program.
     *
     * Warnings allow frontend processing to continue and do not by themselves
     * prevent successful compilation.
     */
    Warning,

    /**
     * @brief Reports a source or semantic condition that makes the program
     * invalid.
     *
     * The frontend may continue after an error in order to discover additional
     * diagnostics, but successful processing cannot be reported while errors
     * remain.
     */
    Error,

    /**
     * @brief Reports a condition that prevents meaningful continuation of the
     * current frontend operation.
     *
     * Fatal diagnostics are reserved for failures where continuing analysis
     * would no longer produce reliable results.
     */
    Fatal
  };

  /**
   * @brief Returns the stable textual name of a diagnostic severity.
   *
   * The returned name is intended for diagnostics, tests, logging, and
   * presentation layers that need a simple textual representation.
   *
   * @param severity Severity to convert.
   *
   * @return Lowercase textual name of the severity.
   */
  [[nodiscard]]
  constexpr std::string_view
  diagnostic_severity_name(DiagnosticSeverity severity) noexcept
  {
    switch (severity)
    {
    case DiagnosticSeverity::Note:
      return "note";

    case DiagnosticSeverity::Warning:
      return "warning";

    case DiagnosticSeverity::Error:
      return "error";

    case DiagnosticSeverity::Fatal:
      return "fatal";
    }

    return "unknown";
  }

  /**
   * @brief Reports whether a severity represents an invalid program or
   * unrecoverable frontend condition.
   *
   * Error and Fatal severities are considered failures. Note and Warning are
   * informational and do not by themselves make the frontend result
   * unsuccessful.
   *
   * @param severity Severity to inspect.
   *
   * @return true for Error and Fatal, otherwise false.
   */
  [[nodiscard]]
  constexpr bool
  diagnostic_is_error(DiagnosticSeverity severity) noexcept
  {
    return severity == DiagnosticSeverity::Error || severity == DiagnosticSeverity::Fatal;
  }

  /**
   * @brief Reports whether a severity represents a fatal frontend condition.
   *
   * @param severity Severity to inspect.
   *
   * @return true only when severity is DiagnosticSeverity::Fatal.
   */
  [[nodiscard]]
  constexpr bool
  diagnostic_is_fatal(DiagnosticSeverity severity) noexcept
  {
    return severity == DiagnosticSeverity::Fatal;
  }

} // namespace vixc

#endif // VIXC_DIAGNOSTIC_SEVERITY_HPP
