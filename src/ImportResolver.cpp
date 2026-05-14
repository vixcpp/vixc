/**
 *
 *  @file ImportResolver.cpp
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

#include <vixc/ImportResolver.hpp>

#include <algorithm>
#include <cctype>
#include <utility>

namespace vixc
{
  namespace
  {
    [[nodiscard]] bool starts_with(std::string_view value,
                                   std::string_view prefix) noexcept
    {
      return value.size() >= prefix.size() &&
             value.substr(0, prefix.size()) == prefix;
    }

    [[nodiscard]] bool is_ident_char(unsigned char c) noexcept
    {
      return std::isalnum(c) != 0 || c == '_';
    }
  }

  std::string ResolvedImport::include_line() const
  {
    if (kind == IncludeKind::Local)
    {
      return "#include \"" + include_path + "\"";
    }

    return "#include <" + include_path + ">";
  }

  ImportResolver::ImportResolver()
      : mappings_{}
  {
    add_mapping("vix.core", "vix/core.hpp");
    add_mapping("vix.http", "vix/http.hpp");
    add_mapping("vix.json", "vix/json.hpp");
    add_mapping("vix.websocket", "vix/websocket.hpp");
    add_mapping("vix.async", "vix/async.hpp");
    add_mapping("vix.db", "vix/db.hpp");
    add_mapping("vix.orm", "vix/orm.hpp");
    add_mapping("vix.cache", "vix/cache.hpp");
    add_mapping("vix.crypto", "vix/crypto.hpp");
    add_mapping("vix.sync", "vix/sync.hpp");
    add_mapping("vix.p2p", "vix/p2p.hpp");

    add_mapping("std.vector", "vector");
    add_mapping("std.string", "string");
    add_mapping("std.string_view", "string_view");
    add_mapping("std.iostream", "iostream");
    add_mapping("std.filesystem", "filesystem");
    add_mapping("std.optional", "optional");
    add_mapping("std.variant", "variant");
    add_mapping("std.memory", "memory");
    add_mapping("std.unordered_map", "unordered_map");
    add_mapping("std.map", "map");
    add_mapping("std.set", "set");
    add_mapping("std.array", "array");
    add_mapping("std.algorithm", "algorithm");
    add_mapping("std.chrono", "chrono");
    add_mapping("std.thread", "thread");
    add_mapping("std.mutex", "mutex");
    add_mapping("std.atomic", "atomic");
    add_mapping("std.functional", "functional");
  }

  std::optional<ResolvedImport> ImportResolver::resolve(
      std::string_view module,
      DiagnosticBag &diagnostics,
      std::string file,
      SourceRange range) const
  {
    if (!is_valid_module_name(module))
    {
      diagnostics.error(
          "invalid import name",
          std::move(file),
          range,
          "imports must use names like vix.http, std.vector, or local.foo");

      return std::nullopt;
    }

    const std::string key(module);

    if (const auto it = mappings_.find(key); it != mappings_.end())
    {
      return ResolvedImport{
          key,
          it->second.include_path,
          it->second.kind};
    }

    if (auto resolved = resolve_vix_prefix(module))
    {
      return resolved;
    }

    if (auto resolved = resolve_std_prefix(module))
    {
      return resolved;
    }

    if (auto resolved = resolve_local_prefix(module))
    {
      return resolved;
    }

    diagnostics.error(
        "unresolved import '" + std::string(module) + "'",
        std::move(file),
        range,
        "use vix.<module>, std.<header>, or local.<header>");

    return std::nullopt;
  }

  void ImportResolver::add_mapping(std::string module,
                                   std::string include_path,
                                   IncludeKind kind)
  {
    mappings_[std::move(module)] = Mapping{
        std::move(include_path),
        kind};
  }

  bool ImportResolver::has_mapping(std::string_view module) const
  {
    return mappings_.find(std::string(module)) != mappings_.end();
  }

  std::optional<ResolvedImport> ImportResolver::resolve_vix_prefix(
      std::string_view module)
  {
    constexpr std::string_view prefix = "vix.";

    if (!starts_with(module, prefix))
    {
      return std::nullopt;
    }

    const std::string_view rest = remove_prefix(module, prefix);

    if (rest.empty())
    {
      return std::nullopt;
    }

    return ResolvedImport{
        std::string(module),
        "vix/" + dots_to_slashes(rest) + ".hpp",
        IncludeKind::System};
  }

  std::optional<ResolvedImport> ImportResolver::resolve_std_prefix(
      std::string_view module)
  {
    constexpr std::string_view prefix = "std.";

    if (!starts_with(module, prefix))
    {
      return std::nullopt;
    }

    const std::string_view rest = remove_prefix(module, prefix);

    if (rest.empty())
    {
      return std::nullopt;
    }

    return ResolvedImport{
        std::string(module),
        std::string(rest),
        IncludeKind::System};
  }

  std::optional<ResolvedImport> ImportResolver::resolve_local_prefix(
      std::string_view module)
  {
    constexpr std::string_view prefix = "local.";

    if (!starts_with(module, prefix))
    {
      return std::nullopt;
    }

    const std::string_view rest = remove_prefix(module, prefix);

    if (rest.empty())
    {
      return std::nullopt;
    }

    return ResolvedImport{
        std::string(module),
        dots_to_slashes(rest) + ".hpp",
        IncludeKind::Local};
  }

  bool ImportResolver::is_valid_module_name(std::string_view module)
  {
    if (module.empty())
    {
      return false;
    }

    if (module.front() == '.' || module.back() == '.')
    {
      return false;
    }

    bool previous_was_dot = false;

    for (const unsigned char c : module)
    {
      if (c == '.')
      {
        if (previous_was_dot)
        {
          return false;
        }

        previous_was_dot = true;
        continue;
      }

      if (!is_ident_char(c))
      {
        return false;
      }

      previous_was_dot = false;
    }

    return true;
  }

  std::string ImportResolver::dots_to_slashes(std::string_view value)
  {
    std::string result(value);

    std::replace(result.begin(), result.end(), '.', '/');

    return result;
  }

  std::string_view ImportResolver::remove_prefix(std::string_view value,
                                                 std::string_view prefix)
  {
    if (!starts_with(value, prefix))
    {
      return value;
    }

    return value.substr(prefix.size());
  }
}
