/**
 *
 *  @file ImportResolverTest.cpp
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

#include <vixc/Diagnostic.hpp>
#include <vixc/ImportResolver.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
  int failures = 0;

  void expect_true(bool value, const std::string &message)
  {
    if (!value)
    {
      ++failures;
      std::cerr << "FAIL: " << message << '\n';
    }
  }

  void expect_false(bool value, const std::string &message)
  {
    expect_true(!value, message);
  }

  void expect_eq(const std::string &actual,
                 const std::string &expected,
                 const std::string &message)
  {
    if (actual != expected)
    {
      ++failures;
      std::cerr << "FAIL: " << message << '\n'
                << "  expected: " << expected << '\n'
                << "  actual  : " << actual << '\n';
    }
  }

  void test_vix_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("vix.http", diagnostics);

    expect_true(resolved.has_value(), "vix.http should resolve");
    expect_false(diagnostics.has_errors(), "vix.http should not emit errors");

    if (!resolved.has_value())
    {
      return;
    }

    expect_eq(resolved->module, "vix.http", "module should be preserved");
    expect_eq(resolved->include_path, "vix/http.hpp", "vix.http include path");
    expect_eq(resolved->include_line(), "#include <vix/http.hpp>", "vix.http include line");
  }

  void test_vix_nested_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("vix.http.server", diagnostics);

    expect_true(resolved.has_value(), "vix.http.server should resolve by convention");
    expect_false(diagnostics.has_errors(), "vix.http.server should not emit errors");

    if (!resolved.has_value())
    {
      return;
    }

    expect_eq(resolved->include_path, "vix/http/server.hpp", "nested vix include path");
    expect_eq(resolved->include_line(), "#include <vix/http/server.hpp>", "nested vix include line");
  }

  void test_std_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("std.vector", diagnostics);

    expect_true(resolved.has_value(), "std.vector should resolve");
    expect_false(diagnostics.has_errors(), "std.vector should not emit errors");

    if (!resolved.has_value())
    {
      return;
    }

    expect_eq(resolved->include_path, "vector", "std.vector include path");
    expect_eq(resolved->include_line(), "#include <vector>", "std.vector include line");
  }

  void test_local_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("local.config", diagnostics);

    expect_true(resolved.has_value(), "local.config should resolve");
    expect_false(diagnostics.has_errors(), "local.config should not emit errors");

    if (!resolved.has_value())
    {
      return;
    }

    expect_eq(resolved->include_path, "config.hpp", "local.config include path");
    expect_eq(resolved->include_line(), "#include \"config.hpp\"", "local.config include line");
  }

  void test_invalid_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("vix..http", diagnostics);

    expect_false(resolved.has_value(), "vix..http should not resolve");
    expect_true(diagnostics.has_errors(), "vix..http should emit an error");
  }

  void test_unknown_import()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    auto resolved = resolver.resolve("unknown.module", diagnostics);

    expect_false(resolved.has_value(), "unknown.module should not resolve");
    expect_true(diagnostics.has_errors(), "unknown.module should emit an error");
  }

  void test_custom_mapping()
  {
    vixc::DiagnosticBag diagnostics{};
    vixc::ImportResolver resolver{};

    resolver.add_mapping("app.config", "app/config.hpp", vixc::IncludeKind::Local);

    expect_true(resolver.has_mapping("app.config"), "custom mapping should exist");

    auto resolved = resolver.resolve("app.config", diagnostics);

    expect_true(resolved.has_value(), "custom mapping should resolve");
    expect_false(diagnostics.has_errors(), "custom mapping should not emit errors");

    if (!resolved.has_value())
    {
      return;
    }

    expect_eq(resolved->include_line(), "#include \"app/config.hpp\"", "custom local include line");
  }
}

int main()
{
  test_vix_import();
  test_vix_nested_import();
  test_std_import();
  test_local_import();
  test_invalid_import();
  test_unknown_import();
  test_custom_mapping();

  if (failures != 0)
  {
    std::cerr << failures << " test failure(s)\n";
    return EXIT_FAILURE;
  }

  std::cout << "ImportResolver tests passed\n";
  return EXIT_SUCCESS;
}
