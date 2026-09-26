/**
 *
 *  @file FailureAnalyzer.hpp
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

#if !defined(VIXC_SEMANTIC_FAILURE_FAILURE_ANALYZER_HPP)
#define VIXC_SEMANTIC_FAILURE_FAILURE_ANALYZER_HPP

#include "OutcomeModel.hpp"

#include "../SemanticContext.hpp"

#include <optional>
#include <string>

namespace vixc::syntax
{

  class SyntaxNode;

} // namespace vixc::syntax

namespace vixc::semantic::failure
{
  /**
   * @brief Performs semantic validation of VixC failure constructs.
   *
   * FailureAnalyzer owns the semantic rules introduced by the VixC failure
   * model. It validates failure specifications, explicit failure statements,
   * and failure propagation expressions after their source structure has been
   * established by the parser.
   *
   * The analyzer deliberately operates on semantic meaning rather than backend
   * representation. It does not decide how a failure is encoded in generated
   * C++, how an outcome is stored at runtime, or how propagation is lowered.
   * Those responsibilities belong to later IR and lowering stages.
   *
   * FailureAnalyzer uses SemanticContext to access source text, diagnostics, and
   * the currently active failure contract.
   *
   * A failure specification establishes that a computation may complete with a
   * recoverable failure in addition to successful completion. The specification
   * itself is represented by OutcomeModel.
   *
   * A `fail` statement is valid only while semantic analysis is inside a
   * failure-aware context. It represents explicit production of the declared
   * recoverable failure outcome.
   *
   * A `try` expression is also valid only inside a failure-aware context. It
   * requests propagation of a recoverable failure produced by another
   * computation. Compatibility between the propagated failure type and the
   * enclosing failure contract requires type information and is therefore kept
   * separate from the structural checks performed by this first analyzer
   * generation.
   *
   * Programmer errors and contract violations are not converted into ordinary
   * Failure outcomes by this analyzer.
   */
  class FailureAnalyzer final
  {
  public:
    /**
     * @brief Creates a failure analyzer using an existing semantic context.
     *
     * The supplied SemanticContext must remain alive for the lifetime of this
     * analyzer.
     *
     * @param context Shared semantic state for the frontend operation.
     */
    explicit FailureAnalyzer(SemanticContext &context) noexcept;

    FailureAnalyzer(const FailureAnalyzer &) = delete;
    FailureAnalyzer &operator=(const FailureAnalyzer &) = delete;

    FailureAnalyzer(FailureAnalyzer &&) = delete;
    FailureAnalyzer &operator=(FailureAnalyzer &&) = delete;

    ~FailureAnalyzer() = default;

    /**
     * @brief Analyzes one failure-related syntax node.
     *
     * The node must represent FailureSpecification, FailStatement, or
     * TryExpression. Passing another syntax kind produces a semantic diagnostic
     * rather than silently accepting an unsupported node.
     *
     * Recoverable semantic errors are reported through the DiagnosticEngine in
     * SemanticContext.
     *
     * @param node Failure syntax node to analyze.
     *
     * @return true when the node satisfies the currently implemented failure
     *         semantics, otherwise false.
     */
    [[nodiscard]]
    bool analyze(const syntax::SyntaxNode &node);

  private:
    /**
     * @brief Validates a failure specification.
     *
     * A valid first-generation failure specification contains exactly one
     * source region describing the declared failure type. The method constructs
     * an OutcomeModel and verifies its structural invariants.
     *
     * This operation validates the specification itself. Activation of that
     * contract for a declaration body is controlled by the semantic traversal
     * that owns the declaration scope.
     *
     * @param node FailureSpecification syntax node.
     *
     * @return true when the specification is structurally valid.
     */
    [[nodiscard]]
    bool analyze_failure_specification(
        const syntax::SyntaxNode &node);

    /**
     * @brief Validates an explicit `fail` statement.
     *
     * The statement must occur in an active failure-aware semantic context and
     * must contain one failure expression.
     *
     * Type compatibility between the expression and the declared failure type
     * is intentionally deferred until semantic type information is available.
     *
     * @param node FailStatement syntax node.
     *
     * @return true when the statement satisfies the currently available
     *         semantic rules.
     */
    [[nodiscard]]
    bool analyze_fail_statement(
        const syntax::SyntaxNode &node);

    /**
     * @brief Validates a failure-propagating `try` expression.
     *
     * The expression must occur inside an active failure-aware context and must
     * contain an operand.
     *
     * The direct-call first slice resolves one collected failure-aware free
     * function and accepts propagation only when caller and callee use the
     * same token-normalized failure-type spelling. Full C++ type identity and
     * conversion remain outside this analyzer.
     *
     * @param node TryExpression syntax node.
     *
     * @return true when the expression satisfies the currently available
     *         semantic rules.
     */
    [[nodiscard]]
    bool analyze_try_expression(
        const syntax::SyntaxNode &node);

    /**
     * @brief Builds an OutcomeModel from a failure specification node.
     *
     * The first child of the specification is expected to identify the source
     * range of the declared failure type.
     *
     * @param node FailureSpecification syntax node.
     *
     * @return Constructed outcome model, or std::nullopt when the syntax node
     *         cannot describe a valid failure contract.
     */
    [[nodiscard]]
    std::optional<OutcomeModel>
    build_outcome_model(
        const syntax::SyntaxNode &node) const;

    /**
     * @brief Reports a semantic failure diagnostic.
     *
     * @param code Stable semantic diagnostic code.
     * @param message Human-readable diagnostic message.
     * @param range Source range associated with the failure.
     * @param hint Optional semantically valid guidance for resolving the
     *        failure.
     *
     * @return false so validation paths can directly return the result.
     */
    bool report_error(
        const char *code,
        const char *message,
        SourceRange range,
        std::string hint = {});

    /// Shared semantic state for the current frontend operation.
    SemanticContext &context_;
  };

} // namespace vixc::semantic::failure

#endif // VIXC_SEMANTIC_FAILURE_FAILURE_ANALYZER_HPP
