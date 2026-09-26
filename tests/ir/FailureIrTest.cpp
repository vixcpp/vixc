/**
 *
 *  @file FailureIrTest.cpp
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

#include "../../src/ir/IrKind.hpp"
#include "../../src/ir/IrNode.hpp"
#include "../../src/ir/Program.hpp"
#include "../../src/ir/failure/Failure.hpp"
#include "../../src/ir/failure/FailureAwareFunction.hpp"
#include "../../src/ir/failure/Outcome.hpp"

#include <vixc/SourceRange.hpp>

#include <cassert>
#include <memory>
#include <string_view>

namespace
{
  using vixc::ir::IrKind;
  using vixc::ir::IrNode;
  using vixc::ir::Program;
  using vixc::ir::failure::Failure;
  using vixc::ir::failure::FailureAwareFunction;
  using vixc::ir::failure::FailurePropagation;
  using vixc::ir::failure::Outcome;
  using vixc::ir::failure::OutcomeState;
  using vixc::ir::failure::Return;

  std::unique_ptr<IrNode>
  make_cxx_region(
      vixc::SourceRange range)
  {
    return std::make_unique<IrNode>(
        IrKind::CxxRegion,
        range);
  }

  void test_ir_kind_names()
  {
    assert(
        vixc::ir::ir_kind_name(
            IrKind::Invalid) == std::string_view{"Invalid"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::Program) == std::string_view{"Program"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::CxxRegion) == std::string_view{"CxxRegion"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::FailureAwareFunction) == std::string_view{"FailureAwareFunction"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::Return) == std::string_view{"Return"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::Outcome) == std::string_view{"Outcome"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::Failure) == std::string_view{"Failure"});

    assert(
        vixc::ir::ir_kind_name(
            IrKind::FailurePropagation) == std::string_view{"FailurePropagation"});
  }

  void test_failure_kind_classification()
  {
    assert(
        !vixc::ir::ir_kind_is_failure_construct(
            IrKind::Invalid));

    assert(
        !vixc::ir::ir_kind_is_failure_construct(
            IrKind::Program));

    assert(
        !vixc::ir::ir_kind_is_failure_construct(
            IrKind::CxxRegion));

    assert(
        vixc::ir::ir_kind_is_failure_construct(
            IrKind::Outcome));

    assert(
        vixc::ir::ir_kind_is_failure_construct(
            IrKind::Failure));

    assert(
        vixc::ir::ir_kind_is_failure_construct(
            IrKind::FailurePropagation));
  }

  void test_cxx_region_kind_classification()
  {
    assert(
        vixc::ir::ir_kind_is_cxx_region(
            IrKind::CxxRegion));

    assert(
        !vixc::ir::ir_kind_is_cxx_region(
            IrKind::Outcome));

    assert(
        !vixc::ir::ir_kind_is_cxx_region(
            IrKind::Failure));
  }

  void test_default_outcome_is_success_only()
  {
    const vixc::SourceRange range{
        0,
        0,
        10};

    const Outcome outcome{
        range};

    assert(outcome.kind() == IrKind::Outcome);
    assert(outcome.range() == range);

    assert(outcome.valid());

    assert(
        outcome.allows(
            OutcomeState::Success));

    assert(outcome.allows_success());

    assert(!outcome.allows_none());
    assert(!outcome.allows_failure());
    assert(!outcome.allows_stopped());

    assert(!outcome.has_failure_contract());

    assert(
        !outcome.failure_type_range()
             .valid());
  }

  void test_failure_aware_outcome()
  {
    const vixc::SourceRange outcome_range{
        0,
        10,
        30};

    const vixc::SourceRange failure_type_range{
        0,
        16,
        21};

    const Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(outcome.kind() == IrKind::Outcome);
    assert(outcome.valid());

    assert(outcome.allows_success());
    assert(outcome.allows_failure());

    assert(!outcome.allows_none());
    assert(!outcome.allows_stopped());

    assert(outcome.has_failure_contract());

    assert(
        outcome.failure_type_range() == failure_type_range);
  }

  void test_outcome_always_allows_success()
  {
    const vixc::SourceRange range{
        0,
        0,
        10};

    const Outcome outcome{
        range};

    assert(outcome.valid());
    assert(outcome.allows_success());

    assert(
        outcome.allows(
            OutcomeState::Success));
  }

  void test_default_outcome_does_not_enable_optional_states()
  {
    const vixc::SourceRange range{
        0,
        0,
        10};

    const Outcome outcome{
        range};

    assert(outcome.valid());

    assert(outcome.allows_success());
    assert(!outcome.allows_none());
    assert(!outcome.allows_failure());
    assert(!outcome.allows_stopped());
  }

  void test_outcome_failure_type_must_be_non_empty()
  {
    const vixc::SourceRange outcome_range{
        0,
        10,
        30};

    const vixc::SourceRange failure_type_range{
        0,
        16,
        16};

    const Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(!outcome.valid());
  }

  void test_outcome_failure_type_must_use_same_source()
  {
    const vixc::SourceRange outcome_range{
        0,
        10,
        30};

    const vixc::SourceRange failure_type_range{
        1,
        16,
        21};

    const Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(!outcome.valid());
  }

  void test_outcome_failure_type_must_be_inside_outcome_range()
  {
    const vixc::SourceRange outcome_range{
        0,
        10,
        30};

    const vixc::SourceRange failure_type_range{
        0,
        31,
        36};

    const Outcome outcome{
        outcome_range,
        failure_type_range};

    assert(!outcome.valid());
  }

  void test_failure_without_operand_is_invalid()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    const Failure failure{
        failure_range,
        failure_type_range};

    assert(failure.kind() == IrKind::Failure);

    assert(
        failure.failure_type_range() == failure_type_range);

    assert(failure.operand() == nullptr);
    assert(failure.child_count() == 0);

    assert(!failure.valid());
  }

  void test_failure_with_operand_is_valid()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    const vixc::SourceRange operand_range{
        0,
        35,
        40};

    Failure failure{
        failure_range,
        failure_type_range,
        make_cxx_region(
            operand_range)};

    assert(failure.kind() == IrKind::Failure);
    assert(failure.valid());

    assert(failure.child_count() == 1);

    const IrNode *operand =
        failure.operand();

    assert(operand != nullptr);

    assert(
        operand->kind() == IrKind::CxxRegion);

    assert(
        operand->range() == operand_range);
  }

  void test_failure_set_operand()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    const vixc::SourceRange operand_range{
        0,
        35,
        40};

    Failure failure{
        failure_range,
        failure_type_range};

    IrNode *stored =
        failure.set_operand(
            make_cxx_region(
                operand_range));

    assert(stored != nullptr);
    assert(stored == failure.operand());

    assert(failure.child_count() == 1);
    assert(failure.valid());
  }

  void test_failure_rejects_second_operand()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    Failure failure{
        failure_range,
        failure_type_range};

    IrNode *first =
        failure.set_operand(
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    35,
                    40}));

    assert(first != nullptr);

    IrNode *second =
        failure.set_operand(
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    36,
                    39}));

    assert(second == nullptr);

    assert(failure.child_count() == 1);
    assert(failure.valid());
  }

  void test_failure_rejects_null_operand()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    Failure failure{
        failure_range,
        failure_type_range};

    std::unique_ptr<IrNode> operand;

    assert(
        failure.set_operand(
            std::move(operand)) == nullptr);

    assert(failure.child_count() == 0);
    assert(!failure.valid());
  }

  void test_failure_requires_valid_failure_type_range()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange invalid_type;

    Failure failure{
        failure_range,
        invalid_type,
        make_cxx_region(
            vixc::SourceRange{
                0,
                35,
                40})};

    assert(!failure.valid());
  }

  void test_failure_requires_non_empty_failure_type()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange empty_type{
        0,
        10,
        10};

    Failure failure{
        failure_range,
        empty_type,
        make_cxx_region(
            vixc::SourceRange{
                0,
                35,
                40})};

    assert(!failure.valid());
  }

  void test_failure_requires_same_source_as_contract()
  {
    const vixc::SourceRange failure_range{
        0,
        30,
        41};

    const vixc::SourceRange failure_type_range{
        1,
        10,
        15};

    Failure failure{
        failure_range,
        failure_type_range,
        make_cxx_region(
            vixc::SourceRange{
                0,
                35,
                40})};

    assert(!failure.valid());
  }

  void test_failure_type_can_be_outside_failure_statement()
  {
    const vixc::SourceRange failure_range{
        0,
        50,
        61};

    const vixc::SourceRange failure_type_range{
        0,
        20,
        25};

    Failure failure{
        failure_range,
        failure_type_range,
        make_cxx_region(
            vixc::SourceRange{
                0,
                55,
                60})};

    assert(failure.valid());

    assert(
        !failure_range.contains(
            failure_type_range.begin()));

    assert(
        failure.failure_type_range() == failure_type_range);
  }

  void test_failure_propagation_without_operand_is_invalid()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    const FailurePropagation propagation{
        propagation_range,
        failure_type_range};

    assert(
        propagation.kind() == IrKind::FailurePropagation);

    assert(propagation.operand() == nullptr);
    assert(propagation.child_count() == 0);

    assert(!propagation.valid());
  }

  void test_failure_propagation_with_operand_is_valid()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    const vixc::SourceRange operand_range{
        0,
        44,
        50};

    FailurePropagation propagation{
        propagation_range,
        failure_type_range,
        make_cxx_region(
            operand_range)};

    assert(
        propagation.kind() == IrKind::FailurePropagation);

    assert(propagation.valid());

    assert(propagation.child_count() == 1);

    const IrNode *operand =
        propagation.operand();

    assert(operand != nullptr);

    assert(
        operand->kind() == IrKind::CxxRegion);

    assert(
        operand->range() == operand_range);
  }

  void test_failure_propagation_set_operand()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    FailurePropagation propagation{
        propagation_range,
        failure_type_range};

    IrNode *stored =
        propagation.set_operand(
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    44,
                    50}));

    assert(stored != nullptr);
    assert(stored == propagation.operand());

    assert(propagation.child_count() == 1);
    assert(propagation.valid());
  }

  void test_failure_propagation_rejects_second_operand()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    FailurePropagation propagation{
        propagation_range,
        failure_type_range};

    IrNode *first =
        propagation.set_operand(
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    44,
                    50}));

    assert(first != nullptr);

    IrNode *second =
        propagation.set_operand(
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    45,
                    49}));

    assert(second == nullptr);
    assert(propagation.child_count() == 1);
    assert(propagation.valid());
  }

  void test_failure_propagation_rejects_null_operand()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        0,
        10,
        15};

    FailurePropagation propagation{
        propagation_range,
        failure_type_range};

    std::unique_ptr<IrNode> operand;

    assert(
        propagation.set_operand(
            std::move(operand)) == nullptr);

    assert(propagation.child_count() == 0);
    assert(!propagation.valid());
  }

  void test_failure_propagation_requires_valid_contract()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange invalid_type;

    FailurePropagation propagation{
        propagation_range,
        invalid_type,
        make_cxx_region(
            vixc::SourceRange{
                0,
                44,
                50})};

    assert(!propagation.valid());
  }

  void test_failure_propagation_requires_same_source()
  {
    const vixc::SourceRange propagation_range{
        0,
        40,
        50};

    const vixc::SourceRange failure_type_range{
        1,
        10,
        15};

    FailurePropagation propagation{
        propagation_range,
        failure_type_range,
        make_cxx_region(
            vixc::SourceRange{
                0,
                44,
                50})};

    assert(!propagation.valid());
  }

  void test_program_accepts_failure_ir_nodes()
  {
    Program program{
        vixc::SourceRange{
            0,
            0,
            100}};

    program.reserve(3);

    auto outcome =
        std::make_unique<Outcome>(
            vixc::SourceRange{
                0,
                10,
                21},
            vixc::SourceRange{
                0,
                16,
                21});

    auto failure =
        std::make_unique<Failure>(
            vixc::SourceRange{
                0,
                40,
                51},
            vixc::SourceRange{
                0,
                16,
                21},
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    45,
                    50}));

    auto propagation =
        std::make_unique<FailurePropagation>(
            vixc::SourceRange{
                0,
                60,
                70},
            vixc::SourceRange{
                0,
                16,
                21},
            make_cxx_region(
                vixc::SourceRange{
                    0,
                    64,
                    70}));

    assert(outcome->valid());
    assert(failure->valid());
    assert(propagation->valid());

    program.add(
        std::move(outcome));

    program.add(
        std::move(failure));

    program.add(
        std::move(propagation));

    assert(program.kind() == IrKind::Program);
    assert(program.size() == 3);
    assert(!program.empty());

    assert(program.node(0) != nullptr);
    assert(program.node(1) != nullptr);
    assert(program.node(2) != nullptr);

    assert(
        program.node(0)->kind() == IrKind::Outcome);

    assert(
        program.node(1)->kind() == IrKind::Failure);

    assert(
        program.node(2)->kind() == IrKind::FailurePropagation);
  }

  void test_ir_node_owns_children()
  {
    IrNode parent{
        IrKind::CxxRegion,
        vixc::SourceRange{
            0,
            0,
            20}};

    auto child =
        make_cxx_region(
            vixc::SourceRange{
                0,
                5,
                10});

    IrNode *stored =
        parent.add_child(
            std::move(child));

    assert(stored != nullptr);

    assert(parent.child_count() == 1);

    assert(parent.child(0) == stored);

    assert(
        parent.child(0)->range() == vixc::SourceRange(
                                        0,
                                        5,
                                        10));
  }

  void test_ir_node_without_children()
  {
    const IrNode node{
        IrKind::CxxRegion,
        vixc::SourceRange{
            0,
            0,
            10}};

    assert(node.child_count() == 0);
    assert(node.child(0) == nullptr);
  }

  void test_failure_aware_function_owns_contract_and_body()
  {
    auto outcome = std::make_unique<Outcome>(
        vixc::SourceRange{0, 20, 35},
        vixc::SourceRange{0, 26, 35});

    FailureAwareFunction function{
        vixc::SourceRange{0, 0, 70},
        vixc::SourceRange{0, 0, 3},
        vixc::SourceRange{0, 4, 19},
        vixc::SourceRange{0, 36, 70},
        std::move(outcome)};

    assert(function.kind() == IrKind::FailureAwareFunction);
    assert(function.valid());
    assert(function.success_type_range() == vixc::SourceRange(0, 0, 3));
    assert(function.declarator_range() == vixc::SourceRange(0, 4, 19));
    assert(function.body_range() == vixc::SourceRange(0, 36, 70));
    assert(function.outcome() != nullptr);
    assert(function.outcome()->failure_type_range() == vixc::SourceRange(0, 26, 35));

    function.add_child(std::make_unique<Failure>(
        vixc::SourceRange{0, 42, 49},
        vixc::SourceRange{0, 26, 35},
        make_cxx_region(vixc::SourceRange{0, 47, 48})));
    function.add_child(std::make_unique<Return>(
        vixc::SourceRange{0, 52, 61},
        make_cxx_region(vixc::SourceRange{0, 59, 60})));

    assert(function.child_count() == 2);
    assert(function.child(0)->kind() == IrKind::Failure);
    assert(function.child(1)->kind() == IrKind::Return);
  }

  void test_return_requires_one_source_owned_operand()
  {
    Return statement{
        vixc::SourceRange{0, 10, 19},
        make_cxx_region(vixc::SourceRange{0, 17, 18})};

    assert(statement.kind() == IrKind::Return);
    assert(statement.valid());
    assert(statement.operand() != nullptr);

    Return missing_operand{
        vixc::SourceRange{0, 10, 19},
        nullptr};
    assert(!missing_operand.valid());
  }

} // namespace

int main()
{
  test_ir_kind_names();
  test_failure_kind_classification();
  test_cxx_region_kind_classification();

  test_default_outcome_is_success_only();
  test_failure_aware_outcome();

  test_outcome_always_allows_success();
  test_default_outcome_does_not_enable_optional_states();

  test_outcome_failure_type_must_be_non_empty();
  test_outcome_failure_type_must_use_same_source();
  test_outcome_failure_type_must_be_inside_outcome_range();

  test_failure_without_operand_is_invalid();
  test_failure_with_operand_is_valid();

  test_failure_set_operand();
  test_failure_rejects_second_operand();
  test_failure_rejects_null_operand();

  test_failure_requires_valid_failure_type_range();
  test_failure_requires_non_empty_failure_type();
  test_failure_requires_same_source_as_contract();
  test_failure_type_can_be_outside_failure_statement();

  test_failure_propagation_without_operand_is_invalid();
  test_failure_propagation_with_operand_is_valid();

  test_failure_propagation_set_operand();
  test_failure_propagation_rejects_second_operand();
  test_failure_propagation_rejects_null_operand();

  test_failure_propagation_requires_valid_contract();
  test_failure_propagation_requires_same_source();

  test_program_accepts_failure_ir_nodes();

  test_ir_node_owns_children();
  test_ir_node_without_children();
  test_failure_aware_function_owns_contract_and_body();
  test_return_requires_one_source_owned_operand();

  return 0;
}
