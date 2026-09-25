/**
 *
 *  @file Outcome.hpp
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

#if !defined(VIXC_IR_FAILURE_OUTCOME_HPP)
#define VIXC_IR_FAILURE_OUTCOME_HPP

#include "../IrNode.hpp"

#include <vixc/SourceRange.hpp>

#include <cstdint>
#include <string_view>

namespace vixc::ir::failure
{
  /**
   * @brief Identifies one completion state represented by the VixC IR.
   *
   * OutcomeState is the IR-level representation of the distinct ways a
   * computation may complete.
   *
   * Success represents ordinary successful completion.
   *
   * None represents legitimate semantic absence. It is distinct from an
   * operational failure.
   *
   * Failure represents a recoverable failure declared by the computation's
   * failure contract.
   *
   * Stopped represents computation that was explicitly stopped or cancelled
   * before producing its normal result.
   *
   * Contract violations and programmer errors are not OutcomeState values.
   * They describe invalid program behavior rather than ordinary completion
   * states that callers are expected to handle.
   */
  enum class OutcomeState : std::uint8_t
  {
    /// Normal successful completion.
    Success,

    /// Legitimate semantic absence.
    None,

    /// Recoverable failure.
    Failure,

    /// Explicitly stopped or cancelled computation.
    Stopped
  };

  /**
   * @brief Returns the stable textual name of an IR outcome state.
   *
   * The returned value is intended for IR inspection, tests, diagnostics,
   * tracing, and development tools.
   *
   * @param state Outcome state to describe.
   *
   * @return Stable lowercase name of the outcome state.
   */
  [[nodiscard]]
  std::string_view
  outcome_state_name(OutcomeState state) noexcept;

  /**
   * @brief Represents a computation outcome contract in the VixC IR.
   *
   * Outcome is the backend-independent IR representation of the completion
   * states available to a computation after semantic analysis has established
   * its outcome contract.
   *
   * The node does not represent a C++ result type, variant, exception, union,
   * return convention, or runtime container. Those are backend implementation
   * choices that may change without changing the semantics represented here.
   *
   * Every Outcome permits successful completion.
   *
   * The first VixC failure generation additionally supports a recoverable
   * Failure state introduced by a valid `fails` specification. When Failure is
   * enabled, failure_type_range() identifies the original source spelling of
   * the declared failure type.
   *
   * None and Stopped are represented explicitly because their meanings are
   * distinct from Failure. They are not enabled by the first failure syntax and
   * remain disabled until corresponding language semantics are introduced.
   *
   * The SourceRange inherited from IrNode normally identifies the source
   * construct that established this outcome contract.
   */
  class Outcome final : public IrNode
  {
  public:
    /**
     * @brief Creates a success-only outcome contract.
     *
     * The resulting node permits only OutcomeState::Success.
     *
     * @param range Original source range associated with the outcome contract.
     */
    explicit Outcome(
        SourceRange range = {}) noexcept;

    /**
     * @brief Creates a failure-aware outcome contract.
     *
     * The resulting node permits Success and Failure.
     *
     * The node range normally covers the complete failure specification while
     * failure_type_range identifies only the declared failure type.
     *
     * The constructor preserves the supplied source ranges without consulting a
     * SourceManager. Structural invariants can be checked with valid().
     *
     * @param range Original source range of the outcome specification.
     * @param failure_type_range Original source range of the declared failure
     *        type.
     */
    Outcome(
        SourceRange range,
        SourceRange failure_type_range) noexcept;

    Outcome(const Outcome &) = delete;
    Outcome &operator=(const Outcome &) = delete;

    Outcome(Outcome &&) noexcept = default;
    Outcome &operator=(Outcome &&) noexcept = default;

    ~Outcome() override = default;

    /**
     * @brief Reports whether this IR outcome contract is structurally valid.
     *
     * A success-only outcome is valid when no failure type range is attached.
     *
     * A failure-aware outcome requires a valid, non-empty failure type range
     * belonging to the same source as the Outcome node range and contained
     * within that range.
     *
     * Source bounds are not checked here because the IR does not own source
     * storage.
     *
     * @return true when the node satisfies its structural invariants.
     */
    [[nodiscard]]
    bool valid() const noexcept;

    /**
     * @brief Reports whether a completion state is permitted.
     *
     * @param state Outcome state to query.
     *
     * @return true when the computation may complete with the requested state.
     */
    [[nodiscard]]
    bool allows(OutcomeState state) const noexcept;

    /**
     * @brief Reports whether successful completion is permitted.
     *
     * All current Outcome nodes permit Success.
     *
     * @return true when OutcomeState::Success is enabled.
     */
    [[nodiscard]]
    bool allows_success() const noexcept;

    /**
     * @brief Reports whether semantic absence is permitted.
     *
     * @return true when OutcomeState::None is enabled.
     */
    [[nodiscard]]
    bool allows_none() const noexcept;

    /**
     * @brief Reports whether recoverable failure is permitted.
     *
     * @return true when OutcomeState::Failure is enabled.
     */
    [[nodiscard]]
    bool allows_failure() const noexcept;

    /**
     * @brief Reports whether stopped execution is permitted.
     *
     * @return true when OutcomeState::Stopped is enabled.
     */
    [[nodiscard]]
    bool allows_stopped() const noexcept;

    /**
     * @brief Reports whether this outcome contains a recoverable failure
     * contract.
     *
     * @return true when Failure is an available outcome.
     */
    [[nodiscard]]
    bool has_failure_contract() const noexcept;

    /**
     * @brief Returns the source range of the declared failure type.
     *
     * The returned range is invalid for a success-only outcome.
     *
     * @return Original source range containing the failure type.
     */
    [[nodiscard]]
    SourceRange failure_type_range() const noexcept;

  private:
    /**
     * @brief Internal bit associated with successful completion.
     */
    static constexpr std::uint8_t success_bit = 1u << 0u;

    /**
     * @brief Internal bit associated with semantic absence.
     */
    static constexpr std::uint8_t none_bit = 1u << 1u;

    /**
     * @brief Internal bit associated with recoverable failure.
     */
    static constexpr std::uint8_t failure_bit = 1u << 2u;

    /**
     * @brief Internal bit associated with stopped execution.
     */
    static constexpr std::uint8_t stopped_bit = 1u << 3u;

    /**
     * @brief Converts an outcome state into its internal state bit.
     *
     * @param state Outcome state to convert.
     *
     * @return Bit representing the requested state.
     */
    [[nodiscard]]
    static std::uint8_t bit_for(OutcomeState state) noexcept;

    /// Set of completion states permitted by this outcome contract.
    std::uint8_t states_{success_bit};

    /// Original source range containing the declared recoverable failure type.
    SourceRange failure_type_range_{};
  };

} // namespace vixc::ir::failure

#endif // VIXC_IR_FAILURE_OUTCOME_HPP
