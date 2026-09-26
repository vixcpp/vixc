/**
 *
 *  @file FailurePipelineTest.cpp
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

namespace
{
  void test_plain_cpp_analyze_pipeline()
  {
    const std::string source =
        "int main() { return 0; }\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());
    assert(!result.has_fatal());

    assert(
        result.action() == vixc::FrontendAction::Analyze);

    assert(!result.has_generated_output());
    assert(result.generated_output().empty());
  }

  void test_plain_cpp_lower_pipeline()
  {
    const std::string source =
        "int main() { return 0; }\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Lower;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());

    assert(
        result.action() == vixc::FrontendAction::Lower);

    assert(!result.has_generated_output());
  }

  void test_plain_cpp_emit_pipeline()
  {
    const std::string source =
        "int main() { return 0; }\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source);

    assert(result.success());
    assert(!result.has_errors());

    assert(
        result.action() == vixc::FrontendAction::Emit);

    assert(
        result.backend() == vixc::BackendTarget::Cxx);

    assert(result.has_generated_output());

    assert(
        result.generated_output() == source);
  }

  void test_plain_cpp_emit_preserves_source_mapping()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Emit;

    options.retain_source_map =
        true;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());

    assert(
        result.generated_output() == source);

    assert(result.has_source_mappings());

    const auto &mappings =
        result.source_mappings();

    assert(mappings.size() == 1);

    const vixc::GeneratedSourceMapping &mapping =
        mappings[0];

    assert(mapping.valid());

    assert(
        mapping.generated_begin == 0);

    assert(
        mapping.generated_end == source.size());

    assert(
        mapping.generated_size() == source.size());

    assert(mapping.original_range.valid());

    assert(
        mapping.original_range.begin_offset() == 0);

    assert(
        mapping.original_range.end_offset() == source.size());
  }

  void test_source_mapping_can_be_disabled()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Emit;

    options.retain_source_map =
        false;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());

    assert(
        result.generated_output() == source);

    assert(!result.has_source_mappings());
    assert(result.source_mappings().empty());
  }

  void test_generated_offset_maps_to_original_source()
  {
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
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

  void test_failure_specification_can_be_analyzed()
  {
    const std::string source =
        "fails Error {";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());

    assert(!result.has_generated_output());
  }

  void test_fail_without_failure_contract_is_rejected()
  {
    const std::string source =
        "fail error;";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(!result.success());
    assert(result.has_errors());
    assert(!result.has_generated_output());
  }

  void test_try_without_failure_contract_is_rejected()
  {
    const std::string source =
        "try read();";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(!result.success());
    assert(result.has_errors());
    assert(!result.has_generated_output());
  }

  void test_fail_syntax_error_stops_pipeline()
  {
    const std::string source =
        "fail;";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source);

    assert(!result.success());
    assert(result.has_errors());

    assert(!result.has_generated_output());
    assert(!result.has_source_mappings());
  }

  void test_unterminated_string_stops_pipeline()
  {
    const std::string source =
        "\"unterminated";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source);

    assert(!result.success());
    assert(result.has_errors());

    assert(!result.has_generated_output());
  }

  void test_frontend_can_be_reused()
  {
    const std::string first_source =
        "int first = 1;\n";

    const std::string second_source =
        "int second = 2;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult first =
        frontend.process(
            "first.cpp",
            first_source);

    assert(first.success());

    assert(
        first.generated_output() == first_source);

    const vixc::FrontendResult second =
        frontend.process(
            "second.cpp",
            second_source);

    assert(second.success());

    assert(
        second.generated_output() == second_source);

    assert(
        first.generated_output() == first_source);
  }

  void test_failed_invocation_does_not_poison_next_invocation()
  {
    vixc::Frontend frontend;

    const vixc::FrontendResult failed =
        frontend.process(
            "failure.vix",
            "fail error;");

    assert(!failed.success());
    assert(failed.has_errors());

    const std::string valid_source =
        "int value = 42;\n";

    const vixc::FrontendResult valid =
        frontend.process(
            "main.cpp",
            valid_source);

    assert(valid.success());
    assert(!valid.has_errors());

    assert(
        valid.generated_output() == valid_source);
  }

  void test_failure_declaration_has_scoped_semantics()
  {
    const std::string source =
        "Result load() fails Error {\n"
        "  auto value = try read();\n"
        "  fail error;\n"
        "  return value;\n"
        "}\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());

    assert(!result.has_generated_output());
  }

  void test_failure_emit_uses_lowered_function_representation()
  {
    const std::string source =
        "Result load() fails Error {\n"
        "  fail error;\n"
        "  return value;\n"
        "}\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Emit;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());
    assert(result.has_generated_output());
    assert(result.generated_output().find("vixc_generated::Outcome<Result, Error>") != std::string::npos);
    assert(result.generated_output().find("::failure(error)") != std::string::npos);
    assert(result.generated_output().find("::success(value)") != std::string::npos);
  }

  void test_failure_emit_preserves_signatures_after_cpp_prelude()
  {
    const std::string source =
        "#include <iostream>\n"
        "#include <utility>\n"
        "\n"
        "enum class MathError\n"
        "{\n"
        "  DivisionByZero\n"
        "};\n"
        "\n"
        "int divide(int a, int b) fails MathError\n"
        "{\n"
        "  if (b == 0)\n"
        "  {\n"
        "    fail MathError::DivisionByZero;\n"
        "  }\n"
        "\n"
        "  return a / b;\n"
        "}\n"
        "\n"
        "int calculate() fails MathError\n"
        "{\n"
        "  auto value = try divide(10, 2);\n"
        "  return value;\n"
        "}\n"
        "\n"
        "int main()\n"
        "{\n"
        "  return 0;\n"
        "}\n";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Emit;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.cpp",
            source,
            options);

    assert(result.success());
    assert(!result.has_errors());
    assert(result.has_generated_output());

    const std::string_view output = result.generated_output();
    assert(
        output.find(
            "vixc_generated::Outcome<int, MathError>\n"
            "divide(int a, int b)") !=
        std::string_view::npos);
    assert(
        output.find(
            "vixc_generated::Outcome<int, MathError>\n"
            "calculate()") !=
        std::string_view::npos);
    assert(
        output.find("auto __vixc_outcome_0 = divide(10, 2);") !=
        std::string_view::npos);
    assert(
        output.find("auto value = std::move(__vixc_outcome_0).take_value();") !=
        std::string_view::npos);
  }

  void test_result_keeps_diagnostics_after_frontend_returns()
  {
    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            "fail error;");

    assert(!result.success());
    assert(result.has_diagnostics());
    assert(result.has_errors());

    assert(result.diagnostic_count() > 0);

    for (const vixc::Diagnostic &diagnostic :
         result.diagnostics())
    {
      assert(!diagnostic.message().empty());
    }
  }

  void test_failure_diagnostic_has_source_range()
  {
    const std::string source =
        "fail error;";

    vixc::Frontend frontend;

    vixc::FrontendOptions options;
    options.action =
        vixc::FrontendAction::Analyze;

    const vixc::FrontendResult result =
        frontend.process(
            "failure.vix",
            source,
            options);

    assert(!result.success());
    assert(result.has_errors());

    bool found_ranged_error = false;

    for (const vixc::Diagnostic &diagnostic :
         result.diagnostics())
    {
      if (
          diagnostic.is_error() && diagnostic.has_range())
      {
        found_ranged_error = true;

        assert(
            diagnostic.range().source_id() == 0);

        assert(
            diagnostic.range().end_offset() <= source.size());

        break;
      }
    }

    assert(found_ranged_error);
  }

} // namespace

int main()
{
  test_plain_cpp_analyze_pipeline();
  test_plain_cpp_lower_pipeline();
  test_plain_cpp_emit_pipeline();

  test_plain_cpp_emit_preserves_source_mapping();
  test_source_mapping_can_be_disabled();
  test_generated_offset_maps_to_original_source();

  test_failure_specification_can_be_analyzed();

  test_fail_without_failure_contract_is_rejected();
  test_try_without_failure_contract_is_rejected();

  test_fail_syntax_error_stops_pipeline();
  test_unterminated_string_stops_pipeline();

  test_frontend_can_be_reused();
  test_failed_invocation_does_not_poison_next_invocation();

  test_failure_declaration_has_scoped_semantics();
  test_failure_emit_uses_lowered_function_representation();
  test_failure_emit_preserves_signatures_after_cpp_prelude();

  test_result_keeps_diagnostics_after_frontend_returns();
  test_failure_diagnostic_has_source_range();

  return 0;
}
