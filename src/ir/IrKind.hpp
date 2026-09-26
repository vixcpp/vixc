/**
 *
 *  @file IrKind.hpp
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

#if !defined(VIXC_IR_IR_KIND_HPP)
#define VIXC_IR_IR_KIND_HPP

#include <string_view>

namespace vixc::ir
{
  /**
   * @brief Classifies nodes in the VixC intermediate representation.
   *
   * IrKind identifies the semantic role of an IR node after syntax has been
   * parsed and the frontend has established enough meaning to represent the
   * program independently from its original spelling.
   *
   * The IR is not a syntax tree and is not generated C++. SyntaxKind describes
   * how source code was written. IrKind describes the semantic structure that
   * later lowering and backend stages operate on.
   *
   * Ordinary C++ that VixC does not semantically transform can remain represented
   * as CxxRegion. VixC-owned constructs receive explicit IR kinds so their
   * semantics survive independently from the C++ backend used by the first
   * frontend generation.
   *
   * The first IR generation contains the structural kinds required for the
   * Failure and Outcome model. Additional language areas may extend this enum as
   * their semantics become part of the frontend.
   */
  enum class IrKind
  {
    /**
     * @brief Invalid or unavailable IR.
     *
     * Invalid nodes should not reach successful lowering or backend emission.
     * They may temporarily exist while recovering from earlier frontend errors.
     */
    Invalid,

    /**
     * @brief Root node representing one semantically processed source unit.
     */
    Program,

    /**
     * @brief Ordinary C++ source preserved without VixC semantic transformation.
     *
     * The original source range identifies the bytes that can later be emitted
     * or incorporated by the C++ backend.
     */
    CxxRegion,

    /**
     * @brief Declaration-level Failure contract and ordered function body.
     */
    FailureAwareFunction,

    /**
     * @brief Value-bearing return from a failure-aware function.
     */
    Return,

    /**
     * @brief Semantic description of a computation's possible outcomes.
     *
     * Outcome nodes belong to the VixC failure model. They describe completion
     * semantics rather than a particular generated C++ container type.
     */
    Outcome,

    /**
     * @brief Explicit production of a recoverable failure outcome.
     *
     * This is the IR form produced from a semantically valid `fail` construct.
     */
    Failure,

    /**
     * @brief Propagation of a recoverable failure from another computation.
     *
     * This is the IR form produced from a semantically valid `try` construct.
     * Lowering determines how propagation is represented for the selected
     * backend.
     */
    FailurePropagation
  };

  /**
   * @brief Returns the stable textual name of an IR kind.
   *
   * The returned value is intended for diagnostics, tests, tracing, debugging,
   * IR inspection, and development tools.
   *
   * @param kind IR kind to describe.
   *
   * @return Stable textual name of the IR kind.
   */
  [[nodiscard]]
  constexpr std::string_view
  ir_kind_name(IrKind kind) noexcept
  {
    switch (kind)
    {
    case IrKind::Invalid:
      return "Invalid";

    case IrKind::Program:
      return "Program";

    case IrKind::CxxRegion:
      return "CxxRegion";

    case IrKind::FailureAwareFunction:
      return "FailureAwareFunction";

    case IrKind::Return:
      return "Return";

    case IrKind::Outcome:
      return "Outcome";

    case IrKind::Failure:
      return "Failure";

    case IrKind::FailurePropagation:
      return "FailurePropagation";
    }

    return "Unknown";
  }

  /**
   * @brief Reports whether an IR kind belongs to the failure model.
   *
   * @param kind IR kind to inspect.
   *
   * @return true when the node represents Failure or Outcome semantics.
   */
  [[nodiscard]]
  constexpr bool
  ir_kind_is_failure_construct(IrKind kind) noexcept
  {
    return kind == IrKind::Outcome || kind == IrKind::Failure || kind == IrKind::FailurePropagation;
  }

  /**
   * @brief Reports whether an IR kind represents source preserved as ordinary
   * C++.
   *
   * @param kind IR kind to inspect.
   *
   * @return true only for IrKind::CxxRegion.
   */
  [[nodiscard]]
  constexpr bool
  ir_kind_is_cxx_region(IrKind kind) noexcept
  {
    return kind == IrKind::CxxRegion;
  }

} // namespace vixc::ir

#endif // VIXC_IR_IR_KIND_HPP
