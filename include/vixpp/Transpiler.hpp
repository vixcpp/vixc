/**
 *
 *  @file Transpiler.hpp
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
#ifndef VIXPP_TRANSPILER_HPP
#define VIXPP_TRANSPILER_HPP

#include <string>
#include <string_view>
#include <vector>

#include <vixpp/Diagnostic.hpp>
#include <vixpp/ImportResolver.hpp>
#include <vixpp/SourceFile.hpp>

namespace vixpp
{
  /**
   * @brief Result produced by the Vix++ transpiler.
   */
  struct TranspileResult
  {
    std::string code{};
    std::vector<ResolvedImport> imports{};

    /**
     * @brief Checks whether generated C++ code exists.
     *
     * @return True if the generated code is not empty.
     */
    [[nodiscard]] bool has_code() const noexcept;
  };

  /**
   * @brief Converts Vix++ source code into standard C++.
   *
   * The MVP transpiler supports import lines at top level:
   *
   *   use vix.http;
   *   use std.vector;
   *   use local.config;
   *
   * These lines are converted to C++ #include directives.
   * All other lines are copied as standard C++.
   */
  class Transpiler
  {
  public:
    /**
     * @brief Creates a transpiler with the default import resolver.
     */
    Transpiler();

    /**
     * @brief Creates a transpiler using a custom import resolver.
     *
     * @param resolver Import resolver used to convert use declarations.
     */
    explicit Transpiler(ImportResolver resolver);

    /**
     * @brief Transpiles a loaded source file.
     *
     * @param source Source file to transpile.
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @return Generated C++ code and resolved imports.
     */
    [[nodiscard]] TranspileResult transpile(const SourceFile &source,
                                            DiagnosticBag &diagnostics) const;

    /**
     * @brief Transpiles raw source text.
     *
     * This helper is useful for tests.
     *
     * @param content Source text.
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @param file Optional file name used in diagnostics.
     * @return Generated C++ code and resolved imports.
     */
    [[nodiscard]] TranspileResult transpile_string(std::string_view content,
                                                   DiagnosticBag &diagnostics,
                                                   std::string file = {}) const;

  private:
    ImportResolver resolver_{};

    /**
     * @brief Parses one Vix++ import line.
     *
     * @param line Current source line.
     * @param module Output module name when the line is an import.
     * @return True if the line is a Vix++ use declaration.
     */
    [[nodiscard]] static bool parse_use_line(std::string_view line,
                                             std::string &module);

    /**
     * @brief Returns whether a line is empty or whitespace-only.
     *
     * @param line Source line.
     * @return True if the line contains only whitespace.
     */
    [[nodiscard]] static bool is_blank(std::string_view line) noexcept;

    /**
     * @brief Returns whether a line is a comment line.
     *
     * Only // comments are treated as line comments by the MVP transpiler.
     *
     * @param line Source line.
     * @return True if the trimmed line starts with //.
     */
    [[nodiscard]] static bool is_line_comment(std::string_view line) noexcept;

    /**
     * @brief Trims whitespace from both sides of a string view.
     *
     * @param value Input text.
     * @return Trimmed text view.
     */
    [[nodiscard]] static std::string_view trim(std::string_view value) noexcept;

    /**
     * @brief Trims whitespace from the left side of a string view.
     *
     * @param value Input text.
     * @return Left-trimmed text view.
     */
    [[nodiscard]] static std::string_view ltrim(std::string_view value) noexcept;

    /**
     * @brief Checks whether a character is valid whitespace for syntax parsing.
     *
     * @param c Character to check.
     * @return True if the character is a supported whitespace character.
     */
    [[nodiscard]] static bool is_space(char c) noexcept;
  };
}

#endif
