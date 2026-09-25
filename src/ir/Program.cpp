/**
 *
 *  @file Program.cpp
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

#include "Program.hpp"

#include <utility>

namespace vixc::ir
{

  Program::Program() noexcept
      : IrNode(
            IrKind::Program,
            SourceRange{})
  {
  }

  Program::Program(SourceRange range) noexcept
      : IrNode(
            IrKind::Program,
            range)
  {
  }

  IrNode *Program::add(
      std::unique_ptr<IrNode> node)
  {
    return add_child(
        std::move(node));
  }

  void Program::reserve(std::size_t count)
  {
    reserve_children(count);
  }

  std::size_t Program::size() const noexcept
  {
    return child_count();
  }

  bool Program::empty() const noexcept
  {
    return is_leaf();
  }

  const IrNode *
  Program::node(std::size_t index) const noexcept
  {
    return child(index);
  }

  IrNode *
  Program::node(std::size_t index) noexcept
  {
    return child(index);
  }

} // namespace vixc::ir
