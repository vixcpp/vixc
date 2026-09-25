/**
 *
 *  @file IrNode.hpp
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

#if !defined(VIXC_IR_IR_NODE_HPP)
#define VIXC_IR_IR_NODE_HPP

#include "IrKind.hpp"

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace vixc::ir
{

  /**
   * @brief Base representation of one node in the VixC intermediate
   * representation.
   *
   * IrNode provides the common information shared by all VixC IR nodes:
   * semantic kind, original source range, and ownership of nested IR nodes.
   *
   * The IR exists after syntax analysis and semantic validation. It therefore
   * represents frontend meaning rather than source grammar. A SyntaxNode records
   * how a construct appeared in source, while an IrNode records the semantic
   * construct that lowering and backend stages must preserve.
   *
   * IrNode is intentionally independent from generated C++. No node stores C++
   * fragments as its semantic representation, and backend-specific decisions
   * must not be encoded into the common IR merely because C++ is the first VixC
   * backend.
   *
   * The class is polymorphic so semantic domains can define specialized nodes
   * such as Outcome and Failure while still allowing Program and later lowering
   * stages to own them through std::unique_ptr<IrNode>.
   *
   * Child ownership is exclusive. An IR node owns its direct children, and the
   * complete IR therefore has one clear ownership tree without shared node
   * lifetime.
   *
   * Source ranges always refer to the original source program. They are retained
   * so diagnostics, lowering, generated-source mapping, and backend reporting can
   * trace IR operations back to the code that produced them.
   */
  class IrNode
  {
  public:
    /**
     * @brief Creates an IR node.
     *
     * The constructor does not verify that the supplied source range belongs to
     * an active SourceManager. Source validity is established by earlier
     * frontend stages.
     *
     * @param kind Semantic kind represented by this node.
     * @param range Original source range associated with the node.
     */
    explicit IrNode(
        IrKind kind,
        SourceRange range = {}) noexcept
        : kind_(kind),
          range_(range)
    {
    }

    IrNode(const IrNode &) = delete;
    IrNode &operator=(const IrNode &) = delete;

    IrNode(IrNode &&) noexcept = default;
    IrNode &operator=(IrNode &&) noexcept = default;

    /**
     * @brief Destroys the IR node and all children it owns.
     *
     * The destructor is virtual because specialized semantic IR nodes are
     * commonly owned through std::unique_ptr<IrNode>.
     */
    virtual ~IrNode() = default;

    /**
     * @brief Returns the semantic kind represented by this node.
     *
     * @return IR kind assigned when the node was created.
     */
    [[nodiscard]]
    constexpr IrKind kind() const noexcept
    {
      return kind_;
    }

    /**
     * @brief Returns the original source range associated with this node.
     *
     * Some synthesized IR nodes may have no direct source representation and
     * therefore contain an invalid SourceRange.
     *
     * @return Original source range of the node.
     */
    [[nodiscard]]
    constexpr SourceRange range() const noexcept
    {
      return range_;
    }

    /**
     * @brief Reports whether this node has the requested IR kind.
     *
     * @param expected IR kind to compare against.
     *
     * @return true when kind() equals expected.
     */
    [[nodiscard]]
    constexpr bool is(IrKind expected) const noexcept
    {
      return kind_ == expected;
    }

    /**
     * @brief Reports whether this node belongs to the failure model.
     *
     * @return true when the IR kind represents Outcome, Failure, or failure
     *         propagation semantics.
     */
    [[nodiscard]]
    constexpr bool is_failure_construct() const noexcept
    {
      return ir_kind_is_failure_construct(kind_);
    }

    /**
     * @brief Reports whether this node preserves ordinary C++ source.
     *
     * @return true when kind() is IrKind::CxxRegion.
     */
    [[nodiscard]]
    constexpr bool is_cxx_region() const noexcept
    {
      return ir_kind_is_cxx_region(kind_);
    }

    /**
     * @brief Adds an owned child node.
     *
     * Ownership of the supplied node is transferred to this IrNode.
     *
     * Null child pointers are ignored. This prevents null entries from becoming
     * part of the IR ownership tree.
     *
     * @param child Child node whose ownership will be transferred.
     *
     * @return Pointer to the stored child, or nullptr when child was null.
     */
    IrNode *add_child(std::unique_ptr<IrNode> child)
    {
      if (!child)
        return nullptr;

      IrNode *stored = child.get();

      children_.push_back(
          std::move(child));

      return stored;
    }

    /**
     * @brief Constructs and adds a specialized child node.
     *
     * The requested type must derive from IrNode. Construction arguments are
     * forwarded directly to the child constructor.
     *
     * @tparam Node Concrete IR node type.
     * @tparam Args Constructor argument types.
     *
     * @param args Arguments forwarded to Node's constructor.
     *
     * @return Reference to the newly constructed child.
     */
    template <typename Node, typename... Args>
    Node &emplace_child(Args &&...args)
    {
      auto child =
          std::make_unique<Node>(
              std::forward<Args>(args)...);

      Node &stored = *child;

      children_.push_back(
          std::move(child));

      return stored;
    }

    /**
     * @brief Reserves storage for direct child nodes.
     *
     * Reserving capacity does not create nodes or change child_count().
     *
     * @param count Number of child slots to reserve.
     */
    void reserve_children(std::size_t count)
    {
      children_.reserve(count);
    }

    /**
     * @brief Returns all direct child nodes.
     *
     * The collection preserves insertion order.
     *
     * Callers receive read-only access to ownership pointers. Mutation of the
     * ownership structure remains controlled by IrNode.
     *
     * @return Read-only collection of owned child nodes.
     */
    [[nodiscard]]
    const std::vector<std::unique_ptr<IrNode>> &
    children() const noexcept
    {
      return children_;
    }

    /**
     * @brief Returns the number of direct child nodes.
     *
     * @return Number of children owned by this node.
     */
    [[nodiscard]]
    std::size_t child_count() const noexcept
    {
      return children_.size();
    }

    /**
     * @brief Reports whether the node has no direct children.
     *
     * @return true when child_count() is zero.
     */
    [[nodiscard]]
    bool is_leaf() const noexcept
    {
      return children_.empty();
    }

    /**
     * @brief Returns a direct child by index.
     *
     * The returned pointer remains owned by this node.
     *
     * @param index Zero-based child index.
     *
     * @return Pointer to the requested child, or nullptr when index is outside
     *         the child collection.
     */
    [[nodiscard]]
    const IrNode *child(std::size_t index) const noexcept
    {
      if (index >= children_.size())
        return nullptr;

      return children_[index].get();
    }

    /**
     * @brief Returns mutable access to a direct child by index.
     *
     * Ownership remains with this node.
     *
     * This overload is intended for controlled IR construction and lowering
     * stages that need to mutate an existing node in place.
     *
     * @param index Zero-based child index.
     *
     * @return Pointer to the requested child, or nullptr when index is outside
     *         the child collection.
     */
    [[nodiscard]]
    IrNode *child(std::size_t index) noexcept
    {
      if (index >= children_.size())
        return nullptr;

      return children_[index].get();
    }

  protected:
    /**
     * @brief Updates the source range associated with this IR node.
     *
     * This operation is protected because source identity is part of the IR
     * invariant and should only be adjusted by specialized IR construction or
     * transformation code.
     *
     * @param range New original source range.
     */
    constexpr void set_range(SourceRange range) noexcept
    {
      range_ = range;
    }

  private:
    /// Semantic kind of this IR node.
    IrKind kind_{IrKind::Invalid};

    /// Original source region associated with the semantic operation.
    SourceRange range_{};

    /// Direct IR children owned exclusively by this node.
    std::vector<std::unique_ptr<IrNode>> children_;
  };

} // namespace vixc::ir

#endif // VIXC_IR_IR_NODE_HPP
