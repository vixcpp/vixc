/**
 *
 *  @file Frontend.hpp
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

#if !defined(VIXC_FRONTEND_HPP)
#define VIXC_FRONTEND_HPP

#include <vixc/FrontendOptions.hpp>
#include <vixc/FrontendResult.hpp>

#include <string_view>

namespace vixc
{
  /**
   * @brief Main public entry point to the VixC frontend.
   *
   * Frontend coordinates one complete VixC source-processing operation.
   *
   * A frontend invocation begins with immutable source text and progresses
   * through the internal source, lexical, syntax, semantic, IR, lowering, and
   * backend stages required by the selected FrontendAction.
   *
   * Frontend deliberately exposes the pipeline as one coherent operation rather
   * than requiring embedding applications to construct internal parser,
   * semantic, IR, lowering, or backend objects themselves.
   *
   * The public frontend owns no persistent compilation state between process()
   * calls. Every invocation creates the source and diagnostic state required for
   * that source unit, performs the requested operation, and returns an owning
   * FrontendResult.
   *
   * This makes a Frontend instance reusable and prevents source locations,
   * diagnostics, syntax nodes, semantic state, and intermediate representation
   * from accidentally leaking between independent invocations.
   *
   * Frontend does not read source files from disk. The caller supplies the
   * source identifier and source contents explicitly. This allows the same API
   * to process filesystem files, editor buffers, generated source, tests, remote
   * inputs, and other in-memory source units.
   *
   * The source identifier is used for source tracking and diagnostics. It may be
   * a filesystem path, but VixC does not require it to identify an existing
   * file.
   *
   * For FrontendAction::Analyze, processing stops after syntax and semantic
   * analysis.
   *
   * For FrontendAction::Lower, the frontend additionally constructs and lowers
   * the backend-independent VixC intermediate representation.
   *
   * For FrontendAction::Emit, the complete pipeline runs and the selected
   * backend produces output. The first VixC backend emits ordinary C++.
   *
   * Frontend does not invoke GCC, Clang, MSVC, CMake, Ninja, the Vix.cpp build
   * system, or any other external compilation tool. Generated backend output is
   * returned to the caller through FrontendResult.
   */
  class Frontend final
  {
  public:
    /**
     * @brief Creates a reusable VixC frontend.
     */
    Frontend() = default;

    Frontend(const Frontend &) = delete;
    Frontend &operator=(const Frontend &) = delete;

    Frontend(Frontend &&) noexcept = default;
    Frontend &operator=(Frontend &&) noexcept = default;

    ~Frontend() = default;

    /**
     * @brief Processes one source unit through the VixC frontend.
     *
     * source_name and source are copied into the frontend operation before
     * lexical processing begins. The caller therefore does not need to preserve
     * either string view after this function returns.
     *
     * Diagnostics produced by every executed frontend stage are collected in
     * the returned FrontendResult.
     *
     * Later stages are not executed after an Error or Fatal diagnostic makes
     * the current program invalid.
     *
     * When FrontendAction::Emit is selected and generation succeeds, the
     * returned result owns the generated backend output. Source mappings are
     * also copied into the result when retain_source_map is enabled.
     *
     * @param source_name Path or logical identifier associated with the source.
     * @param source Complete source text.
     * @param options Frontend configuration for this invocation.
     *
     * @return Owning result containing success state, diagnostics, and generated
     *         backend output when requested.
     */
    [[nodiscard]]
    FrontendResult process(
        std::string_view source_name,
        std::string_view source,
        FrontendOptions options = {}) const;
  };

} // namespace vixc

#endif // VIXC_FRONTEND_HPP
