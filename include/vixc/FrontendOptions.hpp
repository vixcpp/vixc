/**
 *
 *  @file FrontendOptions.hpp
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

#if !defined(VIXC_FRONTEND_OPTIONS_HPP)
#define VIXC_FRONTEND_OPTIONS_HPP

#include <string_view>

namespace vixc
{
  /**
   * @brief Selects how far the VixC frontend processes a source unit.
   *
   * FrontendAction allows embedders to use the same frontend for analysis,
   * semantic tooling, compilation preparation, and generated C++ output without
   * exposing individual internal pipeline stages as separate public APIs.
   *
   * Analyze runs source processing, syntax analysis, and semantic analysis. It
   * is suitable for editors, diagnostics, language tooling, and callers that do
   * not require generated code.
   *
   * Lower additionally constructs and lowers the backend-independent VixC
   * intermediate representation. It is useful for frontend validation and
   * tooling that needs semantic IR without requesting target emission.
   *
   * Emit performs the complete frontend pipeline and invokes the selected
   * backend after successful lowering.
   */
  enum class FrontendAction
  {
    /// Stop after semantic analysis.
    Analyze,

    /// Build and lower the backend-independent IR.
    Lower,

    /// Run the complete frontend and generate backend output.
    Emit
  };

  /**
   * @brief Returns the stable textual name of a frontend action.
   *
   * @param action Frontend action to describe.
   *
   * @return Stable lowercase name of the action.
   */
  [[nodiscard]]
  constexpr std::string_view
  frontend_action_name(
      FrontendAction action) noexcept
  {
    switch (action)
    {
    case FrontendAction::Analyze:
      return "analyze";

    case FrontendAction::Lower:
      return "lower";

    case FrontendAction::Emit:
      return "emit";
    }

    return "unknown";
  }

  /**
   * @brief Selects the target backend used by frontend emission.
   *
   * BackendTarget belongs to the public frontend configuration rather than to a
   * concrete backend implementation. This allows callers to select a target
   * without depending on private backend headers.
   *
   * The first VixC generation provides the Cxx backend, which emits ordinary
   * C++ for compilation by existing native C++ toolchains.
   *
   * Additional backend targets can be introduced without changing the meaning
   * of the VixC semantic model or intermediate representation.
   */
  enum class BackendTarget
  {
    /// Emit ordinary C++ source.
    Cxx
  };

  /**
   * @brief Returns the stable textual name of a backend target.
   *
   * @param target Backend target to describe.
   *
   * @return Stable lowercase name of the backend target.
   */
  [[nodiscard]]
  constexpr std::string_view
  backend_target_name(
      BackendTarget target) noexcept
  {
    switch (target)
    {
    case BackendTarget::Cxx:
      return "cxx";
    }

    return "unknown";
  }

  /**
   * @brief Configures one VixC frontend invocation.
   *
   * FrontendOptions contains public choices that change how far the frontend
   * runs or which output representation it produces.
   *
   * The options intentionally avoid exposing parser, semantic analyzer,
   * lowering pass, IR implementation, or backend-internal switches. Those are
   * implementation details of the frontend rather than configuration that every
   * embedding application should need to understand.
   *
   * The default configuration performs a complete frontend operation and emits
   * ordinary C++ using the first VixC backend.
   */
  struct FrontendOptions final
  {
    /**
     * @brief Controls the final stage reached by the frontend.
     *
     * The default is FrontendAction::Emit.
     */
    FrontendAction action{FrontendAction::Emit};

    /**
     * @brief Selects the backend used when action is FrontendAction::Emit.
     *
     * The option is ignored for Analyze and Lower operations.
     *
     * The first frontend generation supports BackendTarget::Cxx.
     */
    BackendTarget backend{BackendTarget::Cxx};

    /**
     * @brief Controls whether generated-source provenance is retained.
     *
     * When enabled, backend output may include a source map connecting generated
     * regions to their original VixC SourceRange values.
     *
     * Disabling source-map retention does not change generated program
     * semantics.
     *
     * The default is true because source provenance is important for diagnostics
     * produced by later native compilation stages.
     */
    bool retain_source_map{true};
  };

} // namespace vixc

#endif // VIXC_FRONTEND_OPTIONS_HPP
