/**
 *
 *  @file Diagnostic.hpp
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
#ifndef VIXPP_DIAGNOSTIC_HPP
#define VIXPP_DIAGNOSTIC_HPP

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vixpp
{
  /**
   * @brief Severity level of a Vix++ diagnostic message.
   */
  enum class DiagnosticSeverity
  {
    Note,
    Warning,
    Error
  };

  /**
   * @brief Source position used by diagnostics.
   *
   * Lines and columns are one-based.
   * A value of 0 means that the position is unknown.
   */
  struct SourcePosition
  {
    std::size_t line{0};
    std::size_t column{0};

    /**
     * @brief Checks whether this position points to a known location.
     *
     * @return True if both line and column are greater than zero.
     */
    [[nodiscard]] bool valid() const noexcept;
  };

  /**
   * @brief Source range used by diagnostics.
   */
  struct SourceRange
  {
    SourcePosition begin{};
    SourcePosition end{};

    /**
     * @brief Checks whether this range has a valid begin position.
     *
     * @return True if the begin position is valid.
     */
    [[nodiscard]] bool valid() const noexcept;
  };

  /**
   * @brief A single diagnostic emitted by Vix++.
   */
  struct Diagnostic
  {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string message{};
    std::string file{};
    SourceRange range{};
    std::optional<std::string> hint{};

    /**
     * @brief Checks whether this diagnostic has a source file attached.
     *
     * @return True if the file path is not empty.
     */
    [[nodiscard]] bool has_file() const noexcept;

    /**
     * @brief Checks whether this diagnostic has a source location attached.
     *
     * @return True if the diagnostic range is valid.
     */
    [[nodiscard]] bool has_location() const noexcept;
  };

  /**
   * @brief Collects diagnostics produced during parsing, transpilation, or execution.
   */
  class DiagnosticBag
  {
  public:
    /**
     * @brief Adds a diagnostic to the bag.
     *
     * @param diagnostic The diagnostic to store.
     */
    void add(Diagnostic diagnostic);

    /**
     * @brief Adds an error diagnostic.
     *
     * @param message Human-readable error message.
     * @param file Optional source file path.
     * @param range Optional source range.
     * @param hint Optional extra guidance.
     */
    void error(std::string message,
               std::string file = {},
               SourceRange range = {},
               std::optional<std::string> hint = std::nullopt);

    /**
     * @brief Adds a warning diagnostic.
     *
     * @param message Human-readable warning message.
     * @param file Optional source file path.
     * @param range Optional source range.
     * @param hint Optional extra guidance.
     */
    void warning(std::string message,
                 std::string file = {},
                 SourceRange range = {},
                 std::optional<std::string> hint = std::nullopt);

    /**
     * @brief Adds a note diagnostic.
     *
     * @param message Human-readable note message.
     * @param file Optional source file path.
     * @param range Optional source range.
     * @param hint Optional extra guidance.
     */
    void note(std::string message,
              std::string file = {},
              SourceRange range = {},
              std::optional<std::string> hint = std::nullopt);

    /**
     * @brief Checks whether the bag contains at least one error.
     *
     * @return True if an error diagnostic exists.
     */
    [[nodiscard]] bool has_errors() const noexcept;

    /**
     * @brief Checks whether the bag is empty.
     *
     * @return True if no diagnostics were recorded.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the number of stored diagnostics.
     *
     * @return The diagnostic count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns all diagnostics.
     *
     * @return Read-only list of diagnostics.
     */
    [[nodiscard]] const std::vector<Diagnostic> &items() const noexcept;

    /**
     * @brief Removes all diagnostics.
     */
    void clear() noexcept;

  private:
    std::vector<Diagnostic> diagnostics_{};
  };

  /**
   * @brief Converts a diagnostic severity to a stable text label.
   *
   * @param severity The severity to convert.
   * @return A lowercase severity label.
   */
  [[nodiscard]] std::string_view to_string(DiagnosticSeverity severity) noexcept;

  /**
   * @brief Writes a diagnostic to an output stream.
   *
   * This function intentionally does not emit ANSI colors.
   * It keeps output portable across terminals and platforms.
   *
   * @param out Destination output stream.
   * @param diagnostic Diagnostic to print.
   */
  void print_diagnostic(std::ostream &out, const Diagnostic &diagnostic);

  /**
   * @brief Writes all diagnostics to an output stream.
   *
   * @param out Destination output stream.
   * @param bag Diagnostic collection to print.
   */
  void print_diagnostics(std::ostream &out, const DiagnosticBag &bag);
}

#endif
