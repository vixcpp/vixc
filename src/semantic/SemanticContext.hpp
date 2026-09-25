/**
 *
 *  @file SemanticContext.hpp
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

#if !defined(VIXC_SEMANTIC_SEMANTIC_CONTEXT_HPP)
#define VIXC_SEMANTIC_SEMANTIC_CONTEXT_HPP

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace vixc::diagnostics
{

  class DiagnosticEngine;

} // namespace vixc::diagnostics

namespace vixc::source
{

  class SourceManager;

} // namespace vixc::source

namespace vixc::semantic
{

  /**
   * @brief Describes the active failure contract during semantic analysis.
   *
   * FailureContext records the source regions associated with a failure-aware
   * declaration while its body is being analyzed.
   *
   * The specification range covers the complete `fails` declaration fragment.
   * The type range identifies the source spelling that describes the failure
   * type itself.
   *
   * This structure deliberately stores source information rather than a final
   * semantic type representation. Failure type interpretation belongs to the
   * failure semantic model and may evolve independently from the shared
   * SemanticContext.
   */
  struct FailureContext final
  {
    /**
     * @brief Source range of the complete failure specification.
     */
    SourceRange specification_range{};

    /**
     * @brief Source range containing the declared failure type.
     */
    SourceRange type_range{};
  };

  /**
   * @brief Shared state used while performing semantic analysis.
   *
   * SemanticContext connects semantic analyzers to the source and diagnostic
   * infrastructure of the frontend. It also tracks contextual state that must
   * remain visible while nested syntax is analyzed.
   *
   * The context does not own source files or diagnostics. SourceManager and
   * DiagnosticEngine are owned by the surrounding frontend operation and must
   * outlive the SemanticContext.
   *
   * Source access is provided through source_text(). Semantic analyzers should
   * use source ranges as their canonical reference to source code rather than
   * storing additional copies of source text.
   *
   * The first contextual semantic state maintained here is the active failure
   * contract. A stack is used because failure-aware declarations may eventually
   * appear inside nested functions, lambdas, generated contexts, or other
   * constructs that temporarily introduce their own failure semantics.
   *
   * SemanticContext does not determine whether a failure specification is
   * valid. It only records the currently active semantic environment. Validation
   * remains the responsibility of the corresponding semantic analyzer.
   */
  class SemanticContext final
  {
  public:
    /**
     * @brief Creates a semantic context for one frontend operation.
     *
     * Both referenced objects must remain alive for the complete lifetime of
     * this SemanticContext.
     *
     * @param sources Source manager containing the source units being analyzed.
     * @param diagnostics Diagnostic engine receiving semantic diagnostics.
     */
    SemanticContext(
        source::SourceManager &sources,
        diagnostics::DiagnosticEngine &diagnostics) noexcept;

    SemanticContext(const SemanticContext &) = delete;
    SemanticContext &operator=(const SemanticContext &) = delete;

    SemanticContext(SemanticContext &&) = delete;
    SemanticContext &operator=(SemanticContext &&) = delete;

    ~SemanticContext() = default;

    /**
     * @brief Returns the source manager associated with this context.
     *
     * @return Mutable reference to the frontend source manager.
     */
    [[nodiscard]]
    source::SourceManager &sources() noexcept;

    /**
     * @brief Returns the source manager associated with this context.
     *
     * @return Read-only reference to the frontend source manager.
     */
    [[nodiscard]]
    const source::SourceManager &sources() const noexcept;

    /**
     * @brief Returns the diagnostic engine associated with this context.
     *
     * Semantic analyzers use this engine to report structured diagnostics
     * without depending on a presentation layer.
     *
     * @return Mutable reference to the frontend diagnostic engine.
     */
    [[nodiscard]]
    diagnostics::DiagnosticEngine &diagnostics() noexcept;

    /**
     * @brief Returns the diagnostic engine associated with this context.
     *
     * @return Read-only reference to the frontend diagnostic engine.
     */
    [[nodiscard]]
    const diagnostics::DiagnosticEngine &diagnostics() const noexcept;

    /**
     * @brief Returns the original source text covered by a source range.
     *
     * The range must be structurally valid, reference a source owned by the
     * associated SourceManager, and remain within that SourceFile's byte bounds.
     *
     * The returned view refers directly to the immutable source buffer and does
     * not own its character storage.
     *
     * @param range Source range to read.
     *
     * @return View of the requested source bytes, or std::nullopt when the
     *         source or range is invalid.
     */
    [[nodiscard]]
    std::optional<std::string_view>
    source_text(SourceRange range) const noexcept;

    /**
     * @brief Enters a failure-aware semantic context.
     *
     * The supplied context becomes the active failure contract until a matching
     * call to pop_failure_context().
     *
     * Nested failure contexts are supported. The most recently pushed context
     * is always considered active.
     *
     * @param context Failure contract to activate.
     */
    void push_failure_context(FailureContext context);

    /**
     * @brief Leaves the currently active failure context.
     *
     * Calling this function when no failure context is active has no effect.
     */
    void pop_failure_context() noexcept;

    /**
     * @brief Reports whether semantic analysis is currently inside a
     * failure-aware context.
     *
     * @return true when at least one failure context is active.
     */
    [[nodiscard]]
    bool has_failure_context() const noexcept;

    /**
     * @brief Returns the currently active failure contract.
     *
     * When failure contexts are nested, this returns the most recently pushed
     * context.
     *
     * The returned pointer refers to storage owned by SemanticContext and
     * remains valid until the failure context stack is modified.
     *
     * @return Pointer to the active failure context, or nullptr when none is
     *         active.
     */
    [[nodiscard]]
    const FailureContext *current_failure_context() const noexcept;

    /**
     * @brief Returns the number of active failure contexts.
     *
     * This is primarily useful for semantic invariants, tests, and controlled
     * unwinding of nested analysis.
     *
     * @return Current failure-context stack depth.
     */
    [[nodiscard]]
    std::size_t failure_context_depth() const noexcept;

    /**
     * @brief Removes all active failure contexts.
     *
     * This restores the context to a state where no failure contract is active.
     * Source and diagnostic state are unaffected.
     */
    void clear_failure_contexts() noexcept;

  private:
    /// Source storage used by semantic analysis.
    source::SourceManager &sources_;

    /// Diagnostic sink shared by semantic analyzers.
    diagnostics::DiagnosticEngine &diagnostics_;

    /// Nested failure contracts in semantic traversal order.
    std::vector<FailureContext> failure_contexts_;
  };

} // namespace vixc::semantic

#endif // VIXC_SEMANTIC_SEMANTIC_CONTEXT_HPP
