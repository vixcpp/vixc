/**
 *
 *  @file CxxBackendTest.cpp
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

#include "../../src/backends/cxx/CxxBackend.hpp"
#include "../../src/backends/cxx/CxxSourceMap.hpp"
#include "../../src/diagnostics/DiagnosticEngine.hpp"
#include "../../src/ir/IrKind.hpp"
#include "../../src/ir/IrNode.hpp"
#include "../../src/ir/Program.hpp"
#include "../../src/ir/failure/Failure.hpp"
#include "../../src/ir/failure/Outcome.hpp"
#include "../../src/source/SourceManager.hpp"

#include <vixc/Diagnostic.hpp>
#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/SourceRange.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
  using vixc::backends::cxx::CxxBackend;
  using vixc::backends::cxx::CxxSourceMap;
  using vixc::ir::IrKind;
  using vixc::ir::IrNode;
  using vixc::ir::Program;
  using vixc::ir::failure::Failure;
  using vixc::ir::failure::FailurePropagation;
  using vixc::ir::failure::Outcome;

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
      std::string_view code)
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

  void test_backend_name()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.name() == std::string_view{"cxx"});
  }

  void test_new_backend_is_empty()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    CxxBackend backend{
        sources,
        diagnostics};

    assert(backend.empty());
    assert(backend.output().empty());
    assert(backend.source_map().empty());
  }

  void test_plain_cpp_region_is_emitted_verbatim()
  {
    const std::string source =
        "int main() { return 0; }\n";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                source.size()}));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(!diagnostics.has_errors());

    assert(!backend.empty());

    assert(
        backend.output() == source);
  }

  void test_plain_cpp_region_creates_source_mapping()
  {
    const std::string source =
        "int value = 42;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    const vixc::SourceRange range{
        source_id,
        0,
        source.size()};

    Program program{
        range};

    program.add(
        make_cxx_region(
            range));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    const CxxSourceMap &source_map =
        backend.source_map();

    assert(source_map.size() == 1);
    assert(!source_map.empty());

    const auto &entries =
        source_map.entries();

    assert(entries.size() == 1);

    const auto &entry =
        entries[0];

    assert(entry.valid());

    assert(
        entry.generated_begin() == 0);

    assert(
        entry.generated_end() == source.size());

    assert(
        entry.generated_size() == source.size());

    assert(
        entry.original_range() == range);
  }

  void test_source_map_lookup()
  {
    const std::string source =
        "abcdef";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    const vixc::SourceRange range{
        source_id,
        0,
        source.size()};

    Program program{
        range};

    program.add(
        make_cxx_region(
            range));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    const CxxSourceMap &source_map =
        backend.source_map();

    const auto *entry =
        source_map.find(2);

    assert(entry != nullptr);

    assert(
        entry->original_range() == range);

    assert(source_map.find(source.size()) == nullptr);

    const auto original =
        source_map.original_range_for(3);

    assert(original.has_value());
    assert(*original == range);

    assert(
        !source_map
             .original_range_for(
                 source.size())
             .has_value());
  }

  void test_multiple_cpp_regions_are_emitted_in_order()
  {
    const std::string source =
        "first(); second();";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    const vixc::SourceRange first_range{
        source_id,
        0,
        8};

    const vixc::SourceRange second_range{
        source_id,
        8,
        source.size()};

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        make_cxx_region(
            first_range));

    program.add(
        make_cxx_region(
            second_range));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(
        backend.output() == source);

    assert(
        backend.source_map().size() == 2);

    const auto &entries =
        backend.source_map().entries();

    assert(
        entries[0].generated_begin() == 0);

    assert(
        entries[0].generated_end() == 8);

    assert(
        entries[0].original_range() == first_range);

    assert(
        entries[1].generated_begin() == 8);

    assert(
        entries[1].generated_end() == source.size());

    assert(
        entries[1].original_range() == second_range);
  }

  void test_empty_cpp_region_is_allowed()
  {
    const std::string source;

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "empty.cpp",
            source);

    const vixc::SourceRange range{
        source_id,
        0,
        0};

    Program program{
        range};

    program.add(
        make_cxx_region(
            range));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(!diagnostics.has_errors());

    assert(backend.output().empty());
    assert(backend.source_map().empty());
  }

  void test_out_of_bounds_cpp_region_is_rejected()
  {
    const std::string source =
        "int value = 42;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                source.size() + 100}));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4008"));
  }

  void test_invalid_cpp_region_is_rejected()
  {
    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    Program program;

    program.add(
        make_cxx_region(
            vixc::SourceRange{}));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4007"));
  }

  void test_success_only_outcome_emits_no_text()
  {
    const std::string source =
        "value";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    auto outcome =
        std::make_unique<Outcome>(
            vixc::SourceRange{
                source_id,
                0,
                source.size()});

    assert(outcome->valid());

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        std::move(outcome));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(!diagnostics.has_errors());

    assert(backend.output().empty());
  }

  void test_failure_aware_outcome_emits_no_standalone_text()
  {
    const std::string source =
        "fails Error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    const vixc::SourceRange outcome_range{
        source_id,
        0,
        source.size()};

    const vixc::SourceRange failure_type_range{
        source_id,
        6,
        11};

    auto outcome =
        std::make_unique<Outcome>(
            outcome_range,
            failure_type_range);

    assert(outcome->valid());
    assert(outcome->allows_failure());

    Program program{
        outcome_range};

    program.add(
        std::move(outcome));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(!diagnostics.has_errors());

    assert(backend.output().empty());
  }

  void test_failure_aware_outcome_requires_source_text()
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
        100};

    const vixc::SourceRange failure_type_range{
        source_id,
        50,
        55};

    auto outcome =
        std::make_unique<Outcome>(
            outcome_range,
            failure_type_range);

    assert(outcome->valid());

    Program program{
        outcome_range};

    program.add(
        std::move(outcome));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4010"));
  }

  void test_failure_requires_declaration_level_lowering()
  {
    const std::string source =
        "Error error";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "failure.vix",
            source);

    auto failure =
        std::make_unique<Failure>(
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
                    11}));

    assert(failure->valid());

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        std::move(failure));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4013"));
  }

  void test_failure_propagation_requires_control_flow_lowering()
  {
    const std::string source =
        "Error read";

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
                    10}));

    assert(propagation->valid());

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        std::move(propagation));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4016"));
  }

  void test_invalid_ir_node_is_rejected()
  {
    const std::string source =
        "value";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    auto invalid =
        std::make_unique<IrNode>(
            IrKind::Invalid,
            vixc::SourceRange{
                source_id,
                0,
                source.size()});

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        std::move(invalid));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(diagnostics.has_errors());

    assert(
        has_diagnostic_code(
            diagnostics,
            "VIXC4001"));
  }

  void test_existing_error_prevents_generation()
  {
    const std::string source =
        "int value = 42;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                source.size()}));

    diagnostics.emit(
        vixc::DiagnosticSeverity::Error,
        "preexisting frontend error");

    assert(diagnostics.has_errors());

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        !backend.generate(
            program));

    assert(backend.empty());
    assert(backend.output().empty());
  }

  void test_generate_resets_previous_output()
  {
    const std::string first_source =
        "first();";

    const std::string second_source =
        "second();";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto first_source_id =
        sources.add_source(
            "first.cpp",
            first_source);

    const auto second_source_id =
        sources.add_source(
            "second.cpp",
            second_source);

    Program first_program{
        vixc::SourceRange{
            first_source_id,
            0,
            first_source.size()}};

    first_program.add(
        make_cxx_region(
            vixc::SourceRange{
                first_source_id,
                0,
                first_source.size()}));

    Program second_program{
        vixc::SourceRange{
            second_source_id,
            0,
            second_source.size()}};

    second_program.add(
        make_cxx_region(
            vixc::SourceRange{
                second_source_id,
                0,
                second_source.size()}));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            first_program));

    assert(
        backend.output() == first_source);

    assert(
        backend.source_map().size() == 1);

    assert(
        backend.generate(
            second_program));

    assert(
        backend.output() == second_source);

    assert(
        backend.source_map().size() == 1);

    const auto &entry =
        backend.source_map().entries()[0];

    assert(
        entry.original_range().source_id() == second_source_id);
  }

  void test_reset_clears_output_and_source_map()
  {
    const std::string source =
        "int value = 42;";

    vixc::source::SourceManager sources;
    vixc::diagnostics::DiagnosticEngine diagnostics;

    const auto source_id =
        sources.add_source(
            "main.cpp",
            source);

    Program program{
        vixc::SourceRange{
            source_id,
            0,
            source.size()}};

    program.add(
        make_cxx_region(
            vixc::SourceRange{
                source_id,
                0,
                source.size()}));

    CxxBackend backend{
        sources,
        diagnostics};

    assert(
        backend.generate(
            program));

    assert(!backend.empty());
    assert(!backend.source_map().empty());

    backend.reset();

    assert(backend.empty());
    assert(backend.output().empty());
    assert(backend.source_map().empty());
  }

} // namespace

int main()
{
  test_backend_name();
  test_new_backend_is_empty();

  test_plain_cpp_region_is_emitted_verbatim();
  test_plain_cpp_region_creates_source_mapping();
  test_source_map_lookup();

  test_multiple_cpp_regions_are_emitted_in_order();
  test_empty_cpp_region_is_allowed();

  test_out_of_bounds_cpp_region_is_rejected();
  test_invalid_cpp_region_is_rejected();

  test_success_only_outcome_emits_no_text();
  test_failure_aware_outcome_emits_no_standalone_text();
  test_failure_aware_outcome_requires_source_text();

  test_failure_requires_declaration_level_lowering();
  test_failure_propagation_requires_control_flow_lowering();

  test_invalid_ir_node_is_rejected();
  test_existing_error_prevents_generation();

  test_generate_resets_previous_output();
  test_reset_clears_output_and_source_map();

  return 0;
}
