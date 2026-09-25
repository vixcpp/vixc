/**
 *
 *  @file DiagnosticEngineTest.cpp
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

#include "../../src/diagnostics/DiagnosticEngine.hpp"

#include <vixc/Diagnostic.hpp>
#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/SourceLocation.hpp>
#include <vixc/SourceRange.hpp>

#include <cassert>
#include <cstddef>
#include <string>

namespace
{

  void test_empty_engine()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    assert(engine.empty());
    assert(engine.size() == 0);

    assert(!engine.has_errors());
    assert(!engine.has_fatal());

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Note) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Warning) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Error) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Fatal) == 0);
  }

  void test_emit_diagnostic()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Warning,
        "something may be wrong"};

    engine.emit(
        std::move(diagnostic));

    assert(!engine.empty());
    assert(engine.size() == 1);

    const auto &diagnostics =
        engine.diagnostics();

    assert(diagnostics.size() == 1);

    assert(
        diagnostics[0].severity() == vixc::DiagnosticSeverity::Warning);

    assert(
        diagnostics[0].message() == "something may be wrong");

    assert(!diagnostics[0].has_code());
    assert(!diagnostics[0].has_range());

    assert(!engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_emit_severity_and_message()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "frontend note");

    assert(engine.size() == 1);

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Note);

    assert(
        diagnostic.message() == "frontend note");

    assert(!diagnostic.has_code());
    assert(!diagnostic.has_range());
  }

  void test_emit_with_range()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        3,
        10,
        18};

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "invalid expression",
        range);

    assert(engine.size() == 1);
    assert(engine.has_errors());
    assert(!engine.has_fatal());

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Error);

    assert(
        diagnostic.message() == "invalid expression");

    assert(!diagnostic.has_code());
    assert(diagnostic.has_range());
    assert(diagnostic.range() == range);
  }

  void test_emit_with_code_and_range()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        1,
        20,
        27};

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "VIXC9999",
        "fatal frontend failure",
        range);

    assert(engine.size() == 1);
    assert(engine.has_errors());
    assert(engine.has_fatal());

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Fatal);

    assert(diagnostic.has_code());
    assert(diagnostic.code() == "VIXC9999");

    assert(
        diagnostic.message() == "fatal frontend failure");

    assert(diagnostic.has_range());
    assert(diagnostic.range() == range);
  }

  void test_note_helper()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        0,
        1,
        4};

    engine.note(
        "additional information",
        range);

    assert(engine.size() == 1);

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Note);

    assert(
        diagnostic.message() == "additional information");

    assert(diagnostic.range() == range);

    assert(!engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_warning_helper()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        0,
        4,
        9};

    engine.warning(
        "warning message",
        range);

    assert(engine.size() == 1);

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Warning);

    assert(
        diagnostic.message() == "warning message");

    assert(diagnostic.range() == range);

    assert(!engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_error_helper()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        2,
        11,
        15};

    engine.error(
        "error message",
        range);

    assert(engine.size() == 1);

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Error);

    assert(
        diagnostic.message() == "error message");

    assert(diagnostic.range() == range);

    assert(engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_fatal_helper()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    const vixc::SourceRange range{
        5,
        100,
        120};

    engine.fatal(
        "fatal message",
        range);

    assert(engine.size() == 1);

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Fatal);

    assert(
        diagnostic.message() == "fatal message");

    assert(diagnostic.range() == range);

    assert(engine.has_errors());
    assert(engine.has_fatal());
  }

  void test_diagnostics_preserve_emission_order()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "first");

    engine.emit(
        vixc::DiagnosticSeverity::Warning,
        "second");

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "third");

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "fourth");

    assert(engine.size() == 4);

    const auto &diagnostics =
        engine.diagnostics();

    assert(diagnostics[0].message() == "first");
    assert(diagnostics[1].message() == "second");
    assert(diagnostics[2].message() == "third");
    assert(diagnostics[3].message() == "fourth");
  }

  void test_severity_counts()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "note one");

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "note two");

    engine.emit(
        vixc::DiagnosticSeverity::Warning,
        "warning one");

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "error one");

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "error two");

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "fatal one");

    assert(engine.size() == 6);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Note) == 2);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Warning) == 1);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Error) == 2);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Fatal) == 1);

    assert(engine.has_errors());
    assert(engine.has_fatal());
  }

  void test_warning_does_not_count_as_error()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Warning,
        "first warning");

    engine.emit(
        vixc::DiagnosticSeverity::Warning,
        "second warning");

    assert(engine.size() == 2);
    assert(!engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_error_sets_error_state()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    assert(!engine.has_errors());

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "error");

    assert(engine.has_errors());
    assert(!engine.has_fatal());
  }

  void test_fatal_sets_error_and_fatal_state()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "fatal");

    assert(engine.has_errors());
    assert(engine.has_fatal());
  }

  void test_clear()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "note");

    engine.emit(
        vixc::DiagnosticSeverity::Warning,
        "warning");

    engine.emit(
        vixc::DiagnosticSeverity::Error,
        "error");

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "fatal");

    assert(engine.size() == 4);
    assert(engine.has_errors());
    assert(engine.has_fatal());

    engine.clear();

    assert(engine.empty());
    assert(engine.size() == 0);

    assert(!engine.has_errors());
    assert(!engine.has_fatal());

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Note) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Warning) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Error) == 0);

    assert(
        engine.count(
            vixc::DiagnosticSeverity::Fatal) == 0);
  }

  void test_reuse_after_clear()
  {
    vixc::diagnostics::DiagnosticEngine engine;

    engine.emit(
        vixc::DiagnosticSeverity::Fatal,
        "old diagnostic");

    assert(engine.has_fatal());

    engine.clear();

    engine.emit(
        vixc::DiagnosticSeverity::Note,
        "new diagnostic");

    assert(engine.size() == 1);
    assert(!engine.has_errors());
    assert(!engine.has_fatal());

    const vixc::Diagnostic &diagnostic =
        engine.diagnostics()[0];

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Note);

    assert(
        diagnostic.message() == "new diagnostic");
  }

} // namespace

int main()
{
  test_empty_engine();
  test_emit_diagnostic();
  test_emit_severity_and_message();
  test_emit_with_range();
  test_emit_with_code_and_range();

  test_note_helper();
  test_warning_helper();
  test_error_helper();
  test_fatal_helper();

  test_diagnostics_preserve_emission_order();
  test_severity_counts();

  test_warning_does_not_count_as_error();
  test_error_sets_error_state();
  test_fatal_sets_error_and_fatal_state();

  test_clear();
  test_reuse_after_clear();

  return 0;
}
