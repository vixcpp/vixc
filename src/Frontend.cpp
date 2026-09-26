/**
 *
 *  @file Frontend.cpp
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

#include <vixc/Frontend.hpp>

#include "backends/cxx/CxxBackend.hpp"
#include "diagnostics/DiagnosticEngine.hpp"
#include "ir/IrKind.hpp"
#include "ir/IrNode.hpp"
#include "ir/Program.hpp"
#include "ir/failure/Failure.hpp"
#include "ir/failure/FailureAwareFunction.hpp"
#include "ir/failure/Outcome.hpp"
#include "lowering/LoweringContext.hpp"
#include "lowering/LoweringPipeline.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "semantic/SemanticContext.hpp"
#include "source/SourceFile.hpp"
#include "source/SourceManager.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Parser.hpp"
#include "syntax/SyntaxKind.hpp"
#include "syntax/SyntaxNode.hpp"
#include <vixc/DiagnosticSeverity.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vixc
{
  namespace
  {

    struct IrBuildContext final
    {
      std::optional<SourceRange> failure_type_range;
    };

    std::unique_ptr<ir::IrNode>
    build_ir_node(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics);

    std::unique_ptr<ir::IrNode>
    build_failure_specification(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics);

    std::unique_ptr<ir::IrNode>
    build_function_declaration(
        const syntax::SyntaxNode &node,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      const syntax::SyntaxNode *success_type = nullptr;
      const syntax::SyntaxNode *declarator = nullptr;
      const syntax::SyntaxNode *specification = nullptr;
      std::size_t specification_index = 0;

      for (std::size_t index = 0;
           index < node.child_count();
           ++index)
      {
        const syntax::SyntaxNode *child = node.child(index);
        if (child == nullptr)
          continue;

        if (child->kind() == syntax::SyntaxKind::FunctionReturnType)
          success_type = child;
        else if (child->kind() == syntax::SyntaxKind::FunctionDeclarator)
          declarator = child;
        else if (child->kind() == syntax::SyntaxKind::FailureSpecification)
        {
          specification = child;
          specification_index = index;
          break;
        }
      }

      if (success_type == nullptr || declarator == nullptr ||
          specification == nullptr)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5015",
            "failure-aware function has no structured declaration signature",
            node.range());

        return nullptr;
      }

      IrBuildContext function_context;

      std::unique_ptr<ir::IrNode> contract_node =
          build_failure_specification(
              *specification,
              function_context,
              diagnostics);
      auto *outcome = dynamic_cast<ir::failure::Outcome *>(
          contract_node.get());

      if (outcome == nullptr)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5016",
            "failure-aware function has an invalid Outcome contract",
            specification->range());

        return nullptr;
      }

      SourceRange body_range{};
      if (specification_index + 1 < node.child_count())
      {
        const syntax::SyntaxNode *first_body =
            node.child(specification_index + 1);
        const syntax::SyntaxNode *last_body =
            node.child(node.child_count() - 1);

        if (first_body != nullptr && last_body != nullptr &&
            first_body->range().valid() && last_body->range().valid())
        {
          body_range = SourceRange{
              first_body->range().begin(),
              last_body->range().end()};
        }
      }

      auto function =
          std::make_unique<ir::failure::FailureAwareFunction>(
              node.range(),
              success_type->range(),
              declarator->range(),
              body_range,
              std::unique_ptr<ir::failure::Outcome>(
                  static_cast<ir::failure::Outcome *>(
                      contract_node.release())));

      if (!function->valid())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5017",
            "unable to construct a valid failure-aware function IR node",
            node.range());

        return nullptr;
      }

      std::unique_ptr<ir::Program> surrounding_source;

      for (const syntax::SyntaxNode &child : node.children())
      {
        if (&child == success_type || &child == declarator ||
            &child == specification)
        {
          continue;
        }

        if (child.range().valid() &&
            child.range().end_offset() <= specification->range().begin_offset())
        {
          std::unique_ptr<ir::IrNode> preserved = build_ir_node(
              child,
              function_context,
              diagnostics);
          if (!preserved)
            return nullptr;

          if (!surrounding_source)
            surrounding_source = std::make_unique<ir::Program>();

          surrounding_source->add(std::move(preserved));
          continue;
        }

        std::unique_ptr<ir::IrNode> lowered =
            build_ir_node(
                child,
                function_context,
                diagnostics);
        if (!lowered)
        {
          if (diagnostics.has_errors())
            return nullptr;

          continue;
        }

        function->add_child(std::move(lowered));
      }

      if (surrounding_source)
      {
        surrounding_source->add(std::move(function));
        return surrounding_source;
      }

      return function;
    }

    bool build_ir_children(
        const syntax::SyntaxNode &syntax_node,
        ir::IrNode &ir_node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      for (const syntax::SyntaxNode &child :
           syntax_node.children())
      {
        std::unique_ptr<ir::IrNode> lowered =
            build_ir_node(
                child,
                context,
                diagnostics);

        if (!lowered)
        {
          if (diagnostics.has_errors())
            return false;

          continue;
        }

        ir_node.add_child(
            std::move(lowered));
      }

      return !diagnostics.has_errors();
    }

    std::unique_ptr<ir::IrNode>
    build_cxx_region(
        const syntax::SyntaxNode &node)
    {
      return std::make_unique<ir::IrNode>(
          ir::IrKind::CxxRegion,
          node.range());
    }

    std::unique_ptr<ir::IrNode>
    build_failure_specification(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (node.child_count() != 1)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5001",
            "failure specification must contain exactly one failure type",
            node.range());

        return nullptr;
      }

      const syntax::SyntaxNode *type_node =
          node.child(0);

      if (
          type_node == nullptr || !type_node->range().valid() || type_node->range().empty())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5002",
            "failure specification has no valid failure type",
            node.range());

        return nullptr;
      }

      context.failure_type_range =
          type_node->range();

      auto outcome =
          std::make_unique<ir::failure::Outcome>(
              node.range(),
              type_node->range());

      if (!outcome->valid())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5003",
            "unable to construct a valid Outcome IR node",
            node.range());

        return nullptr;
      }

      return outcome;
    }

    std::unique_ptr<ir::IrNode>
    build_fail_statement(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (!context.failure_type_range.has_value())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5004",
            "failure statement has no active failure contract",
            node.range());

        return nullptr;
      }

      if (node.child_count() != 1)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5005",
            "failure statement must contain exactly one operand",
            node.range());

        return nullptr;
      }

      const syntax::SyntaxNode *operand_syntax =
          node.child(0);

      if (operand_syntax == nullptr)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5006",
            "failure statement has no operand",
            node.range());

        return nullptr;
      }

      IrBuildContext operand_context =
          context;

      std::unique_ptr<ir::IrNode> operand =
          build_ir_node(
              *operand_syntax,
              operand_context,
              diagnostics);

      if (!operand)
        return nullptr;

      auto failure =
          std::make_unique<ir::failure::Failure>(
              node.range(),
              *context.failure_type_range,
              std::move(operand));

      if (!failure->valid())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5007",
            "unable to construct a valid Failure IR node",
            node.range());

        return nullptr;
      }

      return failure;
    }

    std::unique_ptr<ir::IrNode>
    build_try_expression(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (!context.failure_type_range.has_value())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5008",
            "failure propagation has no active failure contract",
            node.range());

        return nullptr;
      }

      if (node.child_count() != 1)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5009",
            "failure propagation must contain exactly one operand",
            node.range());

        return nullptr;
      }

      const syntax::SyntaxNode *operand_syntax =
          node.child(0);

      if (operand_syntax == nullptr)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5010",
            "failure propagation has no operand",
            node.range());

        return nullptr;
      }

      IrBuildContext operand_context =
          context;

      std::unique_ptr<ir::IrNode> operand =
          build_ir_node(
              *operand_syntax,
              operand_context,
              diagnostics);

      if (!operand)
        return nullptr;

      auto propagation =
          std::make_unique<
              ir::failure::FailurePropagation>(
              node.range(),
              *context.failure_type_range,
              std::move(operand));

      if (!propagation->valid())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5011",
            "unable to construct a valid FailurePropagation IR node",
            node.range());

        return nullptr;
      }

      return propagation;
    }

    std::unique_ptr<ir::IrNode>
    build_return_statement(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (node.child_count() != 1)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5018",
            "return statement must contain exactly one operand",
            node.range());

        return nullptr;
      }

      const syntax::SyntaxNode *operand_syntax = node.child(0);
      if (operand_syntax == nullptr)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5019",
            "return statement has no operand",
            node.range());

        return nullptr;
      }

      std::unique_ptr<ir::IrNode> operand = build_ir_node(
          *operand_syntax,
          context,
          diagnostics);
      if (!operand)
        return nullptr;

      auto statement = std::make_unique<ir::failure::Return>(
          node.range(),
          std::move(operand));
      if (!statement->valid())
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5020",
            "unable to construct a valid Return IR node",
            node.range());

        return nullptr;
      }

      return statement;
    }

    std::unique_ptr<ir::IrNode>
    build_try_initialization(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (node.child_count() != 2)
      {
        diagnostics.emit(DiagnosticSeverity::Error, "VIXC5021", "try initialization must contain a declaration and propagation", node.range());
        return nullptr;
      }

      const syntax::SyntaxNode *declaration = node.child(0);
      const syntax::SyntaxNode *propagation_syntax = node.child(1);
      if (declaration == nullptr || propagation_syntax == nullptr)
        return nullptr;

      std::unique_ptr<ir::IrNode> propagation_node = build_try_expression(
          *propagation_syntax, context, diagnostics);
      auto *propagation = dynamic_cast<ir::failure::FailurePropagation *>(propagation_node.get());
      if (propagation == nullptr)
        return nullptr;

      auto initialization = std::make_unique<ir::failure::TryInitialization>(
          node.range(), declaration->range(),
          std::unique_ptr<ir::failure::FailurePropagation>(
              static_cast<ir::failure::FailurePropagation *>(propagation_node.release())));
      if (!initialization->valid())
      {
        diagnostics.emit(DiagnosticSeverity::Error, "VIXC5022", "unable to construct a valid try initialization IR node", node.range());
        return nullptr;
      }

      return initialization;
    }

    std::unique_ptr<ir::IrNode>
    build_ir_node(
        const syntax::SyntaxNode &node,
        IrBuildContext &context,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      switch (node.kind())
      {
      case syntax::SyntaxKind::Invalid:
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5012",
            "invalid syntax node reached IR construction",
            node.range());

        return nullptr;

      case syntax::SyntaxKind::TranslationUnit:
      {
        auto program =
            std::make_unique<ir::Program>(
                node.range());

        if (!build_ir_children(
                node,
                *program,
                context,
                diagnostics))
        {
          return nullptr;
        }

        return program;
      }

      case syntax::SyntaxKind::FunctionDeclaration:
        return build_function_declaration(node, diagnostics);

      case syntax::SyntaxKind::FunctionReturnType:
      case syntax::SyntaxKind::FunctionDeclarator:
      case syntax::SyntaxKind::CxxRegion:
        return build_cxx_region(node);

      case syntax::SyntaxKind::Identifier:
      case syntax::SyntaxKind::IntegerLiteral:
      case syntax::SyntaxKind::FloatingLiteral:
      case syntax::SyntaxKind::StringLiteral:
      case syntax::SyntaxKind::CharacterLiteral:
      case syntax::SyntaxKind::ParenthesizedExpression:
        return build_cxx_region(node);

      case syntax::SyntaxKind::FailureSpecification:
        return build_failure_specification(
            node,
            context,
            diagnostics);

      case syntax::SyntaxKind::FailStatement:
        return build_fail_statement(
            node,
            context,
            diagnostics);

      case syntax::SyntaxKind::ReturnStatement:
        return build_return_statement(
            node,
            context,
            diagnostics);

      case syntax::SyntaxKind::TryInitialization:
        return build_try_initialization(node, context, diagnostics);

      case syntax::SyntaxKind::TryExpression:
        return build_try_expression(
            node,
            context,
            diagnostics);
      }

      diagnostics.emit(
          DiagnosticSeverity::Error,
          "VIXC5013",
          "unsupported syntax node reached IR construction",
          node.range());

      return nullptr;
    }

    std::unique_ptr<ir::Program>
    build_program_ir(
        const syntax::SyntaxNode &root,
        diagnostics::DiagnosticEngine &diagnostics)
    {
      if (
          root.kind() != syntax::SyntaxKind::TranslationUnit)
      {
        diagnostics.emit(
            DiagnosticSeverity::Error,
            "VIXC5014",
            "IR construction requires a translation-unit root",
            root.range());

        return nullptr;
      }

      IrBuildContext context;

      auto program =
          std::make_unique<ir::Program>(
              root.range());

      program->reserve(
          root.child_count());

      for (const syntax::SyntaxNode &child :
           root.children())
      {
        std::unique_ptr<ir::IrNode> node =
            build_ir_node(
                child,
                context,
                diagnostics);

        if (!node)
        {
          if (diagnostics.has_errors())
            return nullptr;

          continue;
        }

        program->add(
            std::move(node));
      }

      return program;
    }

    std::vector<GeneratedSourceMapping>
    copy_source_map(
        const backends::cxx::CxxSourceMap &source_map)
    {
      std::vector<GeneratedSourceMapping> mappings;

      mappings.reserve(
          source_map.size());

      for (const backends::cxx::CxxSourceMapEntry &entry :
           source_map.entries())
      {
        mappings.push_back(
            GeneratedSourceMapping{
                entry.generated_begin(),
                entry.generated_end(),
                entry.original_range()});
      }

      return mappings;
    }

    FrontendResult make_result(
        bool success,
        FrontendOptions options,
        const diagnostics::DiagnosticEngine &diagnostics,
        std::string generated_output = {},
        std::vector<GeneratedSourceMapping> source_mappings = {})
    {
      return FrontendResult{
          success,
          options.action,
          options.backend,
          diagnostics.diagnostics(),
          std::move(generated_output),
          std::move(source_mappings)};
    }

  } // namespace

  FrontendResult Frontend::process(
      std::string_view source_name,
      std::string_view source,
      FrontendOptions options) const
  {
    source::SourceManager sources;
    diagnostics::DiagnosticEngine diagnostics;

    const SourceId source_id =
        sources.add_source(
            std::string{source_name},
            std::string{source});

    const source::SourceFile *source_file =
        sources.get(source_id);

    if (source_file == nullptr)
    {
      diagnostics.emit(
          DiagnosticSeverity::Fatal,
          "VIXC5000",
          "unable to register frontend source",
          SourceRange{});

      return make_result(
          false,
          options,
          diagnostics);
    }

    syntax::Lexer lexer{
        source_id,
        source_file->contents(),
        diagnostics};

    std::vector<syntax::Token> tokens =
        lexer.lex_all();

    if (diagnostics.has_errors())
    {
      return make_result(
          false,
          options,
          diagnostics);
    }

    syntax::Parser parser{
        std::move(tokens),
        diagnostics};

    syntax::SyntaxNode syntax_root =
        parser.parse();

    if (diagnostics.has_errors())
    {
      return make_result(
          false,
          options,
          diagnostics);
    }

    semantic::SemanticContext semantic_context{
        sources,
        diagnostics};

    semantic::SemanticAnalyzer semantic_analyzer{
        semantic_context};

    if (!semantic_analyzer.analyze(syntax_root))
    {
      return make_result(
          false,
          options,
          diagnostics);
    }

    if (options.action == FrontendAction::Analyze)
    {
      return make_result(
          true,
          options,
          diagnostics);
    }

    std::unique_ptr<ir::Program> program =
        build_program_ir(
            syntax_root,
            diagnostics);

    if (
        !program || diagnostics.has_errors())
    {
      return make_result(
          false,
          options,
          diagnostics);
    }

    lowering::LoweringContext lowering_context{
        sources,
        diagnostics};

    lowering::LoweringPipeline lowering_pipeline{
        lowering_context};

    if (!lowering_pipeline.lower(*program))
    {
      return make_result(
          false,
          options,
          diagnostics);
    }

    if (options.action == FrontendAction::Lower)
    {
      return make_result(
          true,
          options,
          diagnostics);
    }

    switch (options.backend)
    {
    case BackendTarget::Cxx:
    {
      backends::cxx::CxxBackend backend{
          sources,
          diagnostics};

      if (!backend.generate(*program))
      {
        return make_result(
            false,
            options,
            diagnostics);
      }

      std::vector<GeneratedSourceMapping> mappings;

      if (options.retain_source_map)
      {
        mappings =
            copy_source_map(
                backend.source_map());
      }

      return make_result(
          true,
          options,
          diagnostics,
          backend.output(),
          std::move(mappings));
    }
    }

    diagnostics.emit(
        DiagnosticSeverity::Fatal,
        "VIXC5015",
        "unsupported frontend backend target",
        SourceRange{});

    return make_result(
        false,
        options,
        diagnostics);
  }

} // namespace vixc
