/**
 *
 *  @file FailureLowering.hpp
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

#if !defined(VIXC_LOWERING_FAILURE_FAILURE_LOWERING_HPP)
#define VIXC_LOWERING_FAILURE_FAILURE_LOWERING_HPP

#include "../LoweringContext.hpp"
#include <vixc/SourceRange.hpp>

namespace vixc::ir
{

  class IrNode;

} // namespace vixc::ir

namespace vixc::ir::failure
{

  class Outcome;
  class Failure;
  class FailurePropagation;

} // namespace vixc::ir::failure

namespace vixc::lowering::failure
{

  /**
   * @brief Lowers the VixC Failure and Outcome semantic IR.
   *
   * FailureLowering owns the backend-independent lowering rules associated with
   * recoverable failure semantics.
   *
   * The semantic IR already describes what a failure-aware computation means.
   * This lowering stage verifies that those semantics are represented by a
   * complete and internally consistent IR structure before the program reaches
   * a concrete backend.
   *
   * FailureLowering does not choose a C++ representation for Outcome, Failure,
   * or FailurePropagation. It does not generate C++ expressions, temporary
   * variables, helper types, branching code, or runtime support. Those choices
   * belong to the backend that consumes the lowered semantic representation.
   *
   * The first VixC generation keeps Failure and Outcome as explicit semantic IR
   * nodes through this stage. Lowering therefore establishes backend-ready
   * invariants rather than erasing the semantic distinction prematurely.
   *
   * An Outcome node must describe a structurally valid completion contract.
   *
   * A Failure node must contain one valid failure value operand and a valid
   * reference to the failure type declared by the enclosing outcome contract.
   *
   * A FailurePropagation node must contain one computation operand and retain
   * the enclosing failure type through which recoverable failure will be
   * propagated.
   *
   * Operand nodes are recursively checked when they contain other VixC
   * failure-related IR constructs. Ordinary C++ regions are preserved without
   * interpretation so the C++ backend can emit their original source text.
   *
   * Lowering failures are emitted through the DiagnosticEngine owned by
   * LoweringContext. This class does not print or format diagnostics.
   */
  class FailureLowering final
  {
  public:
    /**
     * @brief Creates a failure lowering pass.
     *
     * The supplied LoweringContext must remain alive for the complete lifetime
     * of this object.
     *
     * @param context Shared lowering state for the frontend operation.
     */
    explicit FailureLowering(
        LoweringContext &context) noexcept;

    FailureLowering(const FailureLowering &) = delete;
    FailureLowering &operator=(const FailureLowering &) = delete;

    FailureLowering(FailureLowering &&) = delete;
    FailureLowering &operator=(FailureLowering &&) = delete;

    ~FailureLowering() = default;

    /**
     * @brief Lowers one failure-related IR node.
     *
     * The node must represent Outcome, Failure, or FailurePropagation.
     *
     * The concrete IR object is verified against its IrKind before specialized
     * lowering is performed. This prevents a malformed IR hierarchy from being
     * treated as valid merely because its kind value matches a failure
     * construct.
     *
     * @param node Failure-related IR node to lower.
     *
     * @return true when the node is ready for backend processing, otherwise
     *         false.
     */
    [[nodiscard]]
    bool lower(ir::IrNode &node);

  private:
    /**
     * @brief Lowers an Outcome contract.
     *
     * The first generation preserves Outcome as a semantic node for the backend
     * and therefore performs structural validation without replacing it with a
     * backend-specific representation.
     *
     * @param outcome Outcome node to lower.
     *
     * @return true when the outcome contract is structurally valid.
     */
    [[nodiscard]]
    bool lower_outcome(
        ir::failure::Outcome &outcome);

    /**
     * @brief Lowers an explicit recoverable failure.
     *
     * The Failure node must satisfy its structural invariants and contain one
     * valid operand. The operand is recursively prepared when it represents
     * another VixC semantic construct.
     *
     * @param failure Failure node to lower.
     *
     * @return true when the failure operation is ready for backend processing.
     */
    [[nodiscard]]
    bool lower_failure(
        ir::failure::Failure &failure);

    /**
     * @brief Lowers a recoverable failure propagation operation.
     *
     * The propagation node must contain exactly one valid computation operand
     * and preserve the enclosing failure contract.
     *
     * @param propagation Failure propagation node to lower.
     *
     * @return true when propagation is ready for backend processing.
     */
    [[nodiscard]]
    bool lower_failure_propagation(
        ir::failure::FailurePropagation &propagation);

    /**
     * @brief Prepares an operand owned by a failure construct.
     *
     * CxxRegion operands already represent backend-consumable preserved source
     * and require no common transformation.
     *
     * Nested failure constructs are recursively dispatched through this
     * FailureLowering instance.
     *
     * Other IR kinds are accepted when they represent semantic nodes that a
     * later common lowering stage or backend can process independently.
     *
     * @param operand Operand IR node to prepare.
     *
     * @return true when the operand can continue through the lowering pipeline.
     */
    [[nodiscard]]
    bool lower_operand(ir::IrNode &operand);

    /**
     * @brief Reports a failure-lowering diagnostic.
     *
     * @param code Stable lowering diagnostic code.
     * @param message Human-readable diagnostic message.
     * @param range Original source range associated with the failure.
     *
     * @return false so validation paths can directly return the result.
     */
    bool report_error(
        const char *code,
        const char *message,
        SourceRange range);

    /// Shared state for the current lowering operation.
    LoweringContext &context_;
  };

} // namespace vixc::lowering::failure

#endif // VIXC_LOWERING_FAILURE_FAILURE_LOWERING_HPP
