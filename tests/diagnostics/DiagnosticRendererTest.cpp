/**
 *
 *  @file DiagnosticRendererTest.cpp
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

#include <vixc/Diagnostic.hpp>
#include <vixc/DiagnosticRenderer.hpp>
#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/SourceRange.hpp>

#include <cassert>
#include <cstddef>
#include <string>

namespace
{

  void test_renders_header_location_excerpt_underline_and_hint()
  {
    const std::string source =
        "int main()\n"
        "{\n"
        "    auto value = try calculate();\n"
        "}\n";

    const std::size_t begin =
        source.find("try");
    const std::size_t line_start =
        source.rfind('\n', begin) + 1;

    const vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Error,
        "VIXC2014",
        "'try' can only propagate failure inside a failure-aware computation",
        vixc::SourceRange{
            0,
            begin,
            begin + 3},
        "declare the containing function with `fails <ErrorType>`"};

    const vixc::DiagnosticRenderer renderer;
    const std::string rendered =
        renderer.render(
            diagnostic,
            "/tmp/main.cpp",
            source);

    assert(
        rendered.find(
            "error [VIXC2014]: 'try' can only propagate failure inside a failure-aware computation") !=
        std::string::npos);

    assert(
        rendered.find("--> /tmp/main.cpp:3:18") !=
        std::string::npos);

    assert(
        rendered.find("3 |     auto value = try calculate();") !=
        std::string::npos);

    const std::string underline =
        "  | " +
        std::string(
            begin - line_start,
            ' ') +
        "^^^";

    assert(rendered.find(underline) != std::string::npos);

    assert(
        rendered.find(
            "hint: declare the containing function with `fails <ErrorType>`") !=
        std::string::npos);
  }

  void test_diagnostic_without_hint_omits_hint_line()
  {
    const std::string source =
        "int value;\n";

    const vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Warning,
        "VIXC9000",
        "example warning",
        vixc::SourceRange{
            0,
            4,
            9}};

    const vixc::DiagnosticRenderer renderer;
    const std::string rendered =
        renderer.render(
            diagnostic,
            "warning.cpp",
            source);

    assert(rendered.find("warning [VIXC9000]: example warning") != std::string::npos);
    assert(rendered.find("hint:") == std::string::npos);
  }

  void test_beginning_and_end_of_file_ranges_are_safe()
  {
    const std::string source =
        "try work();\n"
        "fail 1;";

    const vixc::Diagnostic first{
        vixc::DiagnosticSeverity::Error,
        "VIXC2014",
        "invalid propagation",
        vixc::SourceRange{
            0,
            0,
            3}};

    const vixc::Diagnostic last{
        vixc::DiagnosticSeverity::Error,
        "VIXC2006",
        "invalid failure",
        vixc::SourceRange{
            0,
            source.find("fail"),
            source.find("fail") + 4}};

    const vixc::DiagnosticRenderer renderer;

    const std::string first_rendered =
        renderer.render(
            first,
            "boundary.cpp",
            source);

    assert(first_rendered.find("--> boundary.cpp:1:1") != std::string::npos);
    assert(first_rendered.find("0 |") == std::string::npos);

    const std::string last_rendered =
        renderer.render(
            last,
            "boundary.cpp",
            source);

    assert(last_rendered.find("--> boundary.cpp:2:1") != std::string::npos);
    assert(last_rendered.find("3 |") == std::string::npos);
  }

  void test_empty_range_renders_one_caret()
  {
    const std::string source =
        "int main()\n";

    const vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Error,
        "VIXC9001",
        "expected token",
        vixc::SourceRange{
            0,
            4,
            4}};

    const vixc::DiagnosticRenderer renderer;
    const std::string rendered =
        renderer.render(
            diagnostic,
            "empty.cpp",
            source);

    assert(rendered.find("  |     ^") != std::string::npos);
  }

  void test_tabbed_prefix_is_preserved_in_underline()
  {
    const std::string source =
        "\ttry work();\n";

    const vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Error,
        "VIXC2014",
        "invalid propagation",
        vixc::SourceRange{
            0,
            1,
            4}};

    const vixc::DiagnosticRenderer renderer;
    const std::string rendered =
        renderer.render(
            diagnostic,
            "tab.cpp",
            source);

    assert(rendered.find("  | \t^^^") != std::string::npos);
  }

} // namespace

int main()
{
  test_renders_header_location_excerpt_underline_and_hint();
  test_diagnostic_without_hint_omits_hint_line();
  test_beginning_and_end_of_file_ranges_are_safe();
  test_empty_range_renders_one_caret();
  test_tabbed_prefix_is_preserved_in_underline();

  return 0;
}
