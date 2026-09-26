/**
 *
 *  @file FailureAwareFunction.hpp
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

#if !defined(VIXC_IR_FAILURE_FAILURE_AWARE_FUNCTION_HPP)
#define VIXC_IR_FAILURE_FAILURE_AWARE_FUNCTION_HPP

#include "../IrNode.hpp"
#include "Failure.hpp"
#include "Outcome.hpp"

#include <vixc/SourceRange.hpp>

#include <memory>
#include <cstddef>

namespace vixc::ir::failure
{
  /**
   * @brief Represents a declaration whose body participates in Failure semantics.
   *
   * FailureAwareFunction retains the source structure needed to later replace a
   * C++ success return type with an Outcome representation without reparsing
   * opaque source text. Its children are body elements in source order; the
   * declaration-level Outcome contract is owned separately because it describes
   * the function rather than one executable body element.
   *
   * The retained type and declarator ranges are source spellings, not resolved
   * semantic type identities. Concrete C++ Outcome lowering remains a later
   * stage.
   */
  class FailureAwareFunction final : public IrNode
  {
  public:
    /**
     * @brief Creates a failure-aware function with its declaration contract.
     *
     * @param range Complete function declaration range.
     * @param success_type_range Source spelling of the success return type.
     * @param declarator_range Source spelling from function name through
     *        parameters.
     * @param body_range Source range of the braced function body.
     * @param outcome Declared Outcome contract owned by this function.
     */
    FailureAwareFunction(
        SourceRange range,
        SourceRange success_type_range,
        SourceRange declarator_range,
        SourceRange body_range,
        std::unique_ptr<Outcome> outcome);

    /**
     * @brief Creates a function retaining its structural unqualified name.
     *
     * @param range Complete function declaration range.
     * @param success_type_range Source spelling of the success return type.
     * @param function_name_range Source spelling of the unqualified name.
     * @param declarator_range Source spelling from function name through parameters.
     * @param body_range Source range of the braced function body.
     * @param outcome Declared Outcome contract owned by this function.
     */
    FailureAwareFunction(
        SourceRange range,
        SourceRange success_type_range,
        SourceRange function_name_range,
        SourceRange declarator_range,
        SourceRange body_range,
        std::unique_ptr<Outcome> outcome);

    FailureAwareFunction(const FailureAwareFunction &) = delete;
    FailureAwareFunction &operator=(const FailureAwareFunction &) = delete;

    FailureAwareFunction(FailureAwareFunction &&) noexcept = default;
    FailureAwareFunction &operator=(FailureAwareFunction &&) noexcept = default;

    ~FailureAwareFunction() override = default;

    /**
     * @brief Returns the retained success return-type spelling range.
     *
     * @return Original source range containing the success type.
     */
    [[nodiscard]]
    SourceRange success_type_range() const noexcept;

    /**
     * @brief Returns the retained name-and-parameter declarator range.
     *
     * @return Original source range containing the unchanged declarator.
     */
    [[nodiscard]]
    SourceRange declarator_range() const noexcept;

    /** @brief Returns the structural unqualified function-name range. */
    [[nodiscard]]
    SourceRange function_name_range() const noexcept;

    /**
     * @brief Returns the braced body range.
     *
     * @return Original source range spanning the body braces and contents.
     */
    [[nodiscard]]
    SourceRange body_range() const noexcept;

    /**
     * @brief Returns the declaration-level Outcome contract.
     *
     * @return Owned Outcome contract, or nullptr when construction was invalid.
     */
    [[nodiscard]]
    const Outcome *outcome() const noexcept;

    /**
     * @brief Returns mutable access to the declaration-level Outcome contract.
     *
     * @return Owned Outcome contract, or nullptr when construction was invalid.
     */
    [[nodiscard]]
    Outcome *outcome() noexcept;

    /**
     * @brief Reports whether declaration ranges and the Outcome contract agree.
     *
     * @return true when the function has structurally valid source ownership.
     */
    [[nodiscard]]
    bool valid() const noexcept;

    void mark_lowered() noexcept;

    [[nodiscard]]
    bool is_lowered() const noexcept;

  private:
    SourceRange success_type_range_{};
    SourceRange function_name_range_{};
    SourceRange declarator_range_{};
    SourceRange body_range_{};
    std::unique_ptr<Outcome> outcome_;
    bool lowered_{false};
  };

  /**
   * @brief Represents a value-bearing return owned by a failure-aware function.
   *
   * Return preserves the original return statement and exactly one returned
   * expression operand. STEP A does not assign it a generated success Outcome;
   * that transformation belongs to later declaration-level lowering.
   */
  class Return final : public IrNode
  {
  public:
    /**
     * @brief Creates a return node with its returned expression.
     *
     * @param range Original source range of the complete return statement.
     * @param operand IR node representing the returned expression.
     */
    Return(
        SourceRange range,
        std::unique_ptr<IrNode> operand);

    Return(const Return &) = delete;
    Return &operator=(const Return &) = delete;

    Return(Return &&) noexcept = default;
    Return &operator=(Return &&) noexcept = default;

    ~Return() override = default;

    /**
     * @brief Returns the expression supplied to the return statement.
     *
     * @return Owned return expression, or nullptr when unavailable.
     */
    [[nodiscard]]
    const IrNode *operand() const noexcept;

    /**
     * @brief Returns mutable access to the returned expression.
     *
     * @return Owned return expression, or nullptr when unavailable.
     */
    [[nodiscard]]
    IrNode *operand() noexcept;

    /**
     * @brief Reports whether this return has one valid source-owned operand.
     *
     * @return true when the node satisfies STEP A structural invariants.
     */
    [[nodiscard]]
    bool valid() const noexcept;
  };

  /** @brief Structured `auto name = try expression;` propagation statement. */
  class TryInitialization final : public IrNode
  {
  public:
    TryInitialization(
        SourceRange range,
        SourceRange declaration_range,
        std::unique_ptr<FailurePropagation> propagation);

    [[nodiscard]] SourceRange declaration_range() const noexcept;
    [[nodiscard]] const FailurePropagation *propagation() const noexcept;
    [[nodiscard]] FailurePropagation *propagation() noexcept;
    void set_synthetic_id(std::size_t value) noexcept;
    [[nodiscard]] std::size_t synthetic_id() const noexcept;
    [[nodiscard]] bool has_synthetic_id() const noexcept;
    [[nodiscard]] bool valid() const noexcept;

  private:
    SourceRange declaration_range_{};
    std::size_t synthetic_id_{0};
    bool has_synthetic_id_{false};
  };

} // namespace vixc::ir::failure

#endif // VIXC_IR_FAILURE_FAILURE_AWARE_FUNCTION_HPP
