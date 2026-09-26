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

    case syntax::SyntaxKind::FunctionDeclaration:
      return analyze_function_declaration(node);

    case syntax::SyntaxKind::FunctionReturnType:
    case syntax::SyntaxKind::FunctionDeclarator:

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

    case syntax::SyntaxKind::ReturnStatement:
    case syntax::SyntaxKind::TryInitialization:
      return analyze_children(node);
    }

    return false;
  }

  bool SemanticAnalyzer::analyze_function_declaration(
      const syntax::SyntaxNode &node)
  {
    const syntax::SyntaxNode *specification = nullptr;
    std::size_t specification_index = 0;

    for (std::size_t index = 0;
         index < node.child_count();
         ++index)
    {
      const syntax::SyntaxNode *child = node.child(index);
      if (child != nullptr &&
          child->kind() == syntax::SyntaxKind::FailureSpecification)
      {
        specification = child;
        specification_index = index;
        break;
      }
    }

    if (specification == nullptr || specification->child_count() != 1)
      return false;

    failure::FailureAnalyzer analyzer{context_};
    if (!analyzer.analyze(*specification))
      return false;

    const syntax::SyntaxNode *failure_type = specification->child(0);
    if (failure_type == nullptr)
      return false;

    context_.push_failure_context(
        FailureContext{
            specification->range(),
            failure_type->range()});

    bool successful = true;
    for (std::size_t index = specification_index + 1;
         index < node.child_count();
         ++index)
    {
      const syntax::SyntaxNode *child = node.child(index);
      if (child != nullptr && !analyze_node(*child))
        successful = false;
    }

    context_.pop_failure_context();
    return successful;
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
