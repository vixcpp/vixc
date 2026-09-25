/**
 *
 *  @file CxxEmitter.hpp
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

#if !defined(VIXC_BACKENDS_CXX_CXX_EMITTER_HPP)
#define VIXC_BACKENDS_CXX_CXX_EMITTER_HPP

#include "CxxSourceMap.hpp"

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace vixc::backends::cxx
{
  /**
   * @brief Builds generated C++ source and records its source provenance.
   *
   * CxxEmitter is the text emission layer used by the C++ backend. It owns the
   * generated translation-unit text and the CxxSourceMap that relates generated
   * regions back to the original VixC source.
   *
   * The emitter does not understand VixC semantic meaning. It does not decide
   * how Outcome, Failure, FailurePropagation, or future IR constructs should be
   * represented in C++. Those decisions belong to CxxBackend.
   *
   * CxxEmitter provides only the mechanics required by that backend: appending
   * text, attaching source provenance, managing indentation, and exposing the
   * completed generated source.
   *
   * Text can be emitted either with or without an original SourceRange.
   * Backend-generated support code normally has no direct source mapping, while
   * code corresponding to original source or a VixC construct can be emitted
   * with an explicit mapping.
   *
   * Generated offsets are byte offsets into output(). Source-map entries use
   * the same half-open interval convention as SourceRange.
   *
   * The emitter is deterministic. Given the same sequence of operations, it
   * produces the same output bytes and source-map entries.
   */
  class CxxEmitter final
  {
  public:
    /**
     * @brief Creates an empty C++ emitter.
     *
     * The generated output is initially empty, the indentation level is zero,
     * and the source map contains no entries.
     */
    CxxEmitter() = default;

    CxxEmitter(const CxxEmitter &) = delete;
    CxxEmitter &operator=(const CxxEmitter &) = delete;

    CxxEmitter(CxxEmitter &&) noexcept = default;
    CxxEmitter &operator=(CxxEmitter &&) noexcept = default;

    ~CxxEmitter() = default;

    /**
     * @brief Appends generated C++ text without source provenance.
     *
     * This operation is intended for backend-generated syntax, support code,
     * formatting, whitespace, and other text that does not correspond directly
     * to a specific original source range.
     *
     * No source-map entry is created.
     *
     * @param text Text to append to the generated output.
     */
    void write(std::string_view text);

    /**
     * @brief Appends generated C++ text associated with original source.
     *
     * A source-map entry is created for the exact generated interval occupied
     * by text.
     *
     * The supplied SourceRange must be structurally valid. When it is invalid,
     * the text is still emitted but no source-map entry is recorded.
     *
     * Empty text is allowed. Because a zero-length mapping contains no generated
     * byte, no source-map entry is created for an empty string.
     *
     * @param text Text to append.
     * @param original_range Original VixC source represented by the text.
     */
    void write(
        std::string_view text,
        SourceRange original_range);

    /**
     * @brief Emits indentation for the current line.
     *
     * One indentation level corresponds to two ASCII space bytes.
     *
     * Indentation is backend-generated formatting and therefore does not create
     * source-map entries.
     */
    void write_indent();

    /**
     * @brief Appends one newline to the generated source.
     *
     * The emitter uses the LF byte consistently regardless of host platform so
     * generated output remains deterministic.
     */
    void newline();

    /**
     * @brief Appends text followed by a newline.
     *
     * No source mapping is created for the appended text.
     *
     * @param text Text to append before the newline.
     */
    void write_line(std::string_view text);

    /**
     * @brief Appends mapped text followed by a newline.
     *
     * The source-map entry covers only text. The generated newline is considered
     * backend formatting and is not included in the mapping.
     *
     * @param text Text to append before the newline.
     * @param original_range Original source represented by text.
     */
    void write_line(
        std::string_view text,
        SourceRange original_range);

    /**
     * @brief Emits current indentation followed by text.
     *
     * Indentation is not included in source provenance.
     *
     * @param text Text to append after indentation.
     */
    void write_indented(std::string_view text);

    /**
     * @brief Emits current indentation followed by mapped text.
     *
     * The source-map entry covers only text and excludes indentation.
     *
     * @param text Text to append after indentation.
     * @param original_range Original source represented by text.
     */
    void write_indented(
        std::string_view text,
        SourceRange original_range);

    /**
     * @brief Emits current indentation, text, and a newline.
     *
     * @param text Text to emit on the line.
     */
    void write_indented_line(std::string_view text);

    /**
     * @brief Emits current indentation, mapped text, and a newline.
     *
     * Only text is associated with original source. Indentation and the newline
     * remain backend-generated formatting.
     *
     * @param text Text to emit on the line.
     * @param original_range Original source represented by text.
     */
    void write_indented_line(
        std::string_view text,
        SourceRange original_range);

    /**
     * @brief Increases the indentation level by one.
     */
    void indent() noexcept;

    /**
     * @brief Decreases the indentation level by one.
     *
     * Calling this function at indentation level zero has no effect.
     */
    void dedent() noexcept;

    /**
     * @brief Returns the current indentation level.
     *
     * @return Number of active indentation levels.
     */
    [[nodiscard]]
    std::size_t indentation_level() const noexcept;

    /**
     * @brief Returns the current generated byte offset.
     *
     * This is equivalent to output().size() and identifies the position where
     * the next emitted byte will be written.
     *
     * @return Current generated output size in bytes.
     */
    [[nodiscard]]
    std::size_t offset() const noexcept;

    /**
     * @brief Returns the complete generated C++ source.
     *
     * The returned reference remains valid until the emitter is modified,
     * moved from, reset, or destroyed.
     *
     * @return Generated C++ translation-unit text.
     */
    [[nodiscard]]
    const std::string &output() const noexcept;

    /**
     * @brief Returns the generated C++ source map.
     *
     * @return Read-only source map associated with output().
     */
    [[nodiscard]]
    const CxxSourceMap &source_map() const noexcept;

    /**
     * @brief Reports whether no C++ source has been emitted.
     *
     * @return true when output() is empty.
     */
    [[nodiscard]]
    bool empty() const noexcept;

    /**
     * @brief Clears generated output and source mapping state.
     *
     * The indentation level is also restored to zero.
     */
    void reset() noexcept;

  private:
    /**
     * @brief Appends one source mapping for a generated interval.
     *
     * The function assumes the generated interval has already been appended to
     * output_. Invalid or empty mappings are ignored.
     *
     * @param generated_begin Inclusive generated byte offset.
     * @param generated_end Exclusive generated byte offset.
     * @param original_range Original source represented by the interval.
     */
    void map(
        std::size_t generated_begin,
        std::size_t generated_end,
        SourceRange original_range);

    /// Complete generated C++ source.
    std::string output_;

    /// Provenance information for generated source regions.
    CxxSourceMap source_map_;

    /// Current indentation depth.
    std::size_t indentation_level_{0};
  };

} // namespace vixc::backends::cxx

#endif // VIXC_BACKENDS_CXX_CXX_EMITTER_HPP
