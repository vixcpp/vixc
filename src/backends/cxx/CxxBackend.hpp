/**
 *
 *  @file CxxBackend.hpp
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

#if !defined(VIXC_BACKENDS_CXX_CXX_BACKEND_HPP)
#define VIXC_BACKENDS_CXX_CXX_BACKEND_HPP

#include "CxxEmitter.hpp"

#include "../Backend.hpp"

#include <vixc/SourceRange.hpp>

#include <string>
#include <string_view>

namespace vixc::diagnostics
{

  class DiagnosticEngine;

} // namespace vixc::diagnostics

namespace vixc::source
{

  class SourceManager;

} // namespace vixc::source

namespace vixc::ir
{

  class IrNode;
  class Program;

} // namespace vixc::ir

namespace vixc::ir::failure
{

  class Outcome;
  class Failure;
  class FailurePropagation;

} // namespace vixc::ir::failure

namespace vixc::backends::cxx
{

  /**
   * @brief Emits ordinary C++ from lowered VixC intermediate representation.
   *
   * CxxBackend is the first concrete backend of the VixC frontend. It translates
   * backend-independent IR into C++ source that can be compiled by existing
   * native C++ toolchains.
   *
   * The backend receives a Program only after syntax analysis, semantic
   * analysis, IR construction, and common lowering have completed
   * successfully. It does not decide whether VixC source is semantically valid.
   * Its responsibility is to preserve the established semantics in generated
   * C++.
   *
   * Ordinary C++ represented by CxxRegion nodes is recovered from the original
   * SourceManager and emitted without requiring VixC to reconstruct the complete
   * C++ grammar.
   *
   * VixC-owned IR nodes are emitted according to the C++ representation selected
   * by this backend. The representation may use generated helper types,
   * temporaries, control flow, functions, or other C++ mechanisms, but those
   * implementation choices must preserve the meaning established by the common
   * frontend.
   *
   * CxxBackend does not invoke GCC, Clang, MSVC, a linker, CMake, Ninja, or any
   * other external build tool. The result of generation is an in-memory C++
   * translation unit that the caller can pass to an existing compilation
   * pipeline.
   *
   * Generated source provenance is recorded by CxxEmitter through
   * CxxSourceMap. Source copied directly from the input as well as generated
   * representations of VixC constructs can therefore remain connected to their
   * original SourceRange.
   *
   * The backend is reusable. Calling generate() starts a new generation and
   * clears output produced by the previous operation.
   *
   * CxxBackend does not own SourceManager or DiagnosticEngine. Both objects must
   * outlive the backend.
   */
  class CxxBackend final : public Backend
  {
  public:
    /**
     * @brief Creates a C++ backend for one frontend environment.
     *
     * The source manager is used to recover original C++ regions and source
     * spellings retained by the IR.
     *
     * Backend diagnostics are written to the supplied DiagnosticEngine as
     * structured values.
     *
     * @param sources Source manager containing the original source units.
     * @param diagnostics Diagnostic engine receiving backend diagnostics.
     */
    CxxBackend(
        source::SourceManager &sources,
        diagnostics::DiagnosticEngine &diagnostics) noexcept;

    CxxBackend(const CxxBackend &) = delete;
    CxxBackend &operator=(const CxxBackend &) = delete;

    CxxBackend(CxxBackend &&) = delete;
    CxxBackend &operator=(CxxBackend &&) = delete;

    ~CxxBackend() override = default;

    /**
     * @brief Returns the stable backend name.
     *
     * @return `"cxx"`.
     */
    [[nodiscard]]
    std::string_view name() const noexcept override;

    /**
     * @brief Generates C++ for a lowered VixC program.
     *
     * Previous generated output is cleared before generation begins.
     *
     * Generation is not attempted when the shared DiagnosticEngine already
     * contains an Error or Fatal diagnostic. Invalid frontend state must not be
     * translated as though it were a valid program.
     *
     * Top-level IR nodes are emitted in program order. Ordinary C++ regions are
     * copied from their original source. VixC-owned semantic nodes are
     * dispatched to their corresponding C++ emission routines.
     *
     * @param program Lowered backend-independent VixC program.
     *
     * @return true when the complete C++ translation unit is generated without
     *         backend errors, otherwise false.
     */
    [[nodiscard]]
    bool generate(
        const ir::Program &program) override;

    /**
     * @brief Clears generated C++ and source-map state.
     *
     * SourceManager and DiagnosticEngine are not modified.
     */
    void reset() override;

    /**
     * @brief Returns the generated C++ translation unit.
     *
     * The returned reference remains valid until the backend is modified,
     * reset, moved from, or destroyed.
     *
     * @return Complete generated C++ source.
     */
    [[nodiscard]]
    const std::string &output() const noexcept;

    /**
     * @brief Returns the source map associated with generated C++.
     *
     * @return Read-only mapping between generated C++ and original VixC source.
     */
    [[nodiscard]]
    const CxxSourceMap &source_map() const noexcept;

    /**
     * @brief Reports whether no C++ source has been generated.
     *
     * @return true when output() is empty.
     */
    [[nodiscard]]
    bool empty() const noexcept;

  private:
    /**
     * @brief Emits one IR node.
     *
     * The node is dispatched according to its IrKind. Program nodes recurse into
     * their children, CxxRegion nodes copy original source, and VixC semantic
     * nodes are handled by specialized backend routines.
     *
     * @param node IR node to emit.
     *
     * @return true when emission succeeds.
     */
    bool emit_node(const ir::IrNode &node);

    /**
     * @brief Emits all direct children of an IR node.
     *
     * Children are emitted in stored order.
     *
     * @param node Parent node whose children will be emitted.
     *
     * @return true when every child is emitted successfully.
     */
    bool emit_children(const ir::IrNode &node);

    /**
     * @brief Emits an ordinary C++ source region.
     *
     * The source bytes are recovered from SourceManager using the node's
     * SourceRange and copied directly into the generated output.
     *
     * The copied bytes are associated with their original range in the C++
     * source map.
     *
     * @param node CxxRegion IR node.
     *
     * @return true when the original source can be recovered and emitted.
     */
    bool emit_cxx_region(
        const ir::IrNode &node);

    /**
     * @brief Emits the C++ representation required for an Outcome contract.
     *
     * Outcome remains a semantic IR concept until this backend boundary.
     * This method is responsible for translating that contract into the concrete
     * C++ machinery selected by the first VixC backend.
     *
     * @param outcome Outcome IR node.
     *
     * @return true when the outcome contract is emitted successfully.
     */
    bool emit_outcome(
        const ir::failure::Outcome &outcome);

    /**
     * @brief Emits an explicit recoverable Failure operation.
     *
     * The generated C++ must complete the current failure-aware computation
     * with the same recoverable failure semantics represented by the IR.
     *
     * @param failure Failure IR node.
     *
     * @return true when the operation is emitted successfully.
     */
    bool emit_failure(
        const ir::failure::Failure &failure);

    /**
     * @brief Emits recoverable failure propagation.
     *
     * The generated C++ evaluates the propagation operand and preserves the
     * distinction between successful completion and recoverable failure
     * established by the VixC semantic model.
     *
     * @param propagation FailurePropagation IR node.
     *
     * @return true when propagation is emitted successfully.
     */
    bool emit_failure_propagation(
        const ir::failure::FailurePropagation &propagation);

    /**
     * @brief Returns the original source bytes covered by a SourceRange.
     *
     * The range must belong to a source registered with SourceManager and remain
     * within that source's byte bounds.
     *
     * The returned view is non-owning.
     *
     * @param range Original source range to recover.
     *
     * @return Source bytes, or an empty view when the range cannot be resolved.
     */
    [[nodiscard]]
    std::string_view source_text(
        SourceRange range) const noexcept;

    /**
     * @brief Reports a C++ backend diagnostic.
     *
     * @param code Stable backend diagnostic code.
     * @param message Human-readable diagnostic message.
     * @param range Original source range associated with the failure.
     *
     * @return false so generation paths can directly return the result.
     */
    bool report_error(
        const char *code,
        const char *message,
        SourceRange range);

    /// Original source storage used to recover preserved C++ regions.
    source::SourceManager &sources_;

    /// Diagnostic sink shared with the frontend operation.
    diagnostics::DiagnosticEngine &diagnostics_;

    /// C++ text and source-map builder for the current generation.
    CxxEmitter emitter_;
  };

} // namespace vixc::backends::cxx

#endif // VIXC_BACKENDS_CXX_CXX_BACKEND_HPP
