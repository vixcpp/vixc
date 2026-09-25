/**
 *
 *  @file SourceFile.hpp
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

#if !defined(VIXC_SOURCE_SOURCE_FILE_HPP)
#define VIXC_SOURCE_SOURCE_FILE_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vixc::source
{

  /**
   * @brief Immutable source text owned by the VixC frontend.
   *
   * SourceFile stores the complete contents of one source unit together with
   * the path used to identify it.
   *
   * The class also maintains an index of line start offsets so later frontend
   * stages can translate byte offsets into source lines without rescanning the
   * complete file.
   *
   * SourceFile does not perform filesystem I/O. The caller provides both the
   * path and the source contents. This allows the same representation to be
   * used for files loaded from disk, editor buffers, generated sources, tests,
   * and other in-memory inputs.
   *
   * All offsets used by SourceFile are zero-based byte offsets into the stored
   * source text. They are not Unicode code-point or display-column indexes.
   *
   * Line indexes are also zero-based.
   *
   * A SourceFile always contains at least one logical line. An empty source
   * therefore has a line count of one and line(0) returns an empty view.
   */
  class SourceFile final
  {
  public:
    /**
     * @brief Creates a source file from an identifier and its contents.
     *
     * The supplied strings are moved into the SourceFile and remain owned by
     * the object for its lifetime.
     *
     * The path is an identifier for diagnostics and source tracking. It does
     * not need to refer to an existing filesystem entry.
     *
     * @param path Path or logical identifier associated with the source.
     * @param contents Complete source text.
     */
    SourceFile(std::string path, std::string contents);

    SourceFile(const SourceFile &) = delete;
    SourceFile &operator=(const SourceFile &) = delete;

    SourceFile(SourceFile &&) noexcept = default;
    SourceFile &operator=(SourceFile &&) noexcept = default;

    ~SourceFile() = default;

    /**
     * @brief Returns the path or logical identifier of the source.
     *
     * @return A non-owning view of the stored path.
     */
    [[nodiscard]]
    std::string_view path() const noexcept;

    /**
     * @brief Returns the complete source text.
     *
     * The returned view remains valid until the SourceFile is destroyed or
     * moved from.
     *
     * @return A non-owning view of the complete source contents.
     */
    [[nodiscard]]
    std::string_view contents() const noexcept;

    /**
     * @brief Returns the size of the source text in bytes.
     *
     * @return Number of bytes stored in the source contents.
     */
    [[nodiscard]]
    std::size_t size() const noexcept;

    /**
     * @brief Reports whether the source text is empty.
     *
     * @return true when the source contains no bytes, otherwise false.
     */
    [[nodiscard]]
    bool empty() const noexcept;

    /**
     * @brief Returns the number of logical lines in the source.
     *
     * The result is always at least one.
     *
     * A trailing newline creates a final empty logical line. For example,
     * `"a\n"` contains two logical lines.
     *
     * @return Number of logical lines in the source.
     */
    [[nodiscard]]
    std::size_t line_count() const noexcept;

    /**
     * @brief Finds the logical line containing a source offset.
     *
     * The offset is a zero-based byte offset into contents().
     *
     * An offset equal to size() is valid and represents the end-of-file
     * position. An offset greater than size() is invalid.
     *
     * When the source ends with a newline, the end-of-file position belongs to
     * the final empty logical line.
     *
     * @param offset Byte offset into the source.
     *
     * @return The zero-based line index containing the offset, or std::nullopt
     *         when the offset is outside the source.
     */
    [[nodiscard]]
    std::optional<std::size_t>
    line_index_for_offset(std::size_t offset) const noexcept;

    /**
     * @brief Returns the byte offset at which a logical line begins.
     *
     * @param line_index Zero-based logical line index.
     *
     * @return The byte offset of the first character of the line, or
     *         std::nullopt when the line index is invalid.
     */
    [[nodiscard]]
    std::optional<std::size_t>
    line_start_offset(std::size_t line_index) const noexcept;

    /**
     * @brief Returns the text of a logical line.
     *
     * The returned view excludes the line terminator. Both LF and CRLF input
     * are handled so the returned line does not contain '\\n' or the '\\r'
     * belonging to a CRLF terminator.
     *
     * The returned view refers directly to the SourceFile contents and remains
     * valid until the SourceFile is destroyed or moved from.
     *
     * @param line_index Zero-based logical line index.
     *
     * @return A view of the requested line, or std::nullopt when the line index
     *         is invalid.
     */
    [[nodiscard]]
    std::optional<std::string_view>
    line(std::size_t line_index) const noexcept;

  private:
    /**
     * @brief Builds the internal index of logical line start offsets.
     *
     * Offset zero is always registered as the beginning of the first logical
     * line. Every LF byte registers the following byte as the start of the next
     * logical line.
     */
    void build_line_index();

    /// Path or logical identifier associated with this source unit.
    std::string path_;

    /// Complete source text owned by this SourceFile.
    std::string contents_;

    /// Zero-based byte offset of the beginning of every logical line.
    std::vector<std::size_t> line_starts_;
  };

} // namespace vixc::source

#endif // VIXC_SOURCE_SOURCE_FILE_HPP
