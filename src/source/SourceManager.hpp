/**
 *
 *  @file SourceManager.hpp
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

#if !defined(VIXC_SOURCE_SOURCE_MANAGER_HPP)
#define VIXC_SOURCE_SOURCE_MANAGER_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <vixc/SourceLocation.hpp>

namespace vixc::source
{

  class SourceFile;

  /**
   * @brief Stable identifier for a source unit owned by a SourceManager.
   *
   * Source identifiers are zero-based and are assigned in insertion order.
   * An identifier remains valid for the lifetime of the SourceManager that
   * created it.
   *
   * SourceId values are local to a SourceManager instance. An identifier from
   * one manager must not be used to access another manager.
   */
  using SourceId = ::vixc::SourceId;

  /**
   * @brief Owns and identifies the source files processed by the frontend.
   *
   * SourceManager is the central registry for source units during a VixC
   * frontend invocation. Every source added to the manager receives a stable
   * SourceId that can later be stored by syntax nodes, semantic information,
   * diagnostics, source locations, and other frontend structures.
   *
   * The manager owns every SourceFile registered with it. Source files are
   * allocated independently so their addresses remain stable even when more
   * sources are added and the internal collection grows.
   *
   * SourceManager does not interpret source text and does not perform parsing,
   * semantic analysis, or diagnostics. Its responsibility is limited to source
   * ownership and source identity.
   *
   * The manager also does not require a source to originate from the
   * filesystem. A source can represent a file on disk, an editor buffer,
   * generated input, a test fixture, or any other in-memory source unit.
   *
   * SourceManager does not currently remove individual sources. This keeps
   * SourceId values stable for the complete lifetime of a frontend operation.
   */
  class SourceManager final
  {
  public:
    /**
     * @brief Creates an empty source manager.
     */
    SourceManager();

    SourceManager(const SourceManager &) = delete;
    SourceManager &operator=(const SourceManager &) = delete;

    SourceManager(SourceManager &&) noexcept;
    SourceManager &operator=(SourceManager &&) noexcept;

    /**
     * @brief Destroys the manager and all source files owned by it.
     */
    ~SourceManager();

    /**
     * @brief Adds a source unit to the manager.
     *
     * A new SourceFile is created from the supplied path and contents. The
     * returned identifier is assigned in insertion order and remains associated
     * with that source for the lifetime of this manager.
     *
     * The path is treated as a logical source identifier. SourceManager does
     * not require it to exist on the filesystem and does not attempt to
     * normalize, resolve, or deduplicate it.
     *
     * Adding multiple source units with the same path is allowed. Each call
     * creates a distinct source and receives a distinct SourceId.
     *
     * @param path Path or logical identifier associated with the source.
     * @param contents Complete source text.
     *
     * @return Stable identifier assigned to the new source.
     */
    [[nodiscard]]
    SourceId add_source(std::string path, std::string contents);

    /**
     * @brief Returns a source file by identifier.
     *
     * The returned pointer refers to the SourceFile owned by this manager and
     * remains valid until the manager is destroyed or moved from.
     *
     * @param source_id Identifier previously returned by add_source().
     *
     * @return Pointer to the requested source, or nullptr when source_id does
     *         not identify a source owned by this manager.
     */
    [[nodiscard]]
    const SourceFile *get(SourceId source_id) const noexcept;

    /**
     * @brief Reports whether a source identifier belongs to this manager.
     *
     * @param source_id Source identifier to test.
     *
     * @return true when source_id identifies a registered source, otherwise
     *         false.
     */
    [[nodiscard]]
    bool contains(SourceId source_id) const noexcept;

    /**
     * @brief Returns the number of source units owned by the manager.
     *
     * @return Number of registered source files.
     */
    [[nodiscard]]
    std::size_t source_count() const noexcept;

    /**
     * @brief Reports whether the manager contains no source files.
     *
     * @return true when no source has been registered, otherwise false.
     */
    [[nodiscard]]
    bool empty() const noexcept;

  private:
    /**
     * @brief Source files indexed by their SourceId.
     *
     * The vector index is the source identifier. Each SourceFile is allocated
     * independently so growing the vector does not move the SourceFile object
     * itself.
     */
    std::vector<std::unique_ptr<SourceFile>> sources_;
  };

} // namespace vixc::source

#endif // VIXC_SOURCE_SOURCE_MANAGER_HPP
