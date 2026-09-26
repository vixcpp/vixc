/**
 *
 *  @file DiagnosticEngine.hpp
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

#if !defined(VIXC_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP)
#define VIXC_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP

#include <vixc/Diagnostic.hpp>
#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace vixc::diagnostics
{
  /**
   * @brief Collects diagnostics produced during a frontend operation.
   *
   * DiagnosticEngine owns the diagnostics emitted by syntax analysis, semantic
   * analysis, lowering, backend processing, and other frontend stages.
   *
   * The engine stores diagnostics as structured values. It does not print,
   * format, colorize, or otherwise present them. Presentation belongs to the
   * caller so the same frontend can be embedded in command-line tools, editors,
   * IDE integrations, tests, build systems, and other software.
   *
   * DiagnosticEngine also tracks whether errors or fatal diagnostics have been
   * emitted. Frontend stages can use this information to decide whether later
   * processing remains meaningful.
   *
   * Diagnostics are stored in emission order.
   */
  class DiagnosticEngine final
  {
  public:
    /**
     * @brief Creates an empty diagnostic engine.
     */
    DiagnosticEngine() = default;

    DiagnosticEngine(const DiagnosticEngine &) = delete;
    DiagnosticEngine &operator=(const DiagnosticEngine &) = delete;

    DiagnosticEngine(DiagnosticEngine &&) noexcept = default;
    DiagnosticEngine &operator=(DiagnosticEngine &&) noexcept = default;

    ~DiagnosticEngine() = default;

    /**
     * @brief Adds a complete diagnostic to the engine.
     *
     * The diagnostic is moved into the internal collection and remains owned by
     * the engine until clear() is called or the engine is destroyed.
     *
     * @param diagnostic Diagnostic to store.
     */
    void emit(Diagnostic diagnostic);

    /**
     * @brief Creates and stores a diagnostic without a source range.
     *
     * @param severity Severity of the diagnostic.
     * @param message Human-readable diagnostic message.
     */
    void emit(
        DiagnosticSeverity severity,
        std::string message);

    /**
     * @brief Creates and stores a diagnostic associated with a source range.
     *
     * @param severity Severity of the diagnostic.
     * @param message Human-readable diagnostic message.
     * @param range Primary source range associated with the diagnostic.
     */
    void emit(
        DiagnosticSeverity severity,
        std::string message,
        SourceRange range);

    /**
     * @brief Creates and stores a diagnostic with a stable diagnostic code.
     *
     * @param severity Severity of the diagnostic.
     * @param code Stable diagnostic identifier.
     * @param message Human-readable diagnostic message.
     * @param range Primary source range associated with the diagnostic.
     */
    void emit(
        DiagnosticSeverity severity,
        std::string code,
        std::string message,
        SourceRange range);

    /**
     * @brief Creates and stores a diagnostic with code, range, and hint.
     *
     * @param severity Severity of the diagnostic.
     * @param code Stable diagnostic identifier.
     * @param message Human-readable diagnostic message.
     * @param range Primary source range associated with the diagnostic.
     * @param hint Semantic guidance selected by the diagnostic producer.
     */
    void emit(
        DiagnosticSeverity severity,
        std::string code,
        std::string message,
        SourceRange range,
        std::string hint);

    /**
     * @brief Emits a note diagnostic.
     *
     * @param message Human-readable diagnostic message.
     * @param range Optional source range associated with the note.
     */
    void note(
        std::string message,
        SourceRange range = {});

    /**
     * @brief Emits a warning diagnostic.
     *
     * @param message Human-readable diagnostic message.
     * @param range Optional source range associated with the warning.
     */
    void warning(
        std::string message,
        SourceRange range = {});

    /**
     * @brief Emits an error diagnostic.
     *
     * @param message Human-readable diagnostic message.
     * @param range Optional source range associated with the error.
     */
    void error(
        std::string message,
        SourceRange range = {});

    /**
     * @brief Emits a fatal diagnostic.
     *
     * Fatal diagnostics indicate that the current frontend operation cannot
     * continue meaningfully.
     *
     * @param message Human-readable diagnostic message.
     * @param range Optional source range associated with the fatal condition.
     */
    void fatal(
        std::string message,
        SourceRange range = {});

    /**
     * @brief Returns all diagnostics in emission order.
     *
     * The returned reference remains valid until the engine is modified or
     * destroyed.
     *
     * @return Read-only diagnostic collection.
     */
    [[nodiscard]]
    const std::vector<Diagnostic> &diagnostics() const noexcept;

    /**
     * @brief Returns the number of diagnostics currently stored.
     *
     * @return Number of emitted diagnostics.
     */
    [[nodiscard]]
    std::size_t size() const noexcept;

    /**
     * @brief Reports whether no diagnostics have been emitted.
     *
     * @return true when the diagnostic collection is empty, otherwise false.
     */
    [[nodiscard]]
    bool empty() const noexcept;

    /**
     * @brief Reports whether at least one Error or Fatal diagnostic exists.
     *
     * @return true when the current diagnostic set contains an error.
     */
    [[nodiscard]]
    bool has_errors() const noexcept;

    /**
     * @brief Reports whether at least one Fatal diagnostic exists.
     *
     * @return true when the current diagnostic set contains a fatal diagnostic.
     */
    [[nodiscard]]
    bool has_fatal() const noexcept;

    /**
     * @brief Counts diagnostics having a specific severity.
     *
     * @param severity Severity to count.
     *
     * @return Number of diagnostics with the requested severity.
     */
    [[nodiscard]]
    std::size_t count(DiagnosticSeverity severity) const noexcept;

    /**
     * @brief Removes all diagnostics from the engine.
     *
     * After clearing, size() returns zero and both has_errors() and has_fatal()
     * return false.
     */
    void clear() noexcept;

  private:
    /// Diagnostics stored in emission order.
    std::vector<Diagnostic> diagnostics_;

    /// Number of diagnostics whose severity is Error or Fatal.
    std::size_t error_count_{0};

    /// Number of diagnostics whose severity is Fatal.
    std::size_t fatal_count_{0};
  };

} // namespace vixc::diagnostics

#endif // VIXC_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP
