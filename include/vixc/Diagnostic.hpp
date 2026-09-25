/**
 *
 *  @file Diagnostic.hpp
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

#if !defined(VIXC_DIAGNOSTIC_HPP)
#define VIXC_DIAGNOSTIC_HPP

#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/SourceRange.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace vixc
{
  /**
   * @brief Structured diagnostic produced by the VixC frontend.
   *
   * Diagnostic represents one frontend message independently from how that
   * message is displayed. It records the severity, human-readable message,
   * optional stable diagnostic code, and the primary source range associated
   * with the problem.
   *
   * Diagnostics may originate from syntax analysis, semantic analysis,
   * lowering, backend processing, or other frontend stages. They are returned
   * as data so callers can render them in terminals, editors, IDEs, tests, or
   * other integrations without coupling frontend logic to a presentation
   * system.
   *
   * A diagnostic may have no source range. This is useful for failures that
   * concern the frontend operation as a whole rather than a specific region of
   * source text. In that case range().valid() returns false.
   *
   * The diagnostic code is optional. When present, it should identify the
   * diagnostic category independently from its human-readable wording. This
   * allows tests and integrations to rely on a stable identifier without
   * parsing diagnostic messages.
   */
  class Diagnostic final
  {
  public:
    /**
     * @brief Creates a diagnostic without a source range or diagnostic code.
     *
     * @param severity Severity of the diagnostic.
     * @param message Human-readable diagnostic message.
     */
    Diagnostic(
        DiagnosticSeverity severity,
        std::string message)
        : severity_(severity),
          message_(std::move(message))
    {
    }

    /**
     * @brief Creates a diagnostic associated with a source range.
     *
     * The supplied range may be invalid when the diagnostic cannot be tied to
     * a concrete source region.
     *
     * @param severity Severity of the diagnostic.
     * @param message Human-readable diagnostic message.
     * @param range Primary source range associated with the diagnostic.
     */
    Diagnostic(
        DiagnosticSeverity severity,
        std::string message,
        SourceRange range)
        : severity_(severity),
          message_(std::move(message)),
          range_(range)
    {
    }

    /**
     * @brief Creates a diagnostic with a stable diagnostic code and source
     * range.
     *
     * Diagnostic codes are intended to identify classes of diagnostics across
     * changes to their human-readable wording.
     *
     * An empty code is allowed and is treated as the absence of a diagnostic
     * code.
     *
     * @param severity Severity of the diagnostic.
     * @param code Stable diagnostic identifier.
     * @param message Human-readable diagnostic message.
     * @param range Primary source range associated with the diagnostic.
     */
    Diagnostic(
        DiagnosticSeverity severity,
        std::string code,
        std::string message,
        SourceRange range)
        : severity_(severity),
          code_(std::move(code)),
          message_(std::move(message)),
          range_(range)
    {
    }

    /**
     * @brief Returns the severity of the diagnostic.
     *
     * @return Diagnostic severity.
     */
    [[nodiscard]]
    constexpr DiagnosticSeverity severity() const noexcept
    {
      return severity_;
    }

    /**
     * @brief Returns the stable diagnostic code.
     *
     * An empty view means that no diagnostic code was assigned.
     *
     * @return Non-owning view of the diagnostic code.
     */
    [[nodiscard]]
    std::string_view code() const noexcept
    {
      return code_;
    }

    /**
     * @brief Reports whether the diagnostic has a stable diagnostic code.
     *
     * @return true when code() is not empty, otherwise false.
     */
    [[nodiscard]]
    bool has_code() const noexcept
    {
      return !code_.empty();
    }

    /**
     * @brief Returns the human-readable diagnostic message.
     *
     * The message describes the problem itself and should not contain terminal
     * formatting, source excerpts, severity prefixes, or presentation-specific
     * decorations.
     *
     * @return Non-owning view of the diagnostic message.
     */
    [[nodiscard]]
    std::string_view message() const noexcept
    {
      return message_;
    }

    /**
     * @brief Returns the primary source range associated with the diagnostic.
     *
     * A diagnostic may legitimately contain an invalid range when the failure
     * cannot be associated with a specific source location.
     *
     * @return Primary source range.
     */
    [[nodiscard]]
    constexpr SourceRange range() const noexcept
    {
      return range_;
    }

    /**
     * @brief Reports whether the diagnostic is associated with source text.
     *
     * @return true when the primary source range is structurally valid,
     *         otherwise false.
     */
    [[nodiscard]]
    constexpr bool has_range() const noexcept
    {
      return range_.valid();
    }

    /**
     * @brief Reports whether the diagnostic represents an error.
     *
     * Both Error and Fatal severities are considered errors.
     *
     * @return true for Error and Fatal diagnostics, otherwise false.
     */
    [[nodiscard]]
    constexpr bool is_error() const noexcept
    {
      return diagnostic_is_error(severity_);
    }

    /**
     * @brief Reports whether the diagnostic is fatal.
     *
     * @return true only for Fatal diagnostics.
     */
    [[nodiscard]]
    constexpr bool is_fatal() const noexcept
    {
      return diagnostic_is_fatal(severity_);
    }

  private:
    /// Severity assigned by the frontend stage that produced the diagnostic.
    DiagnosticSeverity severity_{DiagnosticSeverity::Error};

    /// Optional stable identifier for the diagnostic category.
    std::string code_;

    /// Human-readable description of the diagnostic.
    std::string message_;

    /// Primary source region associated with the diagnostic.
    SourceRange range_{};
  };

} // namespace vixc

#endif // VIXC_DIAGNOSTIC_HPP
