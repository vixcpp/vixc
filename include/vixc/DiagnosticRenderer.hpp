/**
 *
 *  @file DiagnosticRenderer.hpp
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

#if !defined(VIXC_DIAGNOSTIC_RENDERER_HPP)
#define VIXC_DIAGNOSTIC_RENDERER_HPP

#include <vixc/Diagnostic.hpp>

#include <string>
#include <string_view>

namespace vixc
{
  /**
   * @brief Formats structured VixC diagnostics as deterministic plain text.
   *
   * DiagnosticRenderer converts a Diagnostic and its original source text into
   * a terminal-friendly excerpt. It does not emit output itself, interpret
   * diagnostic codes, or determine semantic hints. This keeps semantic
   * producers, embedding applications, and presentation layers independent.
   *
   * Source positions remain byte-oriented, matching SourceLocation and
   * SourceRange. Tabs in the prefix before a highlighted range are preserved
   * in the underline so its horizontal position follows the rendered source.
   */
  class DiagnosticRenderer final
  {
  public:
    /**
     * @brief Creates a stateless diagnostic renderer.
     */
    DiagnosticRenderer() = default;

    /**
     * @brief Renders one diagnostic with optional source context.
     *
     * When the diagnostic range is valid and belongs within source, the result
     * includes the source path, one-based line and column, the affected line,
     * and an underline derived from the range. The immediately preceding and
     * following source lines are shown when they exist.
     *
     * @param diagnostic Structured diagnostic to render.
     * @param source_name Path or logical identifier of the original source.
     * @param source Complete original source text.
     *
     * @return Plain-text rendering suitable for terminals, logs, and tests.
     */
    [[nodiscard]]
    std::string render(
        const Diagnostic &diagnostic,
        std::string_view source_name,
        std::string_view source) const;
  };

} // namespace vixc

#endif // VIXC_DIAGNOSTIC_RENDERER_HPP
