# VixC Architecture

VixC is a programming-language frontend for native software.

Its architecture separates language semantics from their concrete implementation in C++. VixC owns source analysis, syntax, semantic rules, intermediate representation, and lowering. A backend is responsible for translating the resulting semantic program into a target representation.

The first backend emits ordinary C++ that can be compiled by GCC, Clang, or MSVC.

VixC is not defined by source rewriting. The generated C++ backend is an implementation strategy for the first generation of the frontend, not the semantic model of the language.

## Architectural goals

The architecture is designed around several constraints.

VixC must be able to introduce programming semantics that cannot be represented coherently by ordinary library APIs alone.

Those semantics must be understood before code generation. A construct such as `fail` cannot merely become a textual replacement performed before compilation. The frontend must know that the construct means recoverable failure, validate where it is legal, represent it explicitly, and preserve that meaning until the backend selects a concrete implementation.

VixC must also remain compatible with the C++ ecosystem.

Existing C++ expressions, declarations, types, libraries, and toolchains remain usable. VixC should only own the parts of a program for which it introduces additional language meaning.

The architecture therefore distinguishes between ordinary C++ regions and VixC semantic constructs.

## Frontend stages

A VixC source unit passes through several independent stages.

### Source management

The source layer owns immutable source buffers and assigns each source a stable identifier.

Source locations are represented as byte offsets into those buffers.

`SourceLocation` identifies one position:

```cpp
SourceLocation{source_id, offset}
```

`SourceRange` represents a half-open interval:

```text
[begin, end)
```

All later frontend stages preserve source ranges rather than copying source text unnecessarily.

This allows diagnostics, semantic nodes, IR, generated source mappings, and future tooling to refer back to the original source.

The source layer does not perform filesystem I/O. A source may originate from a file, an editor buffer, generated text, a test, or another embedding environment.

### Diagnostics

Diagnostics are represented as structured frontend data.

A diagnostic contains:

- severity
- optional stable diagnostic code
- human-readable message
- optional source range

The diagnostic engine stores diagnostics in emission order.

It does not print them and does not decide how they should appear in a terminal, editor, build system, or IDE.

Presentation belongs to the embedding application.

### Lexical analysis

The lexer converts source bytes into tokens.

VixC-specific words such as:

```cpp
fails
fail
try
```

receive dedicated token kinds.

Ordinary C++ words remain ordinary identifiers unless VixC assigns them language meaning.

Tokens retain non-owning views into the immutable source buffer and preserve their exact source ranges.

Whitespace and comments may be skipped by lexical analysis while their original bytes remain available through the source manager.

This distinction is important because the backend may later preserve ordinary C++ directly from original source ranges.

### Syntax analysis

The parser constructs a syntax tree from the token stream.

The syntax layer answers structural questions such as:

- where a VixC construct begins and ends
- which source region is its operand
- which failure type follows `fails`
- which parts of the source remain ordinary C++

Syntax nodes do not decide backend representation.

A `FailStatement` means that the parser recognized the language construct:

```cpp
fail expression;
```

It does not mean that the parser has decided to generate `return`, throw an exception, or construct a particular C++ type.

The parser intentionally preserves ordinary C++ as `CxxRegion` nodes when VixC does not need to understand the internal structure of that region.

This allows VixC to coexist with C++ without implementing a complete replacement C++ frontend.

## Semantic analysis

Semantic analysis establishes what parsed constructs mean and whether their use is valid.

For the Failure model, semantic analysis is responsible for rules such as:

- a `fails E` specification declares a recoverable failure contract
- `fail value;` requires an active failure-aware computation
- `try expression` requires an active failure-aware computation
- failure propagation must eventually respect compatible failure contracts
- programmer errors are not recoverable Failure outcomes

Semantic analysis operates on language meaning rather than generated code.

The current semantic context contains source access, diagnostics, and active Failure contexts.

As the frontend grows, semantic information may also include resolved declarations, types, ownership information, compile-time values, and other language properties.

## Intermediate representation

VixC IR is a semantic intermediate representation.

It exists between semantic analysis and backend code generation.

The IR must not become a second syntax tree and must not become a collection of generated C++ fragments.

For example, the first Failure IR distinguishes:

- `Outcome`
- `Failure`
- `FailurePropagation`

These nodes describe semantic operations.

A `Failure` node means that a computation completes with a recoverable failure.

A `FailurePropagation` node means that another failure-aware computation is evaluated and its recoverable failure is propagated when necessary.

Neither node specifies how C++ implements that behavior.

The same semantic IR could eventually be consumed by another backend without changing the language meaning.

## Ordinary C++ regions

`CxxRegion` is the boundary used when VixC does not need to model ordinary C++ internally.

A C++ region retains its original `SourceRange`.

The C++ backend can recover the exact source bytes from the source manager and emit them directly.

This approach serves two purposes.

First, VixC does not need to become a complete C++ parser merely to introduce selected language semantics.

Second, existing C++ remains recognizable and compatible with the ecosystem instead of being reconstructed from a second internal representation.

`CxxRegion` should not become a mechanism for hiding VixC semantics from the frontend. When VixC introduces meaning, that meaning should receive an explicit syntax, semantic, and IR representation.

## Lowering

Lowering prepares semantic IR for backend processing.

The common lowering layer is backend-independent.

It may validate IR invariants, normalize semantic operations, introduce internal structure, or transform higher-level semantic nodes into forms that are easier for backends to consume.

Lowering must not introduce C++ merely because the first backend is C++.

For example, Failure lowering may reason about:

- failure contracts
- explicit failure production
- propagation
- control-flow requirements
- synthesized semantic operations

It should not decide that failure is implemented by `std::expected`, exceptions, macros, or another C++ mechanism.

That decision belongs at the backend boundary.

## Backend boundary

A backend consumes lowered VixC IR and produces a concrete target representation.

The first backend is `cxx`.

Its responsibilities include:

- preserving ordinary C++ regions
- selecting a C++ representation for VixC semantics
- generating deterministic source
- maintaining generated-source provenance
- reporting backend diagnostics

The backend does not own VixC language semantics.

If two backend implementations represent recoverable failure differently, both must preserve the same semantic meaning established by the frontend.

The common backend interface intentionally does not perform filesystem I/O or invoke a native compiler.

Generated output remains in memory and is returned to the caller.

## C++ backend

The C++ backend consists of three main pieces.

### `CxxBackend`

`CxxBackend` understands lowered VixC IR and decides how each semantic node should be represented in C++.

It is the semantic-to-C++ boundary.

### `CxxEmitter`

`CxxEmitter` owns generated C++ text.

It provides deterministic text emission, indentation support, and source-map recording.

The emitter itself does not understand Failure, Outcome, or other language semantics.

### `CxxSourceMap`

`CxxSourceMap` records provenance between generated byte ranges and original VixC source ranges.

Mappings use half-open generated intervals and retain the original `SourceRange`.

Generated code and original source do not need to have identical lengths.

The map represents provenance, not textual equivalence.

This infrastructure is intended to support future translation of diagnostics from GCC, Clang, or MSVC back to the original VixC source.

## Public frontend API

The public entry point is:

```cpp
vixc::Frontend
```

An embedding application supplies source text and configuration:

```cpp
vixc::Frontend frontend;

auto result = frontend.process(
    "main.cpp",
    source);
```

`Frontend` coordinates internal stages without exposing their implementation details through the public API.

`FrontendOptions` controls how far processing proceeds.

The supported actions are:

```cpp
FrontendAction::Analyze
FrontendAction::Lower
FrontendAction::Emit
```

`FrontendResult` owns the externally observable result of the invocation:

- success state
- diagnostics
- generated backend output
- generated source mappings

The result does not retain references to temporary parser, semantic, IR, lowering, or backend objects.

A `Frontend` instance can therefore be reused for independent source units.

## CLI boundary

The `vixc` command-line executable is a thin client of the public frontend API.

It is responsible for concerns such as:

- parsing command-line arguments
- reading input files or standard input
- selecting a frontend action
- printing diagnostics
- writing generated output

Language behavior must not live in the CLI.

Any application embedding the VixC library should be able to obtain the same frontend behavior without invoking the command-line tool.

This boundary is also important for Vix.cpp integration. Vix.cpp should invoke VixC through a stable frontend interface rather than becoming the owner of VixC parsing or semantic behavior.

## Ownership model

Source buffers are owned by `SourceManager`.

Tokens use non-owning views into those immutable source buffers.

Syntax nodes own their child syntax nodes.

The IR owns its child IR nodes through `std::unique_ptr`.

Frontend stages that reference source or diagnostic infrastructure do not own those objects.

`FrontendResult` copies the externally required information out of the temporary frontend state before the invocation returns.

This keeps lifetimes explicit and avoids requiring embedding applications to understand internal compiler object ownership.

## Source identity

Every source unit receives a `SourceId`.

Source locations and ranges use that identity throughout the frontend.

The intent is that one source identity model is shared by:

- source management
- syntax
- diagnostics
- semantics
- IR
- lowering
- backend source maps

A source identifier represents one immutable source snapshot for the duration of a frontend operation.

## Failure as the first vertical slice

Failure is the first semantic feature being implemented through the complete architecture.

The intended source model is:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

A complete implementation requires every frontend stage to understand only the part for which it is responsible.

The lexer recognizes the language tokens.

The parser identifies the failure-aware declaration and operations.

Semantic analysis establishes the active failure contract and validates `fail` and `try`.

IR construction represents Outcome, Failure, and FailurePropagation explicitly.

Lowering prepares those operations for backend implementation.

The C++ backend translates the resulting semantics into deterministic ordinary C++.

The generated source can then be compiled by an existing native C++ toolchain.

## Current architectural limitation

The current parser recognizes `fails`, `fail`, and `try`, but it does not yet represent the enclosing function declaration and body as one failure-aware syntax scope.

As a consequence, semantic analysis cannot yet keep the `fails E` contract naturally active across the function body.

This is not a backend problem.

The missing information belongs to the syntax and semantic layers.

The architecture should therefore be extended at that level rather than introducing a workaround in C++ emission.

The first Failure vertical slice is complete only when declaration-level scope is represented through parsing, semantic analysis, IR construction, lowering, and backend emission.

## Evolution

Future VixC research may introduce semantics for areas such as:

- choice and pattern matching
- asynchronous execution and cancellation
- ownership and lifetime models
- compile-time programming and reflection
- composition

Each feature should follow the same architectural rule.

Language meaning is established before backend representation.

A new surface construct should only be introduced when existing C++ mechanisms are insufficient to express the intended model coherently.

VixC should remain narrow enough that ordinary C++ stays ordinary C++, while the semantics VixC does own remain precise across the entire frontend.
