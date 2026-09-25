/**
 *
 *  @file EmbeddingTest.cpp
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

#include <vixc/vixc.hpp>

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
  void test_public_header_exposes_frontend()
  {
    vixc::Frontend frontend;

    const std::string source =
        "int main() { return 0; }\n";

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source);

    assert(result.success());
    assert(!result.has_errors());

    assert(
        result.generated_output() == source);
  }

  void test_frontend_options_are_public()
  {
    vixc::FrontendOptions options;

    assert(
        options.action == vixc::FrontendAction::Emit);

    assert(
        options.backend == vixc::BackendTarget::Cxx);

    assert(options.retain_source_map);

    options.action =
        vixc::FrontendAction::Analyze;

    options.backend =
        vixc::BackendTarget::Cxx;

    options.retain_source_map =
        false;

    assert(
        options.action == vixc::FrontendAction::Analyze);

    assert(
        options.backend == vixc::BackendTarget::Cxx);

    assert(!options.retain_source_map);
  }

  void test_frontend_action_names()
  {
    assert(
        vixc::frontend_action_name(
            vixc::FrontendAction::Analyze) == std::string_view{"analyze"});

    assert(
        vixc::frontend_action_name(
            vixc::FrontendAction::Lower) == std::string_view{"lower"});

    assert(
        vixc::frontend_action_name(
            vixc::FrontendAction::Emit) == std::string_view{"emit"});
  }

  void test_backend_target_name()
  {
    assert(
        vixc::backend_target_name(
            vixc::BackendTarget::Cxx) == std::string_view{"cxx"});
  }

  void test_analyze_embedding()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source,
            options);

    assert(result.success());

    assert(
        result.action() == vixc::FrontendAction::Analyze);

    assert(
        result.backend() == vixc::BackendTarget::Cxx);

    assert(!result.has_generated_output());
    assert(result.generated_output().empty());

    assert(!result.has_errors());
  }

  void test_lower_embedding()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Lower;

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source,
            options);

    assert(result.success());

    assert(
        result.action() == vixc::FrontendAction::Lower);

    assert(!result.has_generated_output());
    assert(!result.has_errors());
  }

  void test_emit_embedding()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Emit;

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source,
            options);

    assert(result.success());

    assert(
        result.action() == vixc::FrontendAction::Emit);

    assert(result.has_generated_output());

    assert(
        result.generated_output() == source);
  }

  void test_result_boolean_conversion()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source);

    assert(result.success());

    if (!result)
      assert(false);
  }

  void test_failed_result_boolean_conversion()
  {
    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            "fail error;");

    assert(!result.success());

    if (result)
      assert(false);
  }

  void test_result_owns_generated_output()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source);

    assert(result.success());

    const std::string copied_output{
        result.generated_output()};

    assert(
        copied_output == source);

    assert(
        result.generated_output() == source);
  }

  void test_result_copy_preserves_output()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult original =
        frontend.process(
            "memory.cpp",
            source);

    assert(original.success());

    const vixc::FrontendResult copy =
        original;

    assert(copy.success());

    assert(
        copy.generated_output() == source);

    assert(
        copy.generated_output() == original.generated_output());
  }

  void test_result_move_preserves_output()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    vixc::FrontendResult original =
        frontend.process(
            "memory.cpp",
            source);

    assert(original.success());

    vixc::FrontendResult moved =
        std::move(original);

    assert(moved.success());

    assert(
        moved.generated_output() == source);
  }

  void test_result_owns_diagnostics()
  {
    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            "fail error;");

    assert(!result.success());
    assert(result.has_diagnostics());
    assert(result.has_errors());

    const std::size_t count =
        result.diagnostic_count();

    assert(count > 0);

    for (const vixc::Diagnostic &diagnostic :
         result.diagnostics())
    {
      assert(!diagnostic.message().empty());
    }

    assert(
        result.diagnostic_count() == count);
  }

  void test_diagnostic_public_api()
  {
    const vixc::SourceRange range{
        7,
        10,
        20};

    const vixc::Diagnostic diagnostic{
        vixc::DiagnosticSeverity::Error,
        "VIXC9000",
        "embedding diagnostic",
        range};

    assert(
        diagnostic.severity() == vixc::DiagnosticSeverity::Error);

    assert(diagnostic.has_code());

    assert(
        diagnostic.code() == "VIXC9000");

    assert(
        diagnostic.message() == "embedding diagnostic");

    assert(diagnostic.has_range());

    assert(
        diagnostic.range() == range);

    assert(diagnostic.is_error());
    assert(!diagnostic.is_fatal());
  }

  void test_source_location_public_api()
  {
    constexpr vixc::SourceLocation location{
        12,
        42};

    static_assert(location.valid());

    static_assert(
        location.source_id() == 12);

    static_assert(
        location.offset() == 42);

    const vixc::SourceLocation same{
        12,
        42};

    const vixc::SourceLocation different{
        12,
        43};

    assert(location == same);
    assert(location != different);
  }

  void test_source_range_public_api()
  {
    constexpr vixc::SourceRange range{
        3,
        10,
        20};

    static_assert(range.valid());

    static_assert(
        range.source_id() == 3);

    static_assert(
        range.begin_offset() == 10);

    static_assert(
        range.end_offset() == 20);

    static_assert(
        range.size() == 10);

    static_assert(!range.empty());

    assert(
        range.contains(
            vixc::SourceLocation{
                3,
                10}));

    assert(
        range.contains(
            vixc::SourceLocation{
                3,
                19}));

    assert(
        !range.contains(
            vixc::SourceLocation{
                3,
                20}));
  }

  void test_generated_source_mapping_public_api()
  {
    const vixc::GeneratedSourceMapping mapping{
        5,
        15,
        vixc::SourceRange{
            2,
            30,
            40}};

    assert(mapping.valid());

    assert(
        mapping.generated_size() == 10);

    assert(mapping.contains(5));
    assert(mapping.contains(14));
    assert(!mapping.contains(15));

    assert(
        mapping.original_range == vixc::SourceRange(
                                      2,
                                      30,
                                      40));
  }

  void test_source_mapping_is_owned_by_result()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source);

    assert(result.success());
    assert(result.has_source_mappings());

    const std::vector<vixc::GeneratedSourceMapping>
        copied_mappings =
            result.source_mappings();

    assert(!copied_mappings.empty());

    assert(
        copied_mappings.size() == result.source_mappings().size());

    assert(
        copied_mappings[0].original_range == result.source_mappings()[0].original_range);
  }

  void test_original_range_lookup()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source);

    assert(result.success());
    assert(result.has_source_mappings());

    const vixc::SourceRange *range =
        result.original_range_for(4);

    assert(range != nullptr);
    assert(range->valid());

    assert(
        range->begin_offset() == 0);

    assert(
        range->end_offset() == source.size());

    assert(
        result.original_range_for(
            source.size()) == nullptr);
  }

  void test_source_map_retention_can_be_disabled()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::FrontendOptions options;
    options.retain_source_map =
        false;

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "memory.cpp",
            source,
            options);

    assert(result.success());

    assert(
        result.generated_output() == source);

    assert(!result.has_source_mappings());
    assert(result.source_mappings().empty());
  }

  void test_frontend_works_with_logical_source_name()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "<editor-buffer>",
            source);

    assert(result.success());

    assert(
        result.generated_output() == source);
  }

  void test_frontend_works_with_empty_source()
  {
    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "<empty>",
            "");

    assert(result.success());
    assert(!result.has_errors());

    assert(!result.has_generated_output());
    assert(result.generated_output().empty());
  }

  void test_frontend_instances_are_independent()
  {
    vixc::Frontend first;
    vixc::Frontend second;

    const vixc::FrontendResult failed =
        first.process(
            "failure.vix",
            "fail error;");

    assert(!failed.success());

    const std::string source =
        "int value = 42;\n";

    const vixc::FrontendResult valid =
        second.process(
            "main.cpp",
            source);

    assert(valid.success());
    assert(!valid.has_errors());

    assert(
        valid.generated_output() == source);
  }

  void test_same_frontend_can_process_multiple_buffers()
  {
    vixc::Frontend frontend;

    const std::string first_source =
        "int first = 1;\n";

    const std::string second_source =
        "int second = 2;\n";

    const vixc::FrontendResult first =
        frontend.process(
            "first.cpp",
            first_source);

    const vixc::FrontendResult second =
        frontend.process(
            "second.cpp",
            second_source);

    assert(first.success());
    assert(second.success());

    assert(
        first.generated_output() == first_source);

    assert(
        second.generated_output() == second_source);
  }

  void test_version_public_api()
  {
    assert(
        vixc::version::string == std::string_view{"0.1.0"});

    assert(
        vixc::version::current.major == vixc::version::major);

    assert(
        vixc::version::current.minor == vixc::version::minor);

    assert(
        vixc::version::current.patch == vixc::version::patch);

    assert(
        vixc::version::at_least(
            0,
            1,
            0));

    assert(
        !vixc::version::at_least(
            1,
            0,
            0));
  }

  void test_version_comparison()
  {
    constexpr vixc::version::Version first{
        0,
        1,
        0};

    constexpr vixc::version::Version same{
        0,
        1,
        0};

    constexpr vixc::version::Version newer_patch{
        0,
        1,
        1};

    constexpr vixc::version::Version newer_minor{
        0,
        2,
        0};

    constexpr vixc::version::Version newer_major{
        1,
        0,
        0};

    static_assert(first == same);
    static_assert(first != newer_patch);

    static_assert(first < newer_patch);
    static_assert(newer_patch < newer_minor);
    static_assert(newer_minor < newer_major);

    static_assert(newer_patch > first);

    static_assert(first <= same);
    static_assert(first <= newer_patch);

    static_assert(same >= first);
    static_assert(newer_major >= first);
  }

} // namespace

int main()
{
  test_public_header_exposes_frontend();

  test_frontend_options_are_public();
  test_frontend_action_names();
  test_backend_target_name();

  test_analyze_embedding();
  test_lower_embedding();
  test_emit_embedding();

  test_result_boolean_conversion();
  test_failed_result_boolean_conversion();

  test_result_owns_generated_output();
  test_result_copy_preserves_output();
  test_result_move_preserves_output();

  test_result_owns_diagnostics();
  test_diagnostic_public_api();

  test_source_location_public_api();
  test_source_range_public_api();

  test_generated_source_mapping_public_api();
  test_source_mapping_is_owned_by_result();
  test_original_range_lookup();
  test_source_map_retention_can_be_disabled();

  test_frontend_works_with_logical_source_name();
  test_frontend_works_with_empty_source();

  test_frontend_instances_are_independent();
  test_same_frontend_can_process_multiple_buffers();

  test_version_public_api();
  test_version_comparison();

  return 0;
}
