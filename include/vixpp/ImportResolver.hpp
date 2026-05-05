/**
 *
 *  @file ImportResolver.hpp
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
#ifndef VIXPP_IMPORT_RESOLVER_HPP
#define VIXPP_IMPORT_RESOLVER_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vixpp/Diagnostic.hpp>

namespace vixpp
{
  /**
   * @brief Describes the kind of C++ include generated from a Vix++ import.
   */
  enum class IncludeKind
  {
    System,
    Local
  };

  /**
   * @brief Represents one resolved import.
   */
  struct ResolvedImport
  {
    std::string module{};
    std::string include_path{};
    IncludeKind kind{IncludeKind::System};

    /**
     * @brief Returns the generated C++ include line.
     *
     * @return A C++ #include directive.
     */
    [[nodiscard]] std::string include_line() const;
  };

  /**
   * @brief Resolves Vix++ import names into C++ include directives.
   *
   * The MVP resolver intentionally stays simple:
   *
   * - use vix.http;       -> #include <vix/http.hpp>
   * - use vix.json;       -> #include <vix/json.hpp>
   * - use std.vector;     -> #include <vector>
   * - use local.foo;      -> #include "foo.hpp"
   *
   * More advanced module metadata and dependency-aware imports can be added
   * later without changing the public API.
   */
  class ImportResolver
  {
  public:
    /**
     * @brief Creates a resolver with the default import mappings.
     */
    ImportResolver();

    /**
     * @brief Resolves one import module name.
     *
     * @param module Import module name, for example "vix.http".
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @param file Optional source file path for diagnostics.
     * @param range Optional source range for diagnostics.
     * @return A resolved import on success, std::nullopt on failure.
     */
    [[nodiscard]] std::optional<ResolvedImport> resolve(
        std::string_view module,
        DiagnosticBag &diagnostics,
        std::string file = {},
        SourceRange range = {}) const;

    /**
     * @brief Adds or overrides an exact import mapping.
     *
     * @param module Import module name.
     * @param include_path C++ include path.
     * @param kind Include kind.
     */
    void add_mapping(std::string module,
                     std::string include_path,
                     IncludeKind kind = IncludeKind::System);

    /**
     * @brief Checks whether an exact mapping exists.
     *
     * @param module Import module name.
     * @return True if the module has an exact mapping.
     */
    [[nodiscard]] bool has_mapping(std::string_view module) const;

  private:
    struct Mapping
    {
      std::string include_path{};
      IncludeKind kind{IncludeKind::System};
    };

    std::unordered_map<std::string, Mapping> mappings_{};

    /**
     * @brief Resolves imports using the "vix." prefix convention.
     */
    [[nodiscard]] static std::optional<ResolvedImport> resolve_vix_prefix(
        std::string_view module);

    /**
     * @brief Resolves imports using the "std." prefix convention.
     */
    [[nodiscard]] static std::optional<ResolvedImport> resolve_std_prefix(
        std::string_view module);

    /**
     * @brief Resolves imports using the "local." prefix convention.
     */
    [[nodiscard]] static std::optional<ResolvedImport> resolve_local_prefix(
        std::string_view module);

    /**
     * @brief Checks whether the import module name is valid.
     */
    [[nodiscard]] static bool is_valid_module_name(std::string_view module);

    /**
     * @brief Replaces dots with slashes.
     */
    [[nodiscard]] static std::string dots_to_slashes(std::string_view value);

    /**
     * @brief Removes the given prefix from a string view.
     */
    [[nodiscard]] static std::string_view remove_prefix(std::string_view value,
                                                        std::string_view prefix);
  };
}

#endif
