/**
 *
 *  @file Backend.hpp
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

#if !defined(VIXC_BACKENDS_BACKEND_HPP)
#define VIXC_BACKENDS_BACKEND_HPP

#include <string_view>

namespace vixc::ir
{

  class Program;

} // namespace vixc::ir

namespace vixc::backends
{
  /**
   * @brief Common interface implemented by VixC code-generation backends.
   *
   * Backend defines the boundary between the backend-independent VixC frontend
   * and a concrete target representation.
   *
   * Syntax analysis, semantic analysis, IR construction, and common lowering
   * establish the meaning of a program before a backend is invoked. A backend
   * receives that prepared Program and translates it into its target form.
   *
   * The first VixC backend emits ordinary C++, but the common backend interface
   * does not encode C++ concepts. Backend-specific source generation, source
   * maps, formatting, helper code, runtime conventions, and output storage
   * belong to the concrete backend implementation.
   *
   * Backend implementations must not redefine VixC semantics. They are
   * responsible for preserving semantics already represented by the lowered IR.
   *
   * The interface intentionally does not perform file I/O or invoke external
   * compilers. Generation produces an in-memory backend result owned by the
   * concrete implementation. The caller decides whether that result is written
   * to disk, passed to another tool, inspected, cached, or compiled.
   *
   * Backend failures should be reported through the diagnostic infrastructure
   * owned by the concrete backend. generate() communicates whether generation
   * completed successfully without forcing callers to parse textual output.
   */
  class Backend
  {
  public:
    Backend() = default;

    Backend(const Backend &) = delete;
    Backend &operator=(const Backend &) = delete;

    Backend(Backend &&) = delete;
    Backend &operator=(Backend &&) = delete;

    /**
     * @brief Destroys the backend.
     *
     * The destructor is virtual because concrete backends are expected to be
     * used through Backend references or pointers.
     */
    virtual ~Backend() = default;

    /**
     * @brief Returns the stable name of the backend.
     *
     * The name identifies the backend implementation rather than an output file
     * extension or executable toolchain.
     *
     * The first backend returns `"cxx"`.
     *
     * @return Stable backend name.
     */
    [[nodiscard]]
    virtual std::string_view name() const noexcept = 0;

    /**
     * @brief Generates the backend representation of a lowered VixC program.
     *
     * The supplied program must already have passed semantic analysis and common
     * lowering successfully.
     *
     * Concrete backends own any generated output and expose it through their own
     * typed interfaces. The common Backend abstraction only coordinates the
     * generation operation.
     *
     * A backend may be reused for multiple generation operations. Each concrete
     * implementation is responsible for resetting previous output before
     * processing a new program.
     *
     * @param program Lowered backend-independent VixC program.
     *
     * @return true when generation completes successfully, otherwise false.
     */
    [[nodiscard]]
    virtual bool generate(const ir::Program &program) = 0;

    /**
     * @brief Clears output and transient state from the previous generation.
     *
     * Resetting a backend does not change external frontend state such as source
     * files or diagnostics owned outside the backend.
     */
    virtual void reset() = 0;
  };

} // namespace vixc::backends

#endif // VIXC_BACKENDS_BACKEND_HPP
