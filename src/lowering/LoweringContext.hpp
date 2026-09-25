/**
 *
 *  @file LoweringContext.hpp
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

#if !defined(VIXC_LOWERING_LOWERING_CONTEXT_HPP)
#define VIXC_LOWERING_LOWERING_CONTEXT_HPP

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace vixc::diagnostics
{

  class DiagnosticEngine;

} // namespace vixc::diagnostics

namespace vixc::source
{

  class SourceManager;

} // namespace vixc::source

namespace vixc::lowering
{
  /**
   * @brief Shared state used while lowering VixC intermediate representation.
   *
   * LoweringContext connects lowering passes to the source and diagnostic
   * infrastructure of the frontend.
   *
   * Lowering operates after syntax and semantic analysis have established the
   * meaning of VixC constructs. Its responsibility is to transform those
   * semantic constructs into progressively simpler forms that later backend
   * stages can implement.
   *
   * LoweringContext deliberately contains no C++ emission state. Generated
   * identifiers, formatting rules, C++ source buffers, include management, and
   * other backend-specific concerns belong to the selected backend rather than
   * to the common lowering layer.
   *
   * The context does not own SourceManager or DiagnosticEngine. Both objects are
   * owned by the surrounding frontend operation and must outlive this context.
   *
   * Source access is provided so lowering passes can recover exact source
   * spelling when an IR node intentionally preserves ordinary C++ through a
   * SourceRange. The returned text always refers to the original immutable
   * source buffer.
   *
   * The context also provides deterministic identifiers for synthesized
   * lowering operations. These identifiers are local to one lowering operation
   * and have no language-level meaning. They may be used to distinguish
   * synthesized IR objects when a transformation requires stable internal
   * identity.
   */
  class LoweringContext final
  {
  public:
    /**
     * @brief Creates a lowering context for one frontend operation.
     *
     * The referenced source manager and diagnostic engine must remain alive for
     * the complete lifetime of this context.
     *
     * @param sources Source manager containing the original source units.
     * @param diagnostics Diagnostic engine receiving lowering diagnostics.
     */
    LoweringContext(
        source::SourceManager &sources,
        diagnostics::DiagnosticEngine &diagnostics) noexcept;

    LoweringContext(const LoweringContext &) = delete;
    LoweringContext &operator=(const LoweringContext &) = delete;

    LoweringContext(LoweringContext &&) = delete;
    LoweringContext &operator=(LoweringContext &&) = delete;

    ~LoweringContext() = default;

    /**
     * @brief Returns the source manager associated with this lowering operation.
     *
     * @return Mutable reference to the source manager.
     */
    [[nodiscard]]
    source::SourceManager &sources() noexcept;

    /**
     * @brief Returns the source manager associated with this lowering operation.
     *
     * @return Read-only reference to the source manager.
     */
    [[nodiscard]]
    const source::SourceManager &sources() const noexcept;

    /**
     * @brief Returns the diagnostic engine used during lowering.
     *
     * Lowering passes report structured diagnostics through this object rather
     * than printing or formatting errors directly.
     *
     * @return Mutable reference to the diagnostic engine.
     */
    [[nodiscard]]
    diagnostics::DiagnosticEngine &diagnostics() noexcept;

    /**
     * @brief Returns the diagnostic engine used during lowering.
     *
     * @return Read-only reference to the diagnostic engine.
     */
    [[nodiscard]]
    const diagnostics::DiagnosticEngine &diagnostics() const noexcept;

    /**
     * @brief Returns source text covered by a source range.
     *
     * The supplied range must be structurally valid, refer to a source owned by
     * the associated SourceManager, and remain inside that SourceFile's byte
     * bounds.
     *
     * The returned view does not own the source text and remains valid while the
     * corresponding SourceFile remains owned by the SourceManager.
     *
     * @param range Original source range to read.
     *
     * @return View of the requested source bytes, or std::nullopt when the range
     *         or source is invalid.
     */
    [[nodiscard]]
    std::optional<std::string_view>
    source_text(SourceRange range) const noexcept;

    /**
     * @brief Allocates a deterministic identifier for synthesized lowering data.
     *
     * Identifiers are monotonically increasing and local to this
     * LoweringContext. The first returned identifier is zero.
     *
     * A synthetic identifier is an implementation detail of lowering. It is not
     * a source identifier, IR kind, symbol identity, or backend-generated name.
     *
     * @return Next synthetic identifier.
     */
    [[nodiscard]]
    std::size_t next_synthetic_id() noexcept;

    /**
     * @brief Returns the number of synthetic identifiers allocated so far.
     *
     * @return Number of calls to next_synthetic_id().
     */
    [[nodiscard]]
    std::size_t synthetic_count() const noexcept;

    /**
     * @brief Reports whether lowering currently has error diagnostics.
     *
     * This reflects the shared DiagnosticEngine and therefore includes errors
     * produced by earlier frontend stages when the same engine is reused.
     *
     * @return true when at least one Error or Fatal diagnostic exists.
     */
    [[nodiscard]]
    bool has_errors() const noexcept;

    /**
     * @brief Reports whether lowering encountered a fatal frontend condition.
     *
     * @return true when the diagnostic engine contains a Fatal diagnostic.
     */
    [[nodiscard]]
    bool has_fatal() const noexcept;

  private:
    /// Original source storage used by lowering passes.
    source::SourceManager &sources_;

    /// Diagnostic sink shared with the surrounding frontend operation.
    diagnostics::DiagnosticEngine &diagnostics_;

    /// Next deterministic identifier available to synthesized lowering data.
    std::size_t next_synthetic_id_{0};
  };

} // namespace vixc::lowering

#endif // VIXC_LOWERING_LOWERING_CONTEXT_HPP
