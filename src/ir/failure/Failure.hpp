/**
 *
 *  @file Failure.hpp
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

#if !defined(VIXC_IR_FAILURE_FAILURE_HPP)
#define VIXC_IR_FAILURE_FAILURE_HPP

#include "../IrNode.hpp"

#include <vixc/SourceRange.hpp>

#include <memory>

namespace vixc::ir::failure
{
  /**
   * @brief Represents explicit production of a recoverable failure.
   *
   * Failure is the backend-independent IR form of a semantically valid `fail`
   * construct.
   *
   * The node represents one precise semantic operation: the current
   * failure-aware computation completes with its declared recoverable Failure
   * outcome.
   *
   * Failure is not an exception, return statement, result container, tagged
   * union, or generated C++ expression. Those are possible implementation
   * strategies for later lowering and backend stages and must not define the
   * meaning of this IR node.
   *
   * A Failure node normally owns exactly one child representing the expression
   * that produces the failure value. In the first frontend generation that
   * expression may still be represented by a CxxRegion when VixC does not need
   * to model its complete C++ expression semantics.
   *
   * failure_type_range() identifies the declared failure type associated with
   * the surrounding failure contract. This preserves the relationship
   * established during semantic analysis without embedding generated C++ type
   * representations into the common IR.
   *
   * The SourceRange inherited from IrNode identifies the original `fail`
   * construct.
   */
  class Failure final : public IrNode
  {
  public:
    /**
     * @brief Creates a failure node without an operand.
     *
     * This form is useful during staged IR construction. valid() remains false
     * until exactly one operand has been attached.
     *
     * @param range Original source range of the `fail` construct.
     * @param failure_type_range Source range of the failure type declared by
     *        the active failure contract.
     */
    Failure(
        SourceRange range,
        SourceRange failure_type_range) noexcept;

    /**
     * @brief Creates a complete failure node with its failure value operand.
     *
     * Ownership of the operand is transferred to this node.
     *
     * @param range Original source range of the `fail` construct.
     * @param failure_type_range Source range of the failure type declared by
     *        the active failure contract.
     * @param operand IR node representing the failure value expression.
     */
    Failure(
        SourceRange range,
        SourceRange failure_type_range,
        std::unique_ptr<IrNode> operand);

    Failure(const Failure &) = delete;
    Failure &operator=(const Failure &) = delete;

    Failure(Failure &&) noexcept = default;
    Failure &operator=(Failure &&) noexcept = default;

    ~Failure() override = default;

    /**
     * @brief Attaches the failure value operand.
     *
     * A Failure node is expected to contain exactly one operand. This function
     * succeeds only while no operand has already been attached.
     *
     * Ownership of the supplied node is transferred to this Failure.
     *
     * @param operand IR node representing the failure value.
     *
     * @return Pointer to the stored operand, or nullptr when operand is null or
     *         an operand is already present.
     */
    IrNode *set_operand(std::unique_ptr<IrNode> operand);

    /**
     * @brief Returns the failure value operand.
     *
     * @return Pointer to the operand, or nullptr when none has been attached.
     */
    [[nodiscard]]
    const IrNode *operand() const noexcept;

    /**
     * @brief Returns mutable access to the failure value operand.
     *
     * Ownership remains with this Failure node.
     *
     * @return Pointer to the operand, or nullptr when none has been attached.
     */
    [[nodiscard]]
    IrNode *operand() noexcept;

    /**
     * @brief Returns the source range of the declared failure type.
     *
     * The range originates from the failure contract active when this Failure
     * node was created.
     *
     * @return Original source range containing the declared failure type.
     */
    [[nodiscard]]
    SourceRange failure_type_range() const noexcept;

    /**
     * @brief Reports whether this failure node satisfies its structural
     * invariants.
     *
     * A valid Failure requires a valid source range, a valid and non-empty
     * failure type range belonging to the same source, and exactly one operand.
     *
     * The failure type range does not need to lie inside the `fail` source
     * range because it normally belongs to the surrounding declaration's
     * `fails` specification.
     *
     * SourceManager bounds and type compatibility are not checked here.
     *
     * @return true when the node is structurally valid.
     */
    [[nodiscard]]
    bool valid() const noexcept;

  private:
    /// Source range of the failure type declared by the active contract.
    SourceRange failure_type_range_{};
  };

  /**
   * @brief Represents propagation of a recoverable failure.
   *
   * FailurePropagation is the backend-independent IR form of a semantically
   * valid VixC `try` expression.
   *
   * The operation evaluates one operand. Successful completion exposes the
   * operand's successful value to the surrounding computation. A recoverable
   * failure is propagated through the active failure contract instead of being
   * converted into an unrelated control-flow mechanism.
   *
   * This node records the semantic request to propagate failure. It does not
   * specify whether a C++ backend implements propagation with branching,
   * helper functions, generated temporaries, macros, exceptions, or another
   * representation. Lowering owns that decision.
   *
   * The node normally owns exactly one operand representing the computation
   * being evaluated.
   *
   * failure_type_range() identifies the failure type of the enclosing
   * computation after semantic compatibility has been established.
   *
   * The SourceRange inherited from IrNode identifies the original `try`
   * expression.
   */
  class FailurePropagation final : public IrNode
  {
  public:
    /**
     * @brief Creates a propagation node without an operand.
     *
     * valid() remains false until exactly one operand is attached.
     *
     * @param range Original source range of the `try` expression.
     * @param failure_type_range Source range of the enclosing failure type.
     */
    FailurePropagation(
        SourceRange range,
        SourceRange failure_type_range) noexcept;

    /**
     * @brief Creates a complete propagation node.
     *
     * Ownership of the operand is transferred to this node.
     *
     * @param range Original source range of the `try` expression.
     * @param failure_type_range Source range of the enclosing failure type.
     * @param operand IR node representing the propagated computation.
     */
    FailurePropagation(
        SourceRange range,
        SourceRange failure_type_range,
        std::unique_ptr<IrNode> operand);

    FailurePropagation(const FailurePropagation &) = delete;
    FailurePropagation &operator=(const FailurePropagation &) = delete;

    FailurePropagation(FailurePropagation &&) noexcept = default;
    FailurePropagation &operator=(FailurePropagation &&) noexcept = default;

    ~FailurePropagation() override = default;

    /**
     * @brief Attaches the computation whose failure may be propagated.
     *
     * A FailurePropagation node is expected to contain exactly one operand.
     * The operation succeeds only while no operand is already attached.
     *
     * @param operand IR node representing the propagated computation.
     *
     * @return Pointer to the stored operand, or nullptr when operand is null or
     *         an operand is already present.
     */
    IrNode *set_operand(std::unique_ptr<IrNode> operand);

    /**
     * @brief Returns the propagated computation operand.
     *
     * @return Pointer to the operand, or nullptr when none has been attached.
     */
    [[nodiscard]]
    const IrNode *operand() const noexcept;

    /**
     * @brief Returns mutable access to the propagated computation operand.
     *
     * Ownership remains with this FailurePropagation node.
     *
     * @return Pointer to the operand, or nullptr when none has been attached.
     */
    [[nodiscard]]
    IrNode *operand() noexcept;

    /**
     * @brief Returns the source range of the enclosing failure type.
     *
     * @return Original source range containing the declared failure type.
     */
    [[nodiscard]]
    SourceRange failure_type_range() const noexcept;

    /**
     * @brief Reports whether this propagation node satisfies its structural
     * invariants.
     *
     * A valid FailurePropagation requires a valid source range, a valid and
     * non-empty failure type range belonging to the same source, and exactly
     * one operand.
     *
     * Compatibility between the operand's failure contract and the enclosing
     * failure contract is a semantic invariant established before IR creation
     * and is not recomputed from source spelling here.
     *
     * @return true when the node is structurally valid.
     */
    [[nodiscard]]
    bool valid() const noexcept;

  private:
    /// Source range of the enclosing computation's declared failure type.
    SourceRange failure_type_range_{};
  };

} // namespace vixc::ir::failure

#endif // VIXC_IR_FAILURE_FAILURE_HPP
