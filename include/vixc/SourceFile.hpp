/**
 *
 *  @file SourceFile.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vixc
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix++
 *
 */
#ifndef VIXC_SOURCE_FILE_HPP
#define VIXC_SOURCE_FILE_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <vixc/Diagnostic.hpp>

namespace vixc
{
  /**
   * @brief Represents a source file loaded by Vix++.
   *
   * SourceFile stores the original file path, the full file content,
   * and a line table used to map character offsets to line and column
   * positions.
   */
  class SourceFile
  {
  public:
    /**
     * @brief Creates an empty source file.
     */
    SourceFile() = default;

    /**
     * @brief Creates a source file from a path and content.
     *
     * @param path Source file path.
     * @param content Source file content.
     */
    SourceFile(std::filesystem::path path, std::string content);

    /**
     * @brief Loads a source file from disk.
     *
     * On failure, the error is stored in the provided DiagnosticBag.
     *
     * @param path Path to the source file.
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @return Loaded source file on success, std::nullopt on failure.
     */
    [[nodiscard]] static std::optional<SourceFile> load(
        const std::filesystem::path &path,
        DiagnosticBag &diagnostics);

    /**
     * @brief Returns the source file path.
     *
     * @return The original path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns the source file path as a string.
     *
     * @return String representation of the source file path.
     */
    [[nodiscard]] std::string path_string() const;

    /**
     * @brief Returns the complete source content.
     *
     * @return Full source text.
     */
    [[nodiscard]] std::string_view content() const noexcept;

    /**
     * @brief Returns whether the file content is empty.
     *
     * @return True if the source content is empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the size of the source content.
     *
     * @return Number of bytes in the source content.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Converts a byte offset to a source position.
     *
     * Lines and columns are one-based.
     *
     * @param offset Byte offset in the source content.
     * @return Corresponding source position.
     */
    [[nodiscard]] SourcePosition position_at(std::size_t offset) const noexcept;

    /**
     * @brief Builds a source range from two byte offsets.
     *
     * @param begin Begin byte offset.
     * @param end End byte offset.
     * @return Source range mapped to line and column positions.
     */
    [[nodiscard]] SourceRange range_from_offsets(std::size_t begin,
                                                 std::size_t end) const noexcept;

    /**
     * @brief Returns all source lines.
     *
     * Line endings are not included in the returned strings.
     *
     * @return Vector containing each source line.
     */
    [[nodiscard]] std::vector<std::string_view> lines() const;

  private:
    std::filesystem::path path_{};
    std::string content_{};
    std::vector<std::size_t> line_offsets_{};

    /**
     * @brief Rebuilds the internal line offset table.
     */
    void rebuild_line_offsets();
  };
}

#endif
