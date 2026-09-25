/**
 *
 *  @file LoweringPipeline.hpp
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

#if !defined(VIXC_LOWERING_LOWERING_PIPELINE_HPP)
#define VIXC_LOWERING_LOWERING_PIPELINE_HPP

#include "LoweringContext.hpp"

#include <vixc/SourceRange.hpp>

namespace vixc::ir
{

  class IrNode;
  class Program;

} // namespace vixc::ir

namespace vixc::lowering
{

  /**
   * @brief Coordinates backend-independent lowering of a VixC program.
   *
   * LoweringPipeline is the main entry point between the semantic IR and the
   * backend layer. It walks a semantically valid Program and dispatches IR nodes
   * to the lowering implementation responsible for their semantic domain.
   *
   * Lowering is performed after semantic analysis. The pipeline therefore
   * assumes that language rules such as failure-context validity and type
   * compatibility have already been established before an IR node reaches this
   * stage.
   *
   * The pipeline does not emit C++, invoke a native compiler, format
   * diagnostics, or make backend-specific representation decisions. Its role is
   * to prepare VixC IR for backend consumption while preserving the semantics
   * established by earlier frontend stages.
   *
   * Ordinary C++ represented by IrKind::CxxRegion passes through the common
   * lowering pipeline unchanged. VixC-owned semantic nodes are delegated to
   * specialized lowering implementations.
   *
   * Failure and Outcome are the first semantic domain handled by this model.
   * Their lowering is implemented by FailureLowering rather than directly
   * inside LoweringPipeline.
   *
   * Future semantic domains can be added to the pipeline without moving their
   * lowering rules into the coordinator itself.
   */
  class LoweringPipeline final
  {
  public:
    /**
     * @brief Creates a lowering pipeline using an existing lowering context.
     *
     * The supplied context must remain alive for the lifetime of this pipeline.
     *
     * @param context Shared state for the current lowering operation.
     */
    explicit LoweringPipeline(LoweringContext &context) noexcept;

    LoweringPipeline(const LoweringPipeline &) = delete;
    LoweringPipeline &operator=(const LoweringPipeline &) = delete;

    LoweringPipeline(LoweringPipeline &&) = delete;
    LoweringPipeline &operator=(LoweringPipeline &&) = delete;

    ~LoweringPipeline() = default;

    /**
     * @brief Lowers a complete VixC program.
     *
     * The program is traversed in deterministic top-level order. Ordinary C++
     * regions are preserved while VixC semantic constructs are dispatched to
     * their specialized lowering passes.
     *
     * Lowering does not begin when the shared DiagnosticEngine already contains
     * an Error or Fatal diagnostic. A program containing known semantic errors
     * must not be transformed as though it were valid.
     *
     * Recoverable lowering errors are reported through LoweringContext. A fatal
     * diagnostic stops traversal immediately.
     *
     * The Program remains owned by the caller.
     *
     * @param program Semantically valid IR program to lower.
     *
     * @return true when the complete program is successfully lowered and no
     *         Error or Fatal diagnostics exist, otherwise false.
     */
    [[nodiscard]]
    bool lower(ir::Program &program);

  private:
    /**
     * @brief Lowers one IR node.
     *
     * Program nodes are traversed recursively. CxxRegion nodes require no common
     * transformation. Failure-related IR kinds are delegated to
     * FailureLowering.
     *
     * @param node IR node to lower.
     *
     * @return true when lowering of the node succeeds.
     */
    bool lower_node(ir::IrNode &node);

    /**
     * @brief Lowers all direct children of an IR node.
     *
     * Children are processed in their stored order. Recoverable failures are
     * accumulated so additional independent nodes may still be checked, while a
     * Fatal diagnostic stops traversal immediately.
     *
     * @param node Parent IR node whose children will be lowered.
     *
     * @return true when every child is successfully lowered.
     */
    bool lower_children(ir::IrNode &node);

    /**
     * @brief Reports an error produced by the common lowering pipeline.
     *
     * Domain-specific lowering implementations should report their own
     * diagnostics instead of routing semantic-specific errors through this
     * function.
     *
     * @param code Stable lowering diagnostic code.
     * @param message Human-readable diagnostic message.
     * @param range Original source range associated with the failure.
     *
     * @return false so callers can directly return the result.
     */
    bool report_error(
        const char *code,
        const char *message,
        SourceRange range);

    /// Shared state for the current lowering operation.
    LoweringContext &context_;
  };

} // namespace vixc::lowering

#endif // VIXC_LOWERING_LOWERING_PIPELINE_HPP
