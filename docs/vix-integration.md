# Vix.cpp Integration

VixC is designed to integrate with Vix.cpp without becoming dependent on the Vix.cpp CLI, build system, runtime, or application model.

The relationship is intentionally one-directional.

VixC owns language frontend behavior.

Vix.cpp consumes that frontend as part of its developer workflow.

This separation allows VixC to remain independently embeddable while allowing Vix.cpp to provide the higher-level experience around building and running native applications.

## Responsibilities

VixC and Vix.cpp solve different parts of the development pipeline.

VixC is responsible for:

- source analysis
- VixC syntax
- VixC semantic rules
- diagnostics for VixC-owned constructs
- semantic intermediate representation
- backend-independent lowering
- backend generation
- generated-source provenance

Vix.cpp is responsible for application-level concerns such as:

- project discovery
- dependency resolution
- build configuration
- compiler selection
- native compilation
- linking
- incremental builds
- application execution
- development workflows

Vix.cpp should not reimplement VixC parsing or semantic analysis.

VixC should not reimplement the Vix.cpp build and application lifecycle.

## Library integration

The primary integration boundary is the VixC public C++ API.

Vix.cpp can embed VixC directly:

```cpp
#include <vixc/vixc.hpp>
```

A source unit can then be processed using:

```cpp
vixc::Frontend frontend;

const vixc::FrontendResult result =
    frontend.process(
        source_path,
        source);
```

The default frontend action is emission through the C++ backend.

When processing succeeds:

```cpp
result.generated_output()
```

contains generated C++ suitable for the next stage of the Vix.cpp build pipeline.

The integration does not require invoking the standalone `vixc` executable.

## CLI integration

The standalone `vixc` command exists as a thin client of the same public frontend.

It is useful for development, testing, inspection, and environments that do not embed the library directly.

Vix.cpp may invoke the executable when process isolation is desirable, but that should not become the semantic integration boundary.

The language implementation belongs to the VixC library.

The command-line tool only handles concerns such as:

- command-line arguments
- file input
- frontend option selection
- terminal diagnostic presentation
- generated output destination

Vix.cpp should therefore prefer direct library integration when practical.

## Source processing

Vix.cpp already knows which source units belong to an application.

For a source unit that requires VixC processing, Vix.cpp can load the source and pass its contents to the frontend.

For example:

```cpp
vixc::Frontend frontend;

vixc::FrontendOptions options;
options.action =
    vixc::FrontendAction::Emit;

const auto result =
    frontend.process(
        path.string(),
        source,
        options);
```

VixC does not need to know how Vix.cpp discovered the source file.

It also does not need to know whether the source came from:

- a project manifest
- a dependency graph
- a generated application
- a development server
- a test target
- a single-file invocation

Those are Vix.cpp concerns.

## Generated C++

The first VixC backend emits ordinary C++.

This makes integration with the existing Vix.cpp native toolchain straightforward.

After successful frontend processing, Vix.cpp can write:

```cpp
result.generated_output()
```

to a generated source location and compile that file using the same native compilation infrastructure already used for ordinary C++.

VixC does not invoke GCC, Clang, or MSVC itself.

That responsibility remains with Vix.cpp or another embedding application.

A conceptual Vix.cpp workflow is:

```text
VixC source
VixC frontend
generated C++
Vix.cpp build engine
native compiler
object files
linker
application
```

The generated C++ stage is an implementation boundary, not a second user-facing programming model.

## Generated file ownership

Generated C++ should be treated as build output.

Vix.cpp should place generated source under its normal generated-build area rather than modifying the user's original source.

For example, Vix.cpp may maintain generated files under its own internal build directory.

The exact directory layout belongs to Vix.cpp.

VixC only returns generated output.

This keeps the frontend independent from assumptions about project structure or build directories.

## Incremental builds

VixC should fit into Vix.cpp incremental build behavior.

A source file only needs to be regenerated when inputs affecting its VixC frontend result have changed.

At minimum, the cache identity may eventually depend on information such as:

- source contents
- VixC version
- frontend configuration
- backend target
- language feature configuration

VixC itself does not currently define a persistent compilation cache.

Vix.cpp can decide how generated frontend output participates in its existing build graph.

The important architectural requirement is determinism.

Given equivalent source and frontend configuration, VixC should produce equivalent generated output.

That allows normal build-system invalidation and caching strategies to remain effective.

## Diagnostics

VixC diagnostics should be surfaced directly through Vix.cpp rather than replaced with generic build failures.

For example:

```cpp
const auto result =
    frontend.process(
        path,
        source);

if (!result)
{
    for (const auto &diagnostic :
         result.diagnostics())
    {
        // Present through the Vix.cpp diagnostic interface.
    }
}
```

Vix.cpp is responsible for presentation.

VixC is responsible for semantic information such as:

- severity
- diagnostic code
- message
- source range

This allows Vix.cpp to preserve its own terminal formatting and developer experience without duplicating language analysis.

## Native compiler diagnostics

Generated C++ may still produce diagnostics from GCC, Clang, or MSVC.

Those diagnostics refer to generated source positions.

VixC source maps provide the basis for translating those positions back to original source.

A frontend result can contain:

```cpp
result.source_mappings()
```

and generated offsets can be queried with:

```cpp
result.original_range_for(offset)
```

A complete Vix.cpp integration can eventually use this information when native compiler diagnostics point into generated C++ corresponding to VixC source.

This is particularly important for generated representations of VixC semantic constructs.

Users should not be required to understand generated implementation code merely to locate an error in their source.

## Source-map retention

Source-map retention is controlled through `FrontendOptions`:

```cpp
vixc::FrontendOptions options;
options.retain_source_map = true;
```

It is enabled by default.

For normal Vix.cpp development builds, retaining mappings is useful because diagnostics and generated-code inspection benefit from source provenance.

A specialized build environment may disable them:

```cpp
options.retain_source_map = false;
```

Disabling source-map retention must not change program semantics.

## Analysis-only integration

Vix.cpp does not always need generated code.

Commands or future tooling may only need diagnostics.

In those cases:

```cpp
vixc::FrontendOptions options;
options.action =
    vixc::FrontendAction::Analyze;
```

allows Vix.cpp to run syntax and semantic validation without requesting backend output.

This can support workflows such as:

- editor checks
- fast validation
- development diagnostics
- future language-service integration

The same semantic frontend is used whether the caller intends to compile the program or only inspect it.

## Lowering-only integration

For frontend development and advanced tooling, Vix.cpp can request:

```cpp
vixc::FrontendAction::Lower
```

This executes the backend-independent frontend through lowering but does not invoke a backend.

The current public API intentionally does not expose internal IR through `FrontendResult`.

If future Vix.cpp tooling genuinely requires semantic IR access, that should be introduced through an explicit stable embedding interface rather than by depending directly on internal `src/` headers.

## VixC must not depend on Vix.cpp

VixC should remain buildable and usable without Vix.cpp.

The VixC library must not require:

- the Vix.cpp CLI
- Vix.cpp project manifests
- Vix.cpp modules
- Vix.cpp application discovery
- the Vix.cpp runtime
- the Vix.cpp build engine

This independence is important for testing and reuse.

For example, another native application should be able to embed:

```cpp
vixc::Frontend
```

without importing the rest of Vix.cpp.

The standalone VixC repository must therefore remain a valid frontend implementation on its own.

## Vix.cpp must not own language semantics

The reverse boundary is equally important.

Vix.cpp should not contain special logic such as:

```cpp
if source contains "fails"
```

or:

```cpp
replace "try" before compilation
```

or:

```cpp
rewrite "fail" in the build command
```

Those behaviors belong to the language frontend.

If Vix.cpp begins interpreting VixC syntax independently, the language acquires multiple implementations of its semantics.

That would make diagnostics, tooling, standalone VixC behavior, and Vix.cpp behavior diverge.

There should be one source of truth for VixC language meaning.

That source is VixC.

## Ordinary C++ files

Vix.cpp must continue to support ordinary C++.

VixC integration should not force every C++ translation unit through additional semantic transformation when no VixC language feature is involved unless doing so is intentionally part of the selected Vix workflow.

A project may contain ordinary C++ alongside source that uses VixC semantics.

The build system can therefore preserve the normal native path where appropriate.

This is important because VixC is intended to extend selected programming semantics while remaining connected to the existing C++ ecosystem.

## C++ compatibility path

When VixC processes ordinary C++ that does not contain VixC-owned constructs, the current frontend preserves it as C++ regions.

For example:

```cpp
int main()
{
    return 0;
}
```

can pass through VixC and produce equivalent ordinary C++.

This property allows Vix.cpp to introduce frontend processing incrementally rather than requiring applications to move into a completely separate language ecosystem.

Compatibility does not mean that every possible C++ construct is already handled perfectly by the current lexer and parser.

Full compatibility must be verified as the frontend matures.

## Failure integration

Failure is the first feature that will exercise the complete Vix.cpp integration.

The target source is:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

The intended Vix.cpp flow is that VixC validates this source and emits ordinary C++.

Vix.cpp then compiles that generated source using its normal native build infrastructure.

Vix.cpp should not need to understand what `fails`, `fail`, or VixC `try` mean.

It only needs to understand whether frontend processing succeeded and where the resulting source is located.

## Current Failure limitation

The current frontend does not yet accept the complete failure-aware function above.

The parser recognizes `fails`, `try`, and `fail`, but does not yet model the containing function declaration and body as one failure-aware semantic scope.

That prevents semantic analysis from keeping the failure contract active across the body.

This limitation must be resolved inside VixC before Vix.cpp integration attempts to work around it.

Vix.cpp should never compensate for missing language semantics in the frontend.

## Build failure behavior

Vix.cpp should stop native compilation of a source unit when VixC reports an Error or Fatal diagnostic.

For example:

```cpp
const auto result =
    frontend.process(
        path,
        source);

if (!result)
{
    present_diagnostics(result);
    return build_failure;
}
```

Generated output from a failed frontend invocation should not be treated as compilable source.

This preserves the frontend boundary and prevents later compiler errors from obscuring the original semantic problem.

## Versioning

Vix.cpp can inspect the VixC version through:

```cpp
vixc::version::current
```

or:

```cpp
vixc::version::string
```

For example:

```cpp
static_assert(
    vixc::version::at_least(
        0,
        1,
        0));
```

As the integration becomes stable, Vix.cpp can use this information to enforce frontend compatibility when necessary.

Version checks should remain explicit rather than depending on undocumented implementation behavior.

## Packaging

VixC provides a CMake package intended for embedding.

Installed applications can consume the library using the exported CMake target:

```cmake
find_package(vixc CONFIG REQUIRED)

target_link_libraries(
  application
  PRIVATE
    vixc::vixc
)
```

This allows Vix.cpp to consume a packaged VixC build without relying on internal repository paths.

During Vix.cpp development, VixC may also be included as a source dependency when appropriate.

The integration contract should remain the public target and public headers in either case.

## Vix CLI behavior

A future Vix.cpp command does not need to expose the internal VixC pipeline to normal users.

A developer should still be able to use the ordinary Vix workflow.

For example:

```text
vix build
```

can internally detect the relevant source path, invoke VixC, compile the resulting C++, and continue the existing build.

Likewise:

```text
vix run
```

can build through the same frontend integration before launching the application.

The presence of VixC should improve the programming model without forcing users to manually coordinate compiler stages.

The standalone:

```text
vixc
```

command remains useful for frontend-specific work, inspection, research, tests, and direct embedding environments.

## Integration principle

The Vix.cpp integration should remain thin.

VixC answers:

> What does this source mean, and what native representation preserves that meaning?

Vix.cpp answers:

> How is this application built, linked, run, and managed as a C++ application?

Keeping those responsibilities separate allows both projects to evolve without collapsing the language frontend into the application tooling layer.

VixC can improve language semantics independently.

Vix.cpp can improve the C++ application experience independently.

Their integration happens through a stable frontend contract rather than shared internal implementation.
