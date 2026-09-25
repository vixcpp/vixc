/**
 *
 *  @file Program.hpp
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

#if !defined(VIXC_IR_PROGRAM_HPP)
#define VIXC_IR_PROGRAM_HPP

#include "IrNode.hpp"

#include <vixc/SourceRange.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace vixc::ir
{
  /**
   * @brief Root container for a VixC intermediate representation.
   *
   * Program represents the semantically processed contents of one frontend
   * compilation unit after syntax analysis and semantic validation have
   * established the constructs that must enter the IR.
   *
   * The program owns its top-level IR nodes through the child ownership model
   * provided by IrNode. Those nodes remain in source order so lowering and
   * backend stages can process ordinary C++ regions and VixC-owned semantics
   * deterministically.
   *
   * Program does not contain generated C++ and does not prescribe a backend
   * representation. It is the root of the backend-independent semantic IR.
   *
   * Ordinary source that does not require VixC transformation can appear as
   * CxxRegion nodes. Language constructs owned by VixC are represented by
   * specialized IR nodes such as Outcome, Failure, and FailurePropagation.
   *
   * The source range associated with Program normally spans the complete source
   * unit represented by the IR. A synthesized program may use an invalid range
   * when no single original source region describes the complete program.
   */
  class Program final : public IrNode
  {
  public:
    /**
     * @brief Creates an empty program without a source range.
     *
     * The program is valid as an IR container even when no original source
     * range is associated with the root itself.
     */
    Program() noexcept;

    /**
     * @brief Creates an empty program associated with a source range.
     *
     * @param range Original source range represented by the program.
     */
    explicit Program(SourceRange range) noexcept;

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

    Program(Program &&) noexcept = default;
    Program &operator=(Program &&) noexcept = default;

    ~Program() override = default;

    /**
     * @brief Adds a top-level IR node to the program.
     *
     * Ownership of the supplied node is transferred to the program.
     *
     * Nodes should be added in semantic source order so later lowering and
     * backend stages observe a deterministic program sequence.
     *
     * A null node is ignored.
     *
     * @param node Top-level IR node to add.
     *
     * @return Pointer to the stored node, or nullptr when node was null.
     */
    IrNode *add(std::unique_ptr<IrNode> node);

    /**
     * @brief Constructs and adds a top-level specialized IR node.
     *
     * The requested type must derive from IrNode. Constructor arguments are
     * forwarded directly to the node constructor.
     *
     * @tparam Node Concrete IR node type.
     * @tparam Args Constructor argument types.
     *
     * @param args Arguments forwarded to Node's constructor.
     *
     * @return Reference to the newly constructed top-level node.
     */
    template <typename Node, typename... Args>
    Node &emplace(Args &&...args)
    {
      return emplace_child<Node>(
          std::forward<Args>(args)...);
    }

    /**
     * @brief Reserves storage for top-level IR nodes.
     *
     * Reserving storage does not create nodes and does not change size().
     *
     * @param count Number of top-level node slots to reserve.
     */
    void reserve(std::size_t count);

    /**
     * @brief Returns the number of top-level IR nodes.
     *
     * @return Number of nodes directly owned by the program.
     */
    [[nodiscard]]
    std::size_t size() const noexcept;

    /**
     * @brief Reports whether the program contains no top-level IR nodes.
     *
     * @return true when size() is zero, otherwise false.
     */
    [[nodiscard]]
    bool empty() const noexcept;

    /**
     * @brief Returns a top-level IR node by index.
     *
     * The returned node remains owned by the program.
     *
     * @param index Zero-based top-level node index.
     *
     * @return Pointer to the requested node, or nullptr when index is outside
     *         the program.
     */
    [[nodiscard]]
    const IrNode *node(std::size_t index) const noexcept;

    /**
     * @brief Returns mutable access to a top-level IR node by index.
     *
     * Ownership remains with the program.
     *
     * This overload is intended for controlled IR construction and lowering
     * stages that need to modify a node in place.
     *
     * @param index Zero-based top-level node index.
     *
     * @return Pointer to the requested node, or nullptr when index is outside
     *         the program.
     */
    [[nodiscard]]
    IrNode *node(std::size_t index) noexcept;
  };

} // namespace vixc::ir

#endif // VIXC_IR_PROGRAM_HPP
