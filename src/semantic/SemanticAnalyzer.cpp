/**
 *
 *  @file SemanticAnalyzer.cpp
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

#include "SemanticAnalyzer.hpp"

#include "failure/FailureAnalyzer.hpp"

#include "../diagnostics/DiagnosticEngine.hpp"
#include "../syntax/SyntaxKind.hpp"
#include "../syntax/SyntaxNode.hpp"

namespace vixc::semantic
{
  SemanticAnalyzer::SemanticAnalyzer(
      SemanticContext &context) noexcept
      : context_(context)
  {
  }

  bool SemanticAnalyzer::analyze(
      const syntax::SyntaxNode &root)
  {
    if (context_.diagnostics().has_fatal())
      return false;

    analyze_node(root);

    return !context_.diagnostics().has_errors();
  }

  bool SemanticAnalyzer::analyze_node(
      const syntax::SyntaxNode &node)
  {
    if (context_.diagnostics().has_fatal())
      return false;

    switch (node.kind())
    {
    case syntax::SyntaxKind::Invalid:
      return false;

    case syntax::SyntaxKind::TranslationUnit:
      return analyze_children(node);

    case syntax::SyntaxKind::CxxRegion:
      return true;

    case syntax::SyntaxKind::Identifier:
    case syntax::SyntaxKind::IntegerLiteral:
    case syntax::SyntaxKind::FloatingLiteral:
    case syntax::SyntaxKind::StringLiteral:
    case syntax::SyntaxKind::CharacterLiteral:
    case syntax::SyntaxKind::ParenthesizedExpression:
      return analyze_children(node);

    case syntax::SyntaxKind::FailureSpecification:
    case syntax::SyntaxKind::FailStatement:
    case syntax::SyntaxKind::TryExpression:
    {
      failure::FailureAnalyzer analyzer{context_};
      return analyzer.analyze(node);
    }
    }

    return false;
  }

  bool SemanticAnalyzer::analyze_children(
      const syntax::SyntaxNode &node)
  {
    bool successful = true;

    for (const syntax::SyntaxNode &child : node.children())
    {
      if (context_.diagnostics().has_fatal())
        return false;

      if (!analyze_node(child))
        successful = false;
    }

    return successful;
  }

} // namespace vixc::semantic
