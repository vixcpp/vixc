# VixC Research

This directory contains the research that informs possible future VixC language semantics.

VixC is not developed by adding syntax first and deciding what it means afterward.

Each research area begins with a programming problem, studies what C++ already provides, defines the semantic distinction being investigated, identifies unresolved questions, and only then moves toward implementation experiments.

The purpose of this directory is to preserve that reasoning separately from the production frontend.

Research documents are not automatically language specifications.

A syntax form, semantic hypothesis, IR shape, or backend strategy documented here remains experimental until implementation and real-program evidence justify making it stable.

## Research principle

VixC should only introduce language semantics when existing C++ mechanisms are insufficient to express an important concept coherently.

C++ already provides an extremely large language and ecosystem.

VixC therefore should not create alternative versions of:

- containers
- classes
- functions
- templates
- concepts
- ownership types
- task libraries
- error containers
- build systems
- dependency injection frameworks

merely because another spelling is possible.

A new language concept needs stronger justification.

It should provide semantic information that the frontend can use for properties such as:

- validation
- diagnostics
- control flow
- type relationships
- source tooling
- lowering
- composition with other language concepts

The value of a VixC feature should come from semantics rather than novelty.

## C++ remains the foundation

VixC research assumes continued access to the C++ ecosystem.

Ordinary C++ should remain the preferred solution when it already expresses the required behavior clearly.

That includes ordinary use of:

```cpp
std::expected
std::optional
std::variant
std::unique_ptr
std::shared_ptr
constexpr
consteval
concepts
coroutines
```

and the broader C++ language and library ecosystem.

VixC research is concerned with places where important meaning remains distributed across conventions, macros, libraries, generated code, runtime frameworks, or build tooling.

The objective is not to make C++ unrecognizable inside VixC.

## Semantics before representation

A recurring rule across the research areas is:

> Source-level meaning should be defined independently from the backend mechanism used to implement it.

For example:

```cpp
fail error;
```

should not mean:

```cpp
return std::unexpected(error);
```

merely because an early backend uses an expected-like representation.

Likewise, a future async computation should not mean:

```text
run this function on the VixC thread pool
```

merely because one experimental runtime uses a thread pool.

The language defines meaning.

Lowering exposes the operations required to preserve that meaning.

The backend chooses a concrete native representation.

This separation is necessary if VixC semantics are expected to remain stable while implementation evolves.

## Research areas

The current research is divided into five primary areas.

### [Failure and Outcome](failure-outcome/README.md)

Failure is the first active VixC semantic experiment.

The current model investigates:

```cpp
T operation(...) fails E
```

```cpp
fail error;
```

```cpp
auto value =
    try operation();
```

The broader Outcome hypothesis distinguishes:

```text
success
none
failure
stopped
```

Recoverable Failure is intentionally distinct from programmer error.

The immediate implementation goal is one complete vertical slice where a real failure-aware computation is parsed, semantically validated, represented in IR, lowered, emitted as ordinary C++, and compiled by an existing native toolchain.

This research is currently the highest implementation priority.

### [Choice and Match](choice-match/README.md)

Choice and Match research investigates explicit semantic alternatives and exhaustive selection.

The research asks whether VixC can provide meaningful language support for values with known alternatives without unnecessarily replacing existing C++ mechanisms such as:

```cpp
enum class
std::variant
std::optional
std::expected
std::visit
switch
```

Important questions include:

- exhaustiveness
- associated values
- payload binding
- partial matching
- value categories
- integration with Outcome
- whether VixC needs a new Choice type at all

This area remains exploratory.

### [Async and Cancellation](async-cancellation/README.md)

Async and Cancellation research investigates computation that completes later and may be intentionally stopped.

The central distinction is between:

```text
failure
```

and:

```text
stopped
```

Cancellation is not automatically treated as recoverable Failure.

The research also studies:

- structured concurrency
- parent-child task lifetime
- cancellation ownership
- cancellation propagation
- suspension
- interaction with C++ coroutines
- RAII
- detached work
- runtime independence

No VixC async syntax, scheduler, task type, or mandatory runtime has been selected.

### [Ownership and Lifetimes](ownership-lifetimes/README.md)

Ownership research investigates whether VixC can reason about important lifetime relationships while preserving ordinary C++ RAII, references, smart pointers, and move semantics.

The goal is not to replace C++ with another ownership system.

The strongest current research directions involve relationships such as:

```text
returned view borrows from argument
callback retains borrowed state
child task borrows parent-owned resource
detached work cannot retain a short-lived borrow
```

This work is expected to interact strongly with async and cancellation.

No borrow syntax or universal ownership model has been selected.

### [Compile-Time and Reflection](compile-time-reflection/README.md)

Compile-Time and Reflection research investigates semantic introspection and compile-time validation.

C++ already provides:

```cpp
constexpr
consteval
templates
concepts
type traits
```

The research therefore focuses on information that is difficult to access coherently through existing mechanisms, such as semantic declarations and future VixC-specific contracts.

Possible applications include:

- compile-time validation
- structured metadata
- improved diagnostics
- deterministic generated metadata
- reflection over VixC semantics

The research does not currently commit to declaration injection, arbitrary host execution, whole-program reflection, or a new template system.

### [Composition](composition/README.md)

Composition research investigates semantic relationships between software requirements and providers.

The goal is not to create another dependency injection container.

The current hypothesis is that VixC may eventually help describe and validate relationships such as:

```text
Service requires Logger
Service requires Database
Application provides Logger
Application provides Database
```

while preserving ordinary C++ construction and ownership.

Important questions include:

- provider identity
- requirement identity
- lifetime
- static versus dynamic resolution
- separate compilation
- provider Failure
- application-level graph validation
- the boundary between VixC and Vix.cpp

No component system or mandatory runtime has been selected.

## Research is not specification

Documents in this directory can contain:

- hypotheses
- candidate syntax
- conceptual notation
- implementation constraints
- open questions
- rejected or competing directions
- temporary frontend limitations

None of those automatically define stable VixC behavior.

A research example such as:

```cpp
T operation() fails E
```

means that this syntax is currently being investigated.

It does not mean VixC guarantees permanent compatibility with that spelling.

Likewise, conceptual notation such as:

```text
success(T)
failure(E)
```

describes semantic reasoning.

It is not necessarily source syntax or runtime representation.

## Research is not implementation convenience

A temporary implementation limitation must not silently become a language rule.

For example, the current frontend may initially support only:

```text
ordinary non-template functions
```

for one experiment.

That does not automatically mean templates should be permanently forbidden.

Likewise, if the first backend requires a particular C++ representation, that representation should not automatically become the language contract.

Every implementation decision should be classified carefully.

Useful categories include:

```text
semantic requirement
temporary experiment
frontend limitation
backend constraint
implementation convenience
stable language contract
```

The distinction matters.

## Evidence before stability

A feature should not become stable merely because a small test passes.

Research should eventually include real C++ programs involving properties such as:

- RAII
- move-only values
- references
- templates
- exceptions
- `noexcept`
- multiple translation units
- native libraries
- preprocessor usage
- ordinary toolchains
- realistic compile times
- realistic runtime behavior

Different research areas require different evidence.

Failure needs real propagation and recovery paths.

Async needs real suspension, cancellation, and resource cleanup.

Ownership needs real escaping references and structured lifetime relationships.

Reflection needs real semantic declarations and compile-time diagnostics.

Composition needs real application dependency graphs.

## Diagnostics are part of language design

VixC research treats diagnostics as part of the feature rather than a later presentation concern.

If VixC owns a semantic rule, it should normally be able to diagnose violations using the source-level concept.

For example, a future stable Failure frontend should prefer:

```text
cannot propagate 'ReadError' through a computation that fails with 'LoadError'
```

over a generated C++ error involving internal helper templates.

This principle applies across the research areas.

Choice should diagnose missing alternatives.

Ownership should describe invalid lifetime relationships.

Composition should describe missing providers.

Compile-time reflection should identify the actual reflected declaration that violates a requirement.

The usefulness of the diagnostic is part of the evidence for whether a language feature is worthwhile.

## Source provenance

Every research area must consider source provenance.

VixC transforms semantic constructs into generated native code.

A small source construct may eventually produce several generated C++ operations.

The frontend should preserve enough information to relate generated code and diagnostics back to original source.

The existing frontend uses:

```cpp
SourceLocation
SourceRange
```

and generated C++ source mappings as the first infrastructure for this requirement.

Future features may require richer semantic provenance.

## Backend independence

The first VixC backend emits C++.

This is useful because it preserves access to:

- GCC
- Clang
- MSVC
- native optimization
- existing libraries
- platform APIs
- native tooling

However, research should not define semantics in terms of generated C++ implementation details.

The C++ backend is the first backend.

It is not the definition of VixC.

This distinction is particularly important when evaluating future language concepts.

## Runtime independence

A language feature should not automatically create a runtime dependency.

Failure may be representable entirely through generated C++.

Choice and Match may lower into ordinary native branching.

Compile-time reflection may disappear completely before runtime.

Static composition may lower into direct C++ object construction.

A runtime should be introduced only when the required semantics genuinely need runtime behavior.

Async and dynamic composition are more likely to require runtime cooperation, but even there VixC should avoid assuming one universal runtime before the semantics demand it.

## Compatibility

Compatibility with C++ is an architectural constraint.

Research must consider:

- existing identifiers
- existing keywords
- templates
- macros
- preprocessing
- exception syntax
- modules
- value categories
- RAII
- ABI
- ordinary native libraries

A feature that works only in isolated examples but conflicts heavily with realistic C++ source is not sufficient.

This is why contextual syntax, semantic integration, and frontend boundaries matter.

## Contextual keywords

Several research areas may eventually introduce new source words.

Examples under discussion include concepts such as:

```text
fail
fails
match
choice
reflect
component
```

Making each one an unconditional keyword could break existing C++ code.

The current frontend uses dedicated tokens for the first Failure experiment, but that implementation choice is not automatically permanent language policy.

Contextual recognition should be considered where it can preserve compatibility without making the grammar unreliable.

## Preprocessing

The C++ preprocessor remains a major architectural consideration.

Real C++ uses:

```cpp
#include
#define
#if
#ifdef
```

and macros can influence declarations and expressions.

VixC currently has a lightweight frontend rather than a complete C++ preprocessing and semantic engine.

Every research area must account for this limitation honestly.

Reflection, ownership analysis, arbitrary expression transformation, and advanced declaration reasoning may eventually require deeper integration with mature C++ frontend infrastructure.

## C++ semantic infrastructure

Some research areas require much richer C++ understanding than others.

Failure can begin with relatively narrow declaration and control-flow semantics.

Ownership requires real value categories, object lifetimes, and declaration identity.

Reflection requires semantic types and declarations.

Templates and macros increase these requirements further.

VixC should not automatically rebuild an entire C++ compiler frontend from scratch if mature infrastructure can provide the required semantic information.

At the same time, any external frontend integration must be evaluated against:

- dependency size
- compilation cost
- portability
- compiler independence
- architecture
- maintainability

This remains an open long-term decision.

## Relationship with Vix.cpp

VixC and Vix.cpp have separate responsibilities.

VixC owns language frontend semantics.

Vix.cpp owns the broader C++ application development workflow.

Some research areas may require project-level information.

For example:

- composition may require application-wide provider metadata
- reflection may require build-level metadata aggregation
- generated code must participate in incremental builds

Vix.cpp can provide project and build context without becoming the owner of VixC semantics.

VixC should remain independently embeddable.

## Research order

The research directories are not an instruction to implement every feature simultaneously.

Failure is currently the active vertical slice.

The other areas exist so that early architectural decisions do not unnecessarily block future work.

For example:

- Failure should leave room for `stopped`
- async should preserve the distinction between stopped and Failure
- ownership should compose with structured task lifetime
- Match may eventually consume Outcome alternatives
- reflection may expose Failure and ownership contracts
- composition may need Failure-aware provider initialization

These relationships matter even before those later features are implemented.

## First active milestone

The immediate VixC milestone remains a complete Failure computation.

A source such as:

```cpp
User load_user(int id) fails LoadError
{
    auto record =
        try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

must eventually pass coherently through:

1. source management
2. lexical analysis
3. parsing
4. declaration-level semantic scope
5. semantic analysis
6. semantic IR
7. backend-independent lowering
8. deterministic C++ generation
9. source mapping
10. native C++ compilation
11. runtime validation of success and Failure behavior

Until that path exists, adding additional language surface is lower priority.

## Research questions should remain visible

Unanswered questions are part of the work.

They should not be hidden merely because the current implementation needs an immediate decision.

When a decision is provisional, documentation should say so.

When evidence contradicts an initial hypothesis, the design should change.

The objective is not to preserve early syntax.

The objective is to discover durable semantics.

## What would justify a VixC language feature

A research area becomes a stronger candidate for stable language support when it demonstrates several properties.

It should solve a real problem that ordinary C++ does not already solve coherently enough.

Its source meaning should be understandable independently from generated implementation.

The frontend should gain meaningful semantic information from the feature.

That information should enable better validation, diagnostics, tooling, lowering, or composition.

The feature should preserve important C++ behavior.

Its generated representation should remain compatible with native toolchains.

Its runtime and compile-time costs should be measurable and acceptable.

Its interaction with other VixC semantics should remain coherent.

## What would argue against a feature

Research should also identify when a language feature is unnecessary.

A proposal should be reconsidered when:

- an ordinary C++ library already solves the problem clearly
- the syntax primarily saves characters
- semantic benefits are weak
- compatibility costs are high
- implementation requires reconstructing excessive amounts of C++
- diagnostics do not improve meaningfully
- runtime or compile-time costs are disproportionate
- interoperability requires pervasive wrappers
- the feature makes other semantic areas harder to define
- real programs become less understandable

Rejecting an unnecessary language feature is a successful research outcome.

## Stable implementation comes last

Production implementation should follow semantic confidence.

The preferred order is:

1. identify a concrete problem
2. study existing C++ solutions
3. define the semantic distinction
4. record open questions
5. build a narrow experiment
6. test real programs
7. measure diagnostics, runtime, and compile-time behavior
8. revise the model where evidence requires it
9. define stable contracts
10. implement the stable frontend path

This is not intended as a rigid bureaucratic process.

Its purpose is to avoid making irreversible language decisions based on the first implementation that happens to compile.

## Current position

VixC currently has one active language experiment, Failure and Outcome, and several intentionally exploratory research areas around it.

The project is not attempting to replace C++.

It is investigating whether selected semantics can be made clearer and more coherent through a dedicated frontend while preserving:

- the native C++ ecosystem
- existing compilers
- ordinary C++ where it already works well
- deterministic native output
- backend freedom
- source-level diagnostics
- future language evolution

Research exists to determine which ideas actually deserve to cross that boundary.

Only ideas supported by semantics, implementation evidence, and real programs should become durable VixC language contracts.
