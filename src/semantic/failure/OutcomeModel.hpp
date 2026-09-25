/**
 *
 *  @file OutcomeModel.hpp
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

#if !defined(VIXC_SEMANTIC_FAILURE_OUTCOME_MODEL_HPP)
#define VIXC_SEMANTIC_FAILURE_OUTCOME_MODEL_HPP

#include <vixc/SourceRange.hpp>

#include <cstdint>
#include <string_view>

namespace vixc::semantic::failure
{
  /**
   * @brief Identifies one semantic outcome of a computation.
   *
   * VixC distinguishes outcomes according to their meaning rather than treating
   * every non-value state as the same form of failure.
   *
   * Success represents normal completion with the operation's result.
   *
   * None represents legitimate absence when absence is part of the operation's
   * declared semantics. It is not an operational failure.
   *
   * Failure represents a recoverable domain or operational failure described by
   * the computation's failure contract.
   *
   * Stopped represents computation that did not complete because execution was
   * explicitly stopped or cancelled.
   *
   * Programmer errors and contract violations are not OutcomeKind values. They
   * represent invalid program behavior rather than ordinary outcomes that callers
   * are expected to handle through the outcome model.
   */
  enum class OutcomeKind : std::uint8_t
  {
    /// Normal successful completion.
    Success,

    /// Legitimate semantic absence.
    None,

    /// Recoverable failure declared by the computation.
    Failure,

    /// Explicitly stopped or cancelled computation.
    Stopped
  };

  /**
   * @brief Returns the stable textual name of an outcome kind.
   *
   * The returned name is intended for diagnostics, tests, tracing, and
   * semantic debugging.
   *
   * @param kind Outcome kind to describe.
   *
   * @return Stable lowercase name of the outcome kind.
   */
  [[nodiscard]]
  std::string_view
  outcome_kind_name(OutcomeKind kind) noexcept;

  /**
   * @brief Describes the outcomes permitted by one computation.
   *
   * OutcomeModel is the semantic representation of an operation's completion
   * contract. It records which outcome categories are available independently
   * from the syntax used to declare them and independently from how a backend
   * eventually represents them.
   *
   * The first VixC generation uses two forms of this model.
   *
   * An ordinary computation has only the Success outcome.
   *
   * A computation declared with `fails E` has Success and Failure outcomes and
   * records both the complete failure specification range and the source range
   * of the declared failure type.
   *
   * None and Stopped already exist as semantic outcome categories because they
   * have meanings distinct from Failure. They are not enabled by the first
   * failure specification syntax and therefore remain unavailable in the
   * initial model unless future language semantics explicitly introduce them.
   *
   * OutcomeModel intentionally does not store generated C++ types or backend
   * representation details. A backend may encode these outcomes differently
   * without changing their VixC meaning.
   */
  class OutcomeModel final
  {
  public:
    /**
     * @brief Creates an ordinary success-only outcome model.
     *
     * The resulting model permits OutcomeKind::Success and has no failure
     * contract.
     */
    OutcomeModel() noexcept;

    /**
     * @brief Creates a failure-aware outcome model.
     *
     * The resulting model permits Success and Failure. The source ranges retain
     * the declaration that introduced the failure contract so semantic
     * diagnostics and later frontend stages can refer to the original program.
     *
     * This constructor preserves the supplied ranges without performing source
     * manager bounds validation. valid() verifies their structural relationship.
     *
     * @param specification_range Complete source range of the `fails`
     *        specification.
     * @param failure_type_range Source range containing the declared failure
     *        type.
     */
    OutcomeModel(
        SourceRange specification_range,
        SourceRange failure_type_range) noexcept;

    /**
     * @brief Reports whether the model is structurally valid.
     *
     * A success-only model is always structurally valid.
     *
     * A failure-aware model requires valid specification and failure-type
     * ranges. Both ranges must belong to the same source, the failure type must
     * be non-empty, and the type range must lie completely inside the failure
     * specification range.
     *
     * SourceManager bounds are not checked here.
     *
     * @return true when the model satisfies its structural invariants.
     */
    [[nodiscard]]
    bool valid() const noexcept;

    /**
     * @brief Reports whether a semantic outcome is permitted by this model.
     *
     * @param kind Outcome category to query.
     *
     * @return true when the computation may produce the requested outcome.
     */
    [[nodiscard]]
    bool allows(OutcomeKind kind) const noexcept;

    /**
     * @brief Reports whether successful completion is permitted.
     *
     * All current VixC outcome models permit successful completion.
     *
     * @return true when OutcomeKind::Success is available.
     */
    [[nodiscard]]
    bool allows_success() const noexcept;

    /**
     * @brief Reports whether legitimate absence is an available outcome.
     *
     * The first failure model does not enable None, but the query is part of the
     * semantic model so future absence semantics remain distinct from Failure.
     *
     * @return true when OutcomeKind::None is available.
     */
    [[nodiscard]]
    bool allows_none() const noexcept;

    /**
     * @brief Reports whether recoverable failure is an available outcome.
     *
     * @return true when the model contains a declared failure contract.
     */
    [[nodiscard]]
    bool allows_failure() const noexcept;

    /**
     * @brief Reports whether stopped execution is an available outcome.
     *
     * The first failure model does not enable Stopped. Cancellation semantics
     * may introduce it independently in a later frontend generation.
     *
     * @return true when OutcomeKind::Stopped is available.
     */
    [[nodiscard]]
    bool allows_stopped() const noexcept;

    /**
     * @brief Reports whether the computation has a failure contract.
     *
     * This is equivalent to allows_failure() and expresses the query in terms
     * of the declaration model used by semantic analysis.
     *
     * @return true when a recoverable failure contract is present.
     */
    [[nodiscard]]
    bool has_failure_contract() const noexcept;

    /**
     * @brief Returns the complete source range of the failure specification.
     *
     * The range is invalid for a success-only model.
     *
     * @return Source range containing the `fails` specification.
     */
    [[nodiscard]]
    SourceRange failure_specification_range() const noexcept;

    /**
     * @brief Returns the source range of the declared failure type.
     *
     * The range is invalid when the model has no failure contract.
     *
     * @return Source range containing the failure type spelling.
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
     * @brief Converts an OutcomeKind into its internal state bit.
     *
     * @param kind Outcome category to convert.
     *
     * @return Bit representing the requested outcome.
     */
    [[nodiscard]]
    static std::uint8_t bit_for(OutcomeKind kind) noexcept;

    /// Set of semantic outcomes permitted by this computation.
    std::uint8_t outcomes_{success_bit};

    /// Complete source region declaring the failure contract.
    SourceRange failure_specification_range_{};

    /// Source region containing the declared failure type.
    SourceRange failure_type_range_{};
  };

} // namespace vixc::semantic::failure

#endif // VIXC_SEMANTIC_FAILURE_OUTCOME_MODEL_HPP
