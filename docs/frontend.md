# VixC Frontend

VixC is a programming-language frontend for native software.

Its frontend is responsible for understanding source code before any backend representation is chosen. This includes source tracking, lexical analysis, syntax recognition, semantic validation, intermediate representation, lowering, diagnostics, and handoff to a concrete backend.

The first backend emits ordinary C++, but the frontend itself is not defined by C++ source generation.

## Public entry point

The public API begins with:

```cpp
vixc::Frontend
```

A caller provides a source name, source text, and optional frontend configuration:

```cpp
#include <vixc/vixc.hpp>

int main()
{
    const std::string source =
        "int value = 42;\n";

    vixc::Frontend frontend;

    const vixc::FrontendResult result =
        frontend.process(
            "main.cpp",
            source);

    if (!result)
        return 1;

    return 0;
}
```

`Frontend` coordinates the internal frontend pipeline.

Embedding applications do not need to construct the lexer, parser, semantic analyzer, IR, lowering pipeline, or backend directly.

## Source input

VixC processes source text supplied by the caller.

The public frontend does not require filesystem input.

A source can come from:

- a file
- an editor buffer
- standard input
- generated source
- a test
- another application embedding VixC

The first argument to `Frontend::process()` is a source identifier for diagnostics and source tracking.

For example:

```cpp
frontend.process(
    "src/main.cpp",
    source);
```

or:

```cpp
frontend.process(
    "<editor-buffer>",
    source);
```

The name does not need to refer to an existing filesystem path.

The frontend copies the source name and source text into the current frontend operation, so the caller does not need to preserve the input string views after `process()` returns.

## Frontend actions

`FrontendOptions` controls how far the frontend proceeds.

The three current actions are:

```cpp
vixc::FrontendAction::Analyze
vixc::FrontendAction::Lower
vixc::FrontendAction::Emit
```

### Analyze

`Analyze` runs the source, lexical, syntax, and semantic stages.

```cpp
vixc::FrontendOptions options;
options.action =
    vixc::FrontendAction::Analyze;
```

This mode is appropriate when a caller needs diagnostics or semantic validation but does not need generated code.

Typical future consumers include editors, language tooling, static analysis, and interactive development environments.

No backend output is produced.

### Lower

`Lower` performs semantic analysis and continues through IR construction and backend-independent lowering.

```cpp
vixc::FrontendOptions options;
options.action =
    vixc::FrontendAction::Lower;
```

This mode is useful for validating the complete semantic frontend without invoking a concrete backend.

The current public result does not expose internal IR objects. The IR remains an implementation detail of the frontend.

### Emit

`Emit` runs the complete frontend and invokes the configured backend.

```cpp
vixc::FrontendOptions options;
options.action =
    vixc::FrontendAction::Emit;
```

This is the default action.

The first backend target is:

```cpp
vixc::BackendTarget::Cxx
```

When generation succeeds, `FrontendResult::generated_output()` contains ordinary C++ source.

## Frontend pipeline

A frontend invocation currently proceeds through the following internal stages.

### Source registration

The source text is registered with `SourceManager`.

The manager owns the immutable source buffer and returns a stable `SourceId`.

Every later source location and source range refers back to this source identity.

### Lexing

`Lexer` converts source bytes into tokens.

The lexer recognizes VixC-specific words such as:

```cpp
fails
fail
try
```

with dedicated token kinds.

Ordinary C++ words remain identifiers unless VixC gives them specific language meaning.

Tokens preserve exact byte ranges into the original source.

### Parsing

`Parser` builds a syntax tree.

The parser recognizes VixC constructs and preserves uninterpreted C++ as `CxxRegion`.

Current VixC syntax nodes include constructs such as:

```cpp
FailureSpecification
FailStatement
TryExpression
```

The parser is structural.

It identifies where constructs exist and what source regions belong to them.

It does not decide whether a construct is semantically legal.

### Semantic analysis

`SemanticAnalyzer` checks the meaning of syntax nodes.

For Failure semantics, this includes validating that `fail` and `try` appear inside an active failure-aware computation.

The semantic stage is responsible for language rules, not generated representation.

It reports structured diagnostics through `DiagnosticEngine`.

### IR construction

After successful semantic analysis, VixC constructs are represented in backend-independent IR.

The current Failure IR includes:

```cpp
Outcome
Failure
FailurePropagation
```

Ordinary C++ remains represented as `CxxRegion`.

The IR does not contain generated C++ fragments.

Its purpose is to preserve semantics independently of backend implementation.

### Lowering

`LoweringPipeline` prepares IR for backend consumption.

The common lowering layer is backend-independent.

It validates and normalizes semantic IR but does not choose a C++ implementation strategy merely because the first backend emits C++.

Failure-specific lowering is handled by `FailureLowering`.

### Backend generation

When the selected action is `Emit`, the configured backend receives the lowered program.

The first backend is the C++ backend.

It emits ordinary C++ while preserving source provenance through `CxxSourceMap`.

The frontend does not invoke GCC, Clang, MSVC, CMake, Ninja, or a linker.

Compilation of generated C++ remains the responsibility of the caller or another tool such as Vix.cpp.

## Diagnostics

Diagnostics are returned through `FrontendResult`.

A diagnostic contains:

- severity
- optional diagnostic code
- message
- optional source range

Example:

```cpp
for (const vixc::Diagnostic &diagnostic :
     result.diagnostics())
{
    // Application-specific presentation.
}
```

The frontend does not print diagnostics itself.

This allows the same frontend library to be embedded in different environments without hard-coding terminal output.

The command-line tool is responsible for formatting diagnostics for terminal use.

## Frontend result

`FrontendResult` owns the externally visible state produced by one invocation.

It contains:

```cpp
result.success()
result.diagnostics()
result.generated_output()
result.source_mappings()
```

The result remains valid after `Frontend::process()` returns.

It does not depend on temporary parser, semantic, IR, lowering, or backend objects.

### Success state

A successful result means the requested frontend action completed without Error or Fatal diagnostics.

```cpp
if (!result.success())
{
    // Handle frontend failure.
}
```

`FrontendResult` also provides explicit boolean conversion:

```cpp
if (!result)
{
    // Handle frontend failure.
}
```

### Diagnostics

Diagnostics remain available after the frontend operation ends:

```cpp
for (const auto &diagnostic :
     result.diagnostics())
{
    // Inspect severity, code, message, and source range.
}
```

### Generated output

For `FrontendAction::Emit`, successful backend generation produces output:

```cpp
std::string_view generated =
    result.generated_output();
```

With the current backend, this output is C++ source.

`Analyze` and `Lower` do not normally produce generated output.

## Source mappings

The frontend can retain mappings between generated output and original source.

This behavior is controlled by:

```cpp
vixc::FrontendOptions options;
options.retain_source_map = true;
```

Source-map retention is enabled by default.

Each mapping records:

```cpp
generated_begin
generated_end
original_range
```

Generated offsets use half-open intervals.

The original range points back to the source processed by the frontend.

A caller can query the original source range for a generated byte offset:

```cpp
const vixc::SourceRange *range =
    result.original_range_for(offset);
```

This is part of the foundation for translating later native compiler diagnostics back to the original VixC source.

## Reuse

A `Frontend` object does not retain compilation state between invocations.

The same object can process multiple independent source buffers:

```cpp
vixc::Frontend frontend;

auto first =
    frontend.process(
        "first.cpp",
        first_source);

auto second =
    frontend.process(
        "second.cpp",
        second_source);
```

A failed invocation does not poison a later invocation.

Each call creates fresh source, diagnostic, syntax, semantic, IR, lowering, and backend state.

## Embedding boundary

The public frontend API intentionally hides internal compiler objects.

Applications embedding VixC should normally depend on headers under:

```cpp
<vixc/...>
```

or simply:

```cpp
#include <vixc/vixc.hpp>
```

Internal headers under `src/` are implementation details.

The frontend API is intended to remain usable by:

- Vix.cpp
- command-line tools
- editors
- build systems
- tests
- future IDE integrations
- other native applications embedding VixC

Vix.cpp integration should remain thin.

Vix.cpp can provide lifecycle, build, run, and application-level tooling around VixC, but the language frontend itself should remain independently embeddable.

## Failure and current frontend limitation

Failure is the first language feature used to validate the complete frontend architecture.

The intended source form is:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

The lexer already recognizes the required VixC tokens.

The parser already identifies `fails`, `fail`, and `try`.

Semantic analysis already knows that `fail` and `try` require an active failure-aware context.

The current missing piece is declaration-level syntax scope.

The parser does not yet represent the function declaration, its `fails LoadError` specification, and its body as one semantic unit.

Because of that, semantic analysis cannot naturally keep the failure contract active while walking the body.

This limitation must be solved in the frontend rather than bypassed in lowering or the backend.

Once declaration-level Failure scope is represented, the complete frontend should be able to carry that contract through semantic analysis, IR construction, lowering, and C++ emission.

## Design rule

VixC frontend stages should preserve a strict separation of responsibilities.

The lexer recognizes tokens.

The parser recognizes structure.

Semantic analysis establishes meaning.

IR preserves that meaning.

Lowering prepares semantic operations.

The backend selects a concrete representation.

The CLI presents the result.

No stage should silently absorb responsibilities that belong to another layer merely to make a particular example compile.

That separation is what allows VixC to evolve as a real frontend instead of becoming a collection of source transformations.
