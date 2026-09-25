/**
 *
 *  @file FailureLoweringTest.cpp
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
#include "../../src/ir/IrKind.hpp"
#include "../../src/ir/IrNode.hpp"
#include "../../src/ir/Program.hpp"
#include "../../src/ir/failure/Failure.hpp"
#include "../../src/ir/failure/Outcome.hpp"
#include "../../src/lowering/LoweringContext.hpp"
#include "../../src/lowering/failure/FailureLowering.hpp"
#include "../../src/source/SourceManager.hpp"

#include <vixc/SourceRange.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace
{
  using vixc::ir::IrKind;
  using vixc::ir::IrNode;
  using vixc::ir::Program;
  using vixc::ir::failure::Failure;
  using vixc::ir::failure::FailurePropagation;
  using vixc::ir::failure::Outcome;
  using vixc::lowering::LoweringContext;
  using vixc::lowering::failure::FailureLowering;

  std::unique_ptr<IrNode>
  make_cxx_region(
      vixc::SourceRange range)
  {
    return std::make_unique<IrNode>(
        IrKind::CxxRegion,
        range);
  }

  bool has_diagnostic_code(
      const vixc::diagnostics::DiagnosticEngine &diagnostics,
      const char *code)
  {
    for (const vixc::Diagnostic &diagnostic :
         diagnostics.diagnostics())
    {
      if (
          diagnostic.has_code() && diagnostic.code() == code)
      {
        return true;
      }
    }

    return false;
  }

  void test_success_only_outcome_is_lowered()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            "value");

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    Outcome outcome{
        vixc::SourceRange{
            source_id,
            0,
            5}};

    assert(outcome.valid());

    assert(
        lowering.lower(
            outcome));

    assert(!diagnostics.has_errors());
  }

  void test_failure_aware_outcome_is_lowered()
  {
    const std::string source =
        "Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const vixc::SourceRange outcome_range{
        source_id,
        0,
        5};

    const vixc::SourceRange failure_type_range{
        source_id,
        0,
        5};

    Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(outcome.valid());
    assert(outcome.allows_failure());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        lowering.lower(
            outcome));

    assert(!diagnostics.has_errors());
  }

  void test_invalid_outcome_is_rejected()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    Outcome outcome{
        vixc::SourceRange{
            0,
            0,
            1},
        vixc::SourceRange{}};

    assert(!outcome.valid());

    assert(
        !lowering.lower(
            outcome));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3105"));
  }

  void test_outcome_requires_recoverable_failure_type_source()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            "Error");

    const vixc::SourceRange outcome_range{
        source_id,
        0,
        100};

    const vixc::SourceRange failure_type_range{
        source_id,
        50,
        55};

    Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(outcome.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            outcome));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3109"));
  }

  void test_valid_failure_is_lowered()
  {
    const std::string source =
        "Error error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    Failure failure{
        vixc::SourceRange{
            source_id,
            6,
            11},
        vixc::SourceRange{
            source_id,
            0,
            5},
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                6,
                11})};

    assert(failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        lowering.lower(
            failure));

    assert(!diagnostics.has_errors());
  }

  void test_failure_without_operand_is_rejected()
  {
    const std::string source =
        "Error error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    Failure failure{
        vixc::SourceRange{
            source_id,
            6,
            11},
        vixc::SourceRange{
            source_id,
            0,
            5}};

    assert(!failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            failure));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3110"));
  }

  void test_failure_requires_failure_type_source()
  {
    const std::string source =
        "error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    Failure failure{
        vixc::SourceRange{
            source_id,
            0,
            5},
        vixc::SourceRange{
            source_id,
            50,
            55},
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                5})};

    assert(failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            failure));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3111"));
  }

  void test_valid_failure_propagation_is_lowered()
  {
    const std::string source =
        "Error read";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    FailurePropagation propagation{
        vixc::SourceRange{
            source_id,
            6,
            10},
        vixc::SourceRange{
            source_id,
            0,
            5},
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                6,
                10})};

    assert(propagation.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        lowering.lower(
            propagation));

    assert(!diagnostics.has_errors());
  }

  void test_failure_propagation_without_operand_is_rejected()
  {
    const std::string source =
        "Error read";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    FailurePropagation propagation{
        vixc::SourceRange{
            source_id,
            6,
            10},
        vixc::SourceRange{
            source_id,
            0,
            5}};

    assert(!propagation.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            propagation));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3113"));
  }

  void test_failure_propagation_requires_failure_type_source()
  {
    const std::string source =
        "read";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    FailurePropagation propagation{
        vixc::SourceRange{
            source_id,
            0,
            4},
        vixc::SourceRange{
            source_id,
            50,
            55},
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                4})};

    assert(propagation.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            propagation));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3114"));
  }

  void test_nested_failure_propagation_is_lowered()
  {
    const std::string source =
        "Error error read";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    auto propagation =
        std::make_unique<FailurePropagation>(
            vixc::SourceRange{
                source_id,
                12,
                16},
            vixc::SourceRange{
                source_id,
                0,
                5},
            make_cxx_region(
                vixc::SourceRange{
                    source_id,
                    12,
                    16}));

    assert(propagation->valid());

    Failure failure{
        vixc::SourceRange{
            source_id,
            6,
            16},
        vixc::SourceRange{
            source_id,
            0,
            5},
        std::move(propagation)};

    assert(failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        lowering.lower(
            failure));

    assert(!diagnostics.has_errors());
  }

  void test_invalid_operand_is_rejected()
  {
    const std::string source =
        "Error error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    auto invalid_operand =
        std::make_unique<IrNode>(
            IrKind::Invalid,
            vixc::SourceRange{
                source_id,
                6,
                11});

    Failure failure{
        vixc::SourceRange{
            source_id,
            6,
            11},
        vixc::SourceRange{
            source_id,
            0,
            5},
        std::move(invalid_operand)};

    assert(failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            failure));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3116"));
  }

  void test_program_cannot_be_failure_operand()
  {
    const std::string source =
        "Error error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    auto program =
        std::make_unique<Program>(
            vixc::SourceRange{
                source_id,
                6,
                11});

    Failure failure{
        vixc::SourceRange{
            source_id,
            6,
            11},
        vixc::SourceRange{
            source_id,
            0,
            5},
        std::move(program)};

    assert(failure.valid());

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            failure));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3117"));
  }

  void test_non_failure_node_is_rejected()
  {
    const std::string source =
        "value";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "plain.cpp",
            source);

    IrNode region{
        IrKind::CxxRegion,
        vixc::SourceRange{
            source_id,
            0,
            5}};

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            region));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3104"));
  }

  void test_program_node_is_rejected()
  {
    const std::string source =
        "value";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "plain.cpp",
            source);

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            5}};

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC3104"));
  }

  void test_existing_fatal_diagnostic_stops_lowering()
  {
    const std::string source =
        "Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    diagnostics.fatal(
        "frontend cannot continue",
        vixc::SourceRange{
            source_id,
            0,
            5});

    assert(diagnostics.has_fatal());

    Outcome outcome{
        vixc::SourceRange{
            source_id,
            0,
            5},
        vixc::SourceRange{
            source_id,
            0,
            5}};

    assert(outcome.valid());

    const std::size_t diagnostic_count =
        diagnostics.size();

    LoweringContext context{
        sources,
        diagnostics};

    FailureLowering lowering{
        context};

    assert(
        !lowering.lower(
            outcome));

    assert(
        diagnostics.size() == diagnostic_count);
  }

  void test_lowering_context_recovers_source_text()
  {
    const std::string source =
        "Error error read";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    LoweringContext context{
        sources,
        diagnostics};

    const auto text =
        context.source_text(
            vixc::SourceRange{
                source_id,
                0,
                5});

    assert(text.has_value());
    assert(*text == "Error");
  }

  void test_lowering_context_rejects_out_of_bounds_range()
  {
    const std::string source =
        "Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    LoweringContext context{
        sources,
        diagnostics};

    const auto text =
        context.source_text(
            vixc::SourceRange{
                source_id,
                0,
                100});

    assert(!text.has_value());
  }

  void test_synthetic_identifiers_are_deterministic()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    LoweringContext context{
        sources,
        diagnostics};

    assert(context.synthetic_count() == 0);

    const std::size_t first =
        context.next_synthetic_id();

    const std::size_t second =
        context.next_synthetic_id();

    const std::size_t third =
        context.next_synthetic_id();

    assert(first == 0);
    assert(second == 1);
    assert(third == 2);

    assert(context.synthetic_count() == 3);
  }

} // namespace

int main()
{
  test_success_only_outcome_is_lowered();
  test_failure_aware_outcome_is_lowered();

  test_invalid_outcome_is_rejected();
  test_outcome_requires_recoverable_failure_type_source();

  test_valid_failure_is_lowered();
  test_failure_without_operand_is_rejected();
  test_failure_requires_failure_type_source();

  test_valid_failure_propagation_is_lowered();
  test_failure_propagation_without_operand_is_rejected();
  test_failure_propagation_requires_failure_type_source();

  test_nested_failure_propagation_is_lowered();

  test_invalid_operand_is_rejected();
  test_program_cannot_be_failure_operand();

  test_non_failure_node_is_rejected();
  test_program_node_is_rejected();

  test_existing_fatal_diagnostic_stops_lowering();

  test_lowering_context_recovers_source_text();
  test_lowering_context_rejects_out_of_bounds_range();
  test_synthetic_identifiers_are_deterministic();

  return 0;
}
