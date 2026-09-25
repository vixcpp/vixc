/**
 *
 *  @file SyntaxNode.hpp
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

#if !defined(VIXC_SYNTAX_SYNTAX_NODE_HPP)
#define VIXC_SYNTAX_SYNTAX_NODE_HPP

#include "SyntaxKind.hpp"

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace vixc::syntax
{
  /**
   * @brief Represents one structural node in the VixC syntax tree.
   *
   * SyntaxNode is the parser-level representation of source structure understood
   * by VixC. Each node records its SyntaxKind, the original SourceRange from
   * which it was parsed, and zero or more child nodes.
   *
   * The syntax tree deliberately preserves a distinction between VixC-owned
   * syntax and ordinary C++. VixC constructs such as failure specifications,
   * fail statements, and try expressions receive explicit syntax nodes.
   * Surrounding C++ that does not require structural interpretation can remain
   * represented by CxxRegion nodes.
   *
   * SyntaxNode contains no semantic information. It does not determine types,
   * validate failure contracts, resolve names, or decide whether a construct is
   * legal in its surrounding program. Those responsibilities belong to semantic
   * analysis.
   *
   * Nodes also do not own copies of their source text. The SourceRange is the
   * canonical connection to the original source buffer. Text can therefore be
   * recovered through the source management layer when required without
   * duplicating source contents throughout the syntax tree.
   *
   * Children are owned directly by their parent node. The syntax tree therefore
   * has clear value ownership and does not require a separate allocation for
   * every node.
   */
  class SyntaxNode final
  {
  public:
    /**
     * @brief Creates an invalid syntax node.
     *
     * The node has SyntaxKind::Invalid, an invalid SourceRange, and no children.
     */
    SyntaxNode() = default;

    /**
     * @brief Creates a syntax node with no children.
     *
     * @param kind Structural kind of the node.
     * @param range Source range occupied by the node.
     */
    SyntaxNode(
        SyntaxKind kind,
        SourceRange range) noexcept
        : kind_(kind),
          range_(range)
    {
    }

    /**
     * @brief Creates a syntax node with an initial child collection.
     *
     * Child nodes are moved into this node and become owned by it.
     *
     * @param kind Structural kind of the node.
     * @param range Source range occupied by the node.
     * @param children Child nodes in source and structural order.
     */
    SyntaxNode(
        SyntaxKind kind,
        SourceRange range,
        std::vector<SyntaxNode> children)
        : kind_(kind),
          range_(range),
          children_(std::move(children))
    {
    }

    SyntaxNode(const SyntaxNode &) = default;
    SyntaxNode &operator=(const SyntaxNode &) = default;

    SyntaxNode(SyntaxNode &&) noexcept = default;
    SyntaxNode &operator=(SyntaxNode &&) noexcept = default;

    ~SyntaxNode() = default;

    /**
     * @brief Returns the structural kind of this node.
     *
     * @return Syntax kind assigned by the parser.
     */
    [[nodiscard]]
    constexpr SyntaxKind kind() const noexcept
    {
      return kind_;
    }

    /**
     * @brief Returns the source range represented by this node.
     *
     * The range refers to the original source rather than generated or lowered
     * code.
     *
     * A parser recovery node may contain an invalid range when no reliable
     * source association is available.
     *
     * @return Source range associated with the node.
     */
    [[nodiscard]]
    constexpr SourceRange range() const noexcept
    {
      return range_;
    }

    /**
     * @brief Reports whether this node has the requested syntax kind.
     *
     * @param expected Syntax kind to compare against.
     *
     * @return true when kind() equals expected, otherwise false.
     */
    [[nodiscard]]
    constexpr bool is(SyntaxKind expected) const noexcept
    {
      return kind_ == expected;
    }

    /**
     * @brief Reports whether the node is structurally invalid.
     *
     * This checks the SyntaxKind only. A node with a non-Invalid kind may still
     * later fail semantic validation.
     *
     * @return true when kind() is SyntaxKind::Invalid.
     */
    [[nodiscard]]
    constexpr bool is_invalid() const noexcept
    {
      return kind_ == SyntaxKind::Invalid;
    }

    /**
     * @brief Reports whether the node represents a VixC expression.
     *
     * @return true when the node kind is classified as an expression.
     */
    [[nodiscard]]
    constexpr bool is_expression() const noexcept
    {
      return syntax_kind_is_expression(kind_);
    }

    /**
     * @brief Reports whether the node belongs to failure syntax.
     *
     * @return true when the node represents a failure specification, fail
     *         statement, or try expression.
     */
    [[nodiscard]]
    constexpr bool is_failure_construct() const noexcept
    {
      return syntax_kind_is_failure_construct(kind_);
    }

    /**
     * @brief Adds a child node.
     *
     * The supplied node is moved into this node's child collection.
     *
     * Children should be added in their structural source order so later
     * frontend stages can traverse the syntax tree deterministically.
     *
     * @param child Syntax node to add.
     *
     * @return Reference to the newly stored child.
     */
    SyntaxNode &add_child(SyntaxNode child)
    {
      children_.push_back(std::move(child));
      return children_.back();
    }

    /**
     * @brief Reserves storage for child nodes.
     *
     * This operation is useful when the parser already knows how many children
     * a node will contain.
     *
     * Reserving storage does not change child_count().
     *
     * @param count Number of child slots to reserve.
     */
    void reserve_children(std::size_t count)
    {
      children_.reserve(count);
    }

    /**
     * @brief Returns all child nodes.
     *
     * Children are returned in the order established by the parser.
     *
     * @return Read-only child collection.
     */
    [[nodiscard]]
    const std::vector<SyntaxNode> &children() const noexcept
    {
      return children_;
    }

    /**
     * @brief Returns the number of direct child nodes.
     *
     * @return Number of children owned directly by this node.
     */
    [[nodiscard]]
    std::size_t child_count() const noexcept
    {
      return children_.size();
    }

    /**
     * @brief Reports whether the node has no children.
     *
     * @return true when child_count() is zero.
     */
    [[nodiscard]]
    bool is_leaf() const noexcept
    {
      return children_.empty();
    }

    /**
     * @brief Returns a child node by index.
     *
     * @param index Zero-based child index.
     *
     * @return Pointer to the requested child, or nullptr when index is outside
     *         the child collection.
     */
    [[nodiscard]]
    const SyntaxNode *child(std::size_t index) const noexcept
    {
      if (index >= children_.size())
        return nullptr;

      return &children_[index];
    }

  private:
    /// Structural category assigned by the parser.
    SyntaxKind kind_{SyntaxKind::Invalid};

    /// Original source region represented by this node.
    SourceRange range_{};

    /// Direct child nodes in parser-defined structural order.
    std::vector<SyntaxNode> children_;
  };

} // namespace vixc::syntax

#endif // VIXC_SYNTAX_SYNTAX_NODE_HPP
