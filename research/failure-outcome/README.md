# Failure and Outcome Research

This directory contains the research behind the first language-level semantic model explored by VixC: recoverable failure and computation outcomes.

The objective is not to design another error container for C++.

C++ already provides exceptions, `std::expected`, `std::optional`, error codes, status objects, custom result types, and application-specific conventions.

The research question is different:

> Can recoverable failure become an explicit language semantic while preserving ordinary C++, native performance, existing toolchains, and the freedom to choose an appropriate backend representation?

The current experiment investigates that question through three source-level constructs:

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

These constructs are provisional.

Their syntax, type-system interaction, lowering strategy, ABI representation, and long-term role remain subject to evidence from implementation and real programs.

## Research scope

The first experiment focuses on two computation outcomes:

```text
success(T)
failure(E)
```

The broader Outcome hypothesis distinguishes:

```text
success
none
failure
stopped
```

These states are intentionally separate.

`success` represents normal completion.

`none` represents normal completion without a value where absence has semantic meaning.

`failure` represents a declared recoverable problem.

`stopped` is reserved for computation that is intentionally interrupted, such as future cancellation semantics.

Programmer errors and contract violations are not currently modeled as ordinary outcomes.

The first vertical slice only attempts to make `success` and recoverable `failure` coherent.

## Current hypothesis

A failure-aware computation declares its recoverable Failure contract:

```cpp
User load_user(int id) fails LoadError
{
    // ...
}
```

Its conceptual outcomes are:

```text
success(User)
failure(LoadError)
```

The successful result remains `User`.

The Failure contract adds a recoverable `LoadError` outcome without requiring the source-level API to expose a particular result wrapper.

Explicit Failure is produced with:

```cpp
fail LoadError{};
```

Failure from another computation is propagated with:

```cpp
auto record =
    try read_record(id);
```

The intended semantic behavior is:

```text
evaluate the operand once

if success:
    expose the successful value

if failure:
    propagate the failure from the enclosing computation
```

This meaning exists independently of how the C++ backend eventually represents it.

## Why representation comes later

The first backend could theoretically represent:

```cpp
T operation() fails E
```

using an expected-like type.

It could also use:

- a generated result type
- a tagged native representation
- explicit status and storage
- exceptions
- another C++ mechanism

Choosing one representation before defining the semantics would make that implementation mechanism define the language.

VixC instead attempts to preserve Failure explicitly through:

- parsing
- semantic analysis
- semantic IR
- lowering
- backend generation

The backend is responsible for representation.

The frontend is responsible for meaning.

## Relationship with C++

This research does not assume that existing C++ failure mechanisms are incorrect.

Different C++ mechanisms provide useful properties.

Exceptions provide language-level propagation and stack unwinding.

`std::expected<T, E>` provides explicit success and error values.

`std::optional<T>` represents value or absence.

Error codes remain useful across system and ABI boundaries.

Custom result types can encode application-specific behavior.

VixC must understand what already exists before deciding which semantics justify frontend support.

The goal is therefore not:

```text
replace C++ error handling
```

The goal is to investigate whether a small language-level model can provide stable semantics above concrete representations while remaining interoperable with them.

## Research documents

The research is divided by question rather than by implementation stage.

### [problem.md](problem.md)

Defines the underlying problem.

It examines why unsuccessful computation cannot always be treated as one generic error state and introduces the working distinction between:

```text
success
none
failure
stopped
```

It also establishes the distinction between recoverable Failure and programmer error.

### [cpp-existing-models.md](cpp-existing-models.md)

Examines existing C++ mechanisms for representing unsuccessful computation.

This includes:

- error codes
- sentinel values
- exceptions
- `std::error_code`
- `std::optional`
- `std::expected`
- custom result types
- propagation macros
- variants
- callbacks
- futures
- coroutines
- assertions
- termination

The purpose is to identify what C++ already solves and where semantic information remains fragmented across conventions.

### [semantics.md](semantics.md)

Defines the current semantic hypothesis.

It describes the intended meaning of:

```cpp
fails
fail
try
```

independently of generated C++.

It also documents requirements around:

- Failure scope
- propagation
- single evaluation
- control flow
- RAII
- move-only values
- references
- exceptions
- `noexcept`
- templates
- Failure compatibility

### [lowering.md](lowering.md)

Examines how validated Failure semantics should be transformed into backend-ready operations.

The central question is how to make propagation and outcome control flow explicit without choosing a C++ storage representation too early.

The document also covers:

- declaration-level lowering
- synthesized values
- expression normalization
- evaluation order
- temporary lifetimes
- RAII
- ABI concerns
- templates
- source provenance
- candidate backend representations

### [diagnostics.md](diagnostics.md)

Defines the diagnostic responsibilities of the Failure frontend.

It distinguishes between:

- lexical diagnostics
- parsing diagnostics
- semantic Failure diagnostics
- IR invariant failures
- lowering diagnostics
- backend diagnostics
- native compiler diagnostics

The central rule is that VixC-owned semantic mistakes should be diagnosed in VixC terminology before generated C++ obscures the original problem.

### [open-questions.md](open-questions.md)

Records unresolved design questions.

These include:

- whether `fails E` is the correct syntax
- whether Failure belongs to function type identity
- how propagation compatibility works
- whether Failure contracts can be inferred
- whether `try` is the correct propagation syntax
- interaction with exceptions and `noexcept`
- templates
- constructors
- virtual functions
- coroutines
- cancellation
- ABI
- preprocessing
- contextual keywords
- source maps
- native C++ interoperability
- control-flow IR
- runtime requirements

These questions are intentionally not hidden behind temporary implementation choices.

## Research method

The Failure work should proceed from semantic requirements rather than syntax preference.

A proposed behavior should be classified as one of:

```text
semantic requirement
implementation convenience
backend constraint
temporary experiment
stable language contract
```

These categories must remain separate.

For example, if the first C++ backend requires a movable result object, that does not automatically mean the VixC language should require every Failure type to be movable.

Likewise, if the first parser treats `fail` as an unconditional keyword, that does not automatically mean `fail` should become a permanent reserved word.

Implementation is evidence.

It is not specification by itself.

## Current frontend architecture

The Failure experiment currently passes through the main VixC frontend layers.

The source layer owns immutable source buffers and stable source locations.

The lexer recognizes the experimental Failure vocabulary.

The parser creates dedicated syntax nodes.

Semantic analysis validates Failure context.

The IR contains dedicated semantic nodes.

The lowering layer validates and prepares Failure semantics.

The C++ backend preserves ordinary C++ and provides the final native representation boundary.

Source provenance is retained throughout the pipeline.

This structure is already enough to test whether Failure can remain semantic rather than becoming source rewriting.

## Current IR

The current Failure IR contains:

```text
Outcome
Failure
FailurePropagation
```

`Outcome` represents the possible completion states of a computation.

`Failure` represents explicit recoverable failure production.

`FailurePropagation` represents propagation from another failure-aware computation.

These nodes carry semantic meaning.

They are not aliases for generated C++ fragments.

## Current implementation boundary

The first implementation is not yet a complete Failure language feature.

The parser recognizes:

```cpp
fails E
```

```cpp
fail expression;
```

```cpp
try expression
```

but does not yet represent the enclosing function declaration and its body as one failure-aware syntax scope.

Consider:

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

The intended semantic structure is one computation with:

```text
success type: User
failure type: LoadError
body:
    propagation
    explicit failure
    successful return
```

The current syntax tree does not yet provide that complete declaration-level relationship.

As a result, semantic analysis cannot naturally keep the `LoadError` Failure contract active across the function body.

This is the immediate architectural problem to solve.

## Why the current backend rejects Failure emission

The C++ backend currently rejects unresolved `Failure` and `FailurePropagation` nodes.

That behavior is deliberate.

The backend does not yet have enough declaration-level and control-flow information to emit them correctly.

For example, generating:

```cpp
return error;
```

for:

```cpp
fail error;
```

would assume a concrete function representation that has not been established.

Likewise, translating:

```cpp
try operation()
```

requires knowledge about:

- the result representation of `operation`
- the enclosing computation's result representation
- failure compatibility
- success extraction
- cleanup
- value category
- evaluation order

The backend should not guess these properties.

The missing structure must be established earlier in the frontend.

## First complete vertical slice

The immediate research target is one real Failure-aware function.

For example:

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

The experiment is complete only when this source can pass through the entire frontend with one coherent meaning.

That requires:

1. recognition of the complete declaration
2. declaration-level Failure scope
3. semantic validation of `fail`
4. semantic validation of `try`
5. explicit Outcome representation
6. semantic IR construction
7. backend-independent lowering
8. deterministic C++ generation
9. source provenance
10. successful compilation with an existing native C++ compiler
11. runtime behavior matching the defined semantics

Parsing the keywords alone is not the milestone.

## Correctness requirements

Any complete Failure implementation must preserve ordinary C++ behavior around the new semantics.

In particular, propagation must not:

- evaluate an operand twice
- reorder observable side effects incorrectly
- bypass destructors
- copy move-only values
- lose reference semantics
- create dangling references
- change temporary lifetime accidentally
- reinterpret ordinary C++ exceptions
- turn programmer errors into recoverable Failure
- leak generated implementation details into normal source diagnostics

These requirements constrain lowering and backend design.

They are part of the research itself.

## C++ compatibility

VixC intends to remain closely connected to ordinary C++.

That creates several compatibility requirements.

Existing C++ identifiers should not become invalid without strong justification.

Native exception syntax must remain valid:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

Preprocessor directives must remain usable.

Templates, RAII, value categories, overloads, libraries, and platform APIs must remain part of the programming model.

The current lexer and parser do not yet satisfy the complete compatibility requirement.

For example, unconditional recognition of `fail` and `fails` as keywords may conflict with valid existing C++ identifiers.

Contextual keyword handling is therefore an active research question.

## No permanent representation decision yet

The research has not selected a permanent C++ representation for Failure.

Candidate strategies include:

```text
std::expected-like representation
generated result type
explicit generated control flow
exceptions
another native representation
```

Each candidate must be evaluated against:

- semantic correctness
- RAII
- move-only values
- references
- ABI
- native interoperability
- generated-code complexity
- runtime cost
- binary size
- native compilation time
- diagnostic quality
- source mapping

The easiest representation to implement first is not automatically the correct long-term representation.

## Real-program evidence

Small tests are necessary but insufficient.

The Failure model should eventually be exercised in programs involving:

- file I/O
- networking
- parsing
- database operations
- multiple failure domains
- several layers of propagation
- RAII resources
- move-only values
- references
- templates
- exceptions
- `noexcept`
- loops
- conditionals
- recursive functions
- multiple translation units
- existing C++ libraries

A language feature should survive these cases before becoming stable.

## What the experiment is not trying to do

The current research is not attempting to:

- eliminate C++ exceptions
- replace `std::expected`
- replace `std::optional`
- force every C++ API into the VixC Failure model
- turn every error into a recoverable value
- define cancellation completely
- define asynchronous execution completely
- redesign the entire C++ type system
- create a universal runtime error hierarchy
- hide the native C++ ecosystem

The experiment is deliberately narrower.

It asks whether recoverable Failure deserves a small amount of direct language semantics.

## Criteria for continuing the design

Evidence would support the current direction if the model provides:

- clear source-level contracts
- explicit and understandable propagation
- stronger semantic diagnostics
- correct interaction with RAII
- correct move and reference behavior
- predictable integration with ordinary C++
- simple native lowering
- useful tooling information
- acceptable runtime cost
- acceptable compile-time cost

The current syntax should be revised if those properties cannot be achieved coherently.

## Criteria for rejecting or redesigning the model

The experiment should also be allowed to fail.

The current design should be reconsidered if evidence shows that:

- the syntax conflicts substantially with real C++
- safe lowering requires reconstructing too much of the C++ language
- templates become impractical
- ABI interoperability becomes excessively difficult
- generated code produces unacceptable compilation cost
- diagnostics are not materially better than library approaches
- runtime performance is consistently worse without compensating benefits
- propagation harms readability in real applications
- the semantic distinction cannot be preserved across realistic C++ features

The purpose of research is to discover these constraints before freezing the language.

## Immediate implementation priorities

The next work should remain focused on completing the first vertical slice rather than expanding the syntax.

The immediate priorities are:

1. represent a complete function declaration and body containing `fails E`
2. establish the Failure contract as a real semantic scope
3. remove sequential or global Failure-contract assumptions from IR construction
4. represent the complete failure-aware computation in semantic IR
5. introduce enough backend-independent control-flow lowering for propagation
6. choose a first experimental C++ representation
7. validate single evaluation, RAII, moves, references, and source provenance
8. compile and execute the generated program through an existing native toolchain

Only after this path works should additional Outcome features be introduced.

## Research status

The research currently establishes a coherent question and an initial semantic hypothesis.

The frontend architecture can represent Failure independently from generated C++.

The first semantic and IR models exist.

The remaining challenge is to connect those pieces through a real declaration-level computation and prove that the resulting lowering works with ordinary C++ semantics.

Until that happens, Failure and Outcome remain research.

They are not yet a stable VixC language contract.
