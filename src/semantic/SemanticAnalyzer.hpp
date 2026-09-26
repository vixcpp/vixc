/**
 *
 *  @file SemanticAnalyzer.hpp
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

#if !defined(VIXC_SEMANTIC_SEMANTIC_ANALYZER_HPP)
#define VIXC_SEMANTIC_SEMANTIC_ANALYZER_HPP

#include "SemanticContext.hpp"

namespace vixc::syntax
{

  class SyntaxNode;

} // namespace vixc::syntax

namespace vixc::semantic
{
  /**
   * @brief Coordinates semantic analysis of a VixC syntax tree.
   *
   * SemanticAnalyzer is the main semantic entry point used after parsing. It
   * walks the syntax tree, dispatches VixC-owned constructs to their specialized
   * semantic analyzers, and keeps the shared SemanticContext available
   * throughout the traversal.
   *
   * The analyzer does not reinterpret ordinary C++ represented by CxxRegion
   * nodes. Those regions remain available to later frontend stages and to the
   * C++ backend, while VixC semantic analysis concentrates on constructs whose
   * meaning is owned by the VixC frontend.
   *
   * Specialized semantic domains remain separate from this coordinator. Failure
   * and outcome semantics are handled by the failure semantic layer rather than
   * being implemented directly in SemanticAnalyzer. Future areas such as choice,
   * matching, asynchronous work, ownership, and compile-time semantics can
   * follow the same model.
   *
   * SemanticAnalyzer does not print diagnostics, generate C++, construct the
   * final intermediate representation, or perform lowering. Semantic failures
   * are reported through the DiagnosticEngine stored in SemanticContext.
   */
  class SemanticAnalyzer final
  {
  public:
    /**
     * @brief Creates a semantic analyzer using an existing semantic context.
     *
     * The supplied context must remain alive for the lifetime of this analyzer.
     *
     * @param context Shared semantic state for the frontend operation.
     */
    explicit SemanticAnalyzer(SemanticContext &context) noexcept;

    SemanticAnalyzer(const SemanticAnalyzer &) = delete;
    SemanticAnalyzer &operator=(const SemanticAnalyzer &) = delete;

    SemanticAnalyzer(SemanticAnalyzer &&) = delete;
    SemanticAnalyzer &operator=(SemanticAnalyzer &&) = delete;

    ~SemanticAnalyzer() = default;

    /**
     * @brief Performs semantic analysis of a syntax tree.
     *
     * The root is normally a SyntaxKind::TranslationUnit produced by Parser,
     * but any syntax node can be analyzed when tests or another frontend stage
     * need to validate an isolated construct.
     *
     * Analysis continues across recoverable semantic errors when possible so a
     * single frontend invocation can report more than one useful diagnostic.
     * Fatal diagnostics stop further semantic traversal.
     *
     * The return value reflects the diagnostic state after analysis. Existing
     * errors already present in the shared DiagnosticEngine also cause this
     * function to return false.
     *
     * @param root Root syntax node to analyze.
     *
     * @return true when semantic analysis completes without Error or Fatal
     *         diagnostics, otherwise false.
     */
    [[nodiscard]]
    bool analyze(const syntax::SyntaxNode &root);

  private:
    /**
     * @brief Analyzes one syntax node.
     *
     * This function dispatches VixC-owned constructs to the semantic subsystem
     * responsible for them and recursively traverses structural nodes.
     *
     * Ordinary CxxRegion nodes require no VixC semantic validation and are
     * accepted without interpretation.
     *
     * @param node Syntax node to analyze.
     *
     * @return true when traversal may continue, otherwise false.
     */
    bool analyze_node(const syntax::SyntaxNode &node);

    /**
     * @brief Collects supported top-level failure-aware function definitions.
     *
     * This phase intentionally records only declarations represented with a
     * structural FunctionName and body, excluding unsupported C++ declarator
     * forms from direct propagation lookup.
     *
     * @param root Translation-unit syntax node.
     * @return true when collection completed without a fatal diagnostic.
     */
    bool collect_failure_declarations(const syntax::SyntaxNode &root);

    /**
     * @brief Collects one supported failure-aware function definition.
     *
     * @param node FunctionDeclaration syntax node.
     * @return true when collection may continue.
     */
    bool collect_failure_declaration(const syntax::SyntaxNode &node);

    /**
     * @brief Analyzes one declaration-scoped failure-aware function body.
     *
     * @param node FunctionDeclaration syntax node.
     * @return true when the declaration and its body are semantically valid.
     */
    bool analyze_function_declaration(const syntax::SyntaxNode &node);

    /**
     * @brief Analyzes all direct children of a syntax node.
     *
     * Child nodes are visited in parser-defined source order. Recoverable errors
     * do not stop traversal, while a fatal diagnostic terminates the walk.
     *
     * @param node Parent syntax node.
     *
     * @return true when traversal completes without encountering a condition
     *         that requires semantic analysis to stop.
     */
    bool analyze_children(const syntax::SyntaxNode &node);

    /// Shared semantic state for the current frontend operation.
    SemanticContext &context_;
  };

} // namespace vixc::semantic

#endif // VIXC_SEMANTIC_SEMANTIC_ANALYZER_HPP
