# Failure and Outcome Lowering

Lowering is the stage where validated VixC semantics are transformed into forms that a backend can implement reliably.

For Failure, lowering sits between semantic IR and the concrete C++ backend.

Its responsibility is not to decide what Failure means.

That meaning has already been established by semantic analysis.

Its responsibility is to make the required control flow and data movement explicit enough that a backend can preserve the semantics without rediscovering them from syntax.

The current Failure IR contains:

```text
Outcome
Failure
FailurePropagation
```

These nodes are semantic.

Lowering must preserve their meaning while preparing them for concrete representation.

## Lowering objective

The first Failure lowering problem can be summarized through three source constructs:

```cpp
T operation(...) fails E
```

```cpp
fail error;
```

```cpp
auto value =
    try other_operation();
```

Semantic analysis establishes that these constructs are valid.

Lowering must then ensure that backend generation has enough information to preserve:

- success and failure as distinct outcomes
- evaluation order
- single evaluation of expressions
- propagation behavior
- object lifetime
- RAII
- value categories
- move-only values
- source provenance
- the failure type associated with the enclosing computation

The backend should not need to infer these properties from source text.

## Semantic IR before lowering

A Failure-aware computation is represented semantically rather than directly as generated C++.

Conceptually:

```text
Outcome
    success(T)
    failure(E)
```

An explicit failure is represented as:

```text
Failure
    operand
```

Propagation is represented as:

```text
FailurePropagation
    operand
```

The actual IR implementation retains source ranges and failure contract information.

At this stage, no decision should have been made about whether the generated C++ uses:

```text
std::expected
generated result type
exceptions
tagged union
status object
```

That decision belongs later.

## Why lowering is necessary

A backend could theoretically translate high-level Failure nodes directly.

For very small examples, that may appear simple.

For example:

```cpp
auto value =
    try load();
```

might tempt a backend to generate a local result object, test it, and return on failure.

However, as soon as the expression appears in more complex source, the backend must reason about:

- evaluation order
- temporary objects
- nested expressions
- scopes
- destruction
- references
- moves
- multiple propagation points
- conditionals
- loops
- return expressions

If every backend independently reconstructs these rules, semantic complexity moves into code generation.

Lowering exists to make that complexity explicit once.

## Lowering must remain backend-independent

Common lowering should describe control flow and semantic operations without choosing a C++ representation.

For example, this source:

```cpp
auto value =
    try load();
```

can be understood conceptually as:

```text
evaluate load()
inspect outcome

if failure:
    propagate failure

if success:
    extract successful value
    bind to value
```

This structure is independent of whether a C++ backend later implements the result using `std::expected`, a generated type, or another mechanism.

Lowering should express this structure.

The backend should implement it.

## `Outcome` lowering

`Outcome` represents the possible completion states of a computation.

For the first Failure slice:

```text
success(T)
failure(E)
```

Lowering should preserve this contract.

It may eventually normalize the representation into a form easier for backends to consume, but it must not erase the distinction between successful and failed completion.

For example, lowering must not replace:

```text
success(T)
failure(E)
```

with a generic integer status unless the semantic relationship between that status and the original outcome remains explicit.

The common lowering layer should not introduce backend storage layout.

## `Failure` lowering

An explicit:

```cpp
fail expression;
```

has several semantic requirements.

Lowering must preserve the equivalent of:

```text
evaluate expression once
produce recoverable failure
leave current success path
perform normal scope cleanup
```

This operation is control-flow terminating for the current successful path.

That property should eventually be represented explicitly in lowered IR.

A backend should not have to inspect a generic expression node and guess that execution does not continue afterward.

## `FailurePropagation` lowering

A propagation expression such as:

```cpp
auto value =
    try operation();
```

requires more structure.

Conceptually, lowering must preserve:

```text
result = evaluate operation()

if result is failure:
    propagate failure from current computation

value = extract successful result
```

Several details are important.

The operand must be evaluated exactly once.

The successful value must preserve its value category and lifetime semantics.

Failure must leave the current successful control-flow path.

Objects already constructed in the current scope must still be destroyed normally.

The generated representation must not require the backend to duplicate the operand.

## Evaluation once

This requirement is fundamental.

Given:

```cpp
auto value =
    try read_next();
```

an incorrect transformation would be:

```cpp
if (!read_next())
{
    // propagate
}

auto value =
    read_next();
```

because `read_next()` is evaluated twice.

That changes program behavior.

The correct lowering model requires one evaluation whose result is then inspected.

Conceptually:

```text
temporary = evaluate read_next()

if temporary failed:
    propagate temporary.failure

value = temporary.success
```

The existence of a temporary is conceptual at the common lowering level.

Its concrete C++ spelling belongs to the backend.

## Synthesized values

Lowering may need internal temporary identities.

For example:

```cpp
auto value =
    try read_next();
```

may require a synthesized semantic object representing the evaluated outcome.

The current `LoweringContext` provides deterministic synthetic identifiers for this purpose.

A synthetic identifier is not:

- a source identifier
- a user-visible variable
- a C++ variable name
- a symbol in the source language

It is internal lowering identity.

A backend may later map it to a concrete generated name.

## Deterministic synthesis

Synthetic lowering state must be deterministic.

Given the same semantic IR and frontend configuration, lowering should produce the same internal structure.

This matters for:

- reproducible generated code
- build caching
- tests
- debugging
- source mapping
- stable diagnostics

Synthetic identifiers should therefore be assigned predictably rather than using pointer values, random values, or global mutable counters.

## Control-flow lowering

Failure propagation is fundamentally a control-flow operation.

The final lowering architecture will likely need representation for control-flow concepts such as:

```text
evaluate
branch
propagate
extract success
continue
```

The current IR does not yet contain a complete control-flow graph.

That is a known limitation.

The existing `FailureLowering` primarily validates Failure IR and preserves the semantic nodes for the backend.

A complete vertical slice will require lowering to become more explicit before final C++ emission.

## Declaration-level lowering

A major requirement is the lowering of a Failure-aware declaration.

Consider:

```cpp
User load_user(int id) fails LoadError
{
    auto record =
        try read_record(id);

    return make_user(record);
}
```

The backend cannot correctly emit the `try` expression without knowing the representation selected for the containing function.

The function's successful result type is:

```text
User
```

and its recoverable failure type is:

```text
LoadError
```

The generated function may therefore need a concrete return representation capable of carrying both states.

This means declaration lowering must happen before individual Failure operations can be emitted completely.

## Current backend boundary

The current C++ backend deliberately rejects standalone `Failure` and `FailurePropagation` nodes when declaration-level lowering has not yet resolved their representation.

This is intentional.

Generating:

```cpp
return error;
```

for `fail`

or inventing an expected-like wrapper inside `emit_failure()` would make a representation decision without enough context.

The missing declaration-level information belongs earlier in the pipeline.

The backend should receive a representation that already makes the surrounding computation model explicit enough to emit correctly.

## Possible lowered declaration form

A future backend-independent lowered computation may conceptually contain information such as:

```text
Computation
    success_type: T
    failure_type: E
    body:
        ...
```

Its operations may then refer to the active outcome contract directly.

This does not require common lowering to decide whether the generated C++ return type is:

```cpp
std::expected<T, E>
```

or another representation.

It only requires the lowered program to state that the computation has those semantic outcomes.

## Possible propagation lowering

A high-level node:

```text
FailurePropagation
    operand
```

may eventually lower conceptually into:

```text
EvaluateOutcome temp, operand

BranchOnFailure temp
    failure:
        PropagateFailure temp.failure

    success:
        ExtractSuccess temp
```

This is not proposed as the final IR naming.

It illustrates the separation between semantic meaning and backend representation.

The important part is that propagation becomes explicit control flow before final text emission.

## Expression context

Propagation can appear inside an expression:

```cpp
auto value =
    try operation();
```

This creates a lowering challenge because propagation can leave the current computation while the successful case must still behave like an expression value.

A statement-only transformation is insufficient if `try` is eventually allowed in arbitrary expression contexts.

For example:

```cpp
consume(
    try load());
```

or:

```cpp
return transform(
    try load());
```

Lowering may need to normalize such expressions into explicit sequencing while preserving original evaluation rules.

## Expression normalization

One possible class of transformation is to separate evaluation from use.

Conceptually:

```cpp
consume(
    try load());
```

may become an internal structure equivalent to:

```text
temp = evaluate load()

if temp failed:
    propagate failure

value = extract success from temp

consume(value)
```

The common lowering layer should model the semantic ordering.

The backend should choose how to spell that structure in C++.

## Sequencing

C++ evaluation order must be respected.

Consider:

```cpp
consume(
    first(),
    try second(),
    third());
```

If this form is accepted by VixC, lowering must preserve the sequencing guarantees of the source language.

It cannot simply move `second()` to an arbitrary earlier statement if that changes observable behavior relative to `first()` or `third()`.

This is one reason arbitrary expression placement should not be declared stable before lowering has been validated against real C++ sequencing rules.

## Temporary lifetimes

Expression normalization can change temporary lifetime if performed incorrectly.

For example:

```cpp
use(
    try make_value());
```

may contain temporaries whose lifetime depends on the original full expression.

Introducing a named temporary can extend or otherwise alter lifetime.

Lowering therefore cannot assume that converting every propagation expression into a local variable is semantically invisible.

The transformation must be validated against normal C++ lifetime rules.

## References

Reference-producing computations require similar care.

For example:

```cpp
Widget &
lookup() fails Error;
```

and:

```cpp
Widget &widget =
    try lookup();
```

must not be lowered into a temporary value that copies the referred object.

The success extraction operation must preserve reference semantics.

This requires semantic type information before final Failure lowering can be considered complete.

## Move-only values

A computation may produce:

```cpp
std::unique_ptr<Resource>
```

or another move-only value.

For example:

```cpp
std::unique_ptr<Resource>
open_resource() fails OpenError;
```

then:

```cpp
auto resource =
    try open_resource();
```

must not introduce a copy.

The lowering model needs to preserve ownership transfer and value category.

Backend implementation should be tested with move-only types before the representation becomes stable.

## Failure values can also be move-only

The same issue applies to failure values.

A failure type may itself contain move-only state.

For example:

```cpp
OperationResult operation() fails Error;
```

where `Error` owns a resource or diagnostic payload.

Then:

```cpp
fail make_error();
```

and propagated failures must not require an unintended copy.

The semantic Failure channel should support ordinary C++ value semantics.

## RAII

Failure lowering must preserve RAII.

Consider:

```cpp
Value operation() fails Error
{
    Resource resource;

    auto value =
        try next();

    return value;
}
```

If `next()` fails, propagation leaves the current successful path.

`resource` must still be destroyed.

This naturally occurs if generated C++ uses normal scope exit.

It can fail if lowering introduces control flow that bypasses destructors or uses unsafe low-level jumps.

Any candidate lowering strategy must therefore be tested against RAII.

## Nested scopes

Propagation can occur inside nested scopes:

```cpp
Value operation() fails Error
{
    Resource outer;

    {
        Resource inner;

        auto value =
            try next();
    }

    return final_value();
}
```

If `next()` fails, both:

```text
inner
outer
```

must be destroyed according to C++ scope rules.

Lowering must not treat propagation as a raw branch to a location outside the function when that would bypass automatic destruction.

## Loops

Failure may occur inside loops:

```cpp
for (auto &item : items)
{
    auto value =
        try process(item);

    consume(value);
}
```

A propagated failure leaves the entire enclosing failure-aware computation, not merely the current loop iteration.

Lowering must distinguish Failure propagation from:

```cpp
break
continue
```

and preserve cleanup for loop-local objects.

## Conditionals

Failure can occur in conditional branches:

```cpp
if (condition)
{
    fail make_error();
}
```

Only the path executing `fail` terminates through failure.

Other paths may continue successfully.

The lowered control-flow representation must preserve that distinction.

## Nested propagation

Propagation operands may themselves contain Failure operations once expression grammar becomes richer.

Conceptually:

```cpp
auto value =
    try transform(
        try load());
```

If such syntax is allowed, lowering must define a deterministic order and ensure each propagation boundary uses the correct enclosing Failure contract.

Nested transformations must not duplicate evaluation or lose source provenance.

## Failure inside ordinary C++ regions

VixC should not hide owned semantics inside `CxxRegion`.

If a source range contains a VixC `fail` or VixC propagation operation, it should eventually have explicit semantic IR.

`CxxRegion` is intended for ordinary C++ that VixC does not need to interpret.

Lowering cannot correctly transform Failure if the parser has already hidden the operation inside opaque source.

This makes parser precision important to lowering correctness.

## Failure type information

The current IR stores failure type source ranges.

That is useful for early experiments but not sufficient for mature lowering.

Eventually, lowering needs semantic type identity rather than only:

```text
source range containing the spelling of the type
```

This is required for:

- aliases
- templates
- dependent types
- conversions
- references
- cv qualification
- generic code
- declaration matching

Source spelling is provenance.

It is not a complete type system.

## Compatibility conversion

If the language eventually permits propagation from one failure type to another, lowering must know whether conversion is required.

For example:

```cpp
Data read() fails ReadError;
```

inside:

```cpp
Value operation() fails OperationError
{
    auto data =
        try read();
}
```

if `ReadError` can become `OperationError`, lowering may need an explicit semantic conversion operation.

That conversion should be represented before the backend.

The backend should not infer conversion policy from C++ overload resolution accidentally unless the language explicitly defines that behavior.

## Failure transformation

A future handling construct may transform failures intentionally.

Conceptually:

```text
ReadError -> OperationError
```

Such a transformation belongs to semantic or lowered IR.

It should not appear only as generated helper code invisible to the frontend.

This is important for diagnostics and tooling.

## `none` lowering

The wider Outcome model includes:

```text
none
```

No complete source semantics exist yet.

When `none` is introduced, lowering must preserve it as distinct from:

```text
failure(E)
```

A backend may choose a combined tagged representation, but common lowering must retain the semantic state identity.

It must not turn all non-success outcomes into one generic error path.

## `stopped` lowering

The same principle applies to:

```text
stopped
```

Future cancellation semantics may require cleanup and propagation behavior different from recoverable Failure.

Failure lowering should not hard-code assumptions that every non-success path behaves identically.

The current architecture keeps the broader Outcome model explicit partly for this reason.

## Programmer errors

Programmer errors are outside the ordinary Outcome model.

Lowering should not automatically translate:

```text
contract violation
invalid state
undefined behavior
```

into:

```text
failure(E)
```

A future contract or safety feature may have its own lowering behavior.

That behavior should remain semantically distinct.

## Exceptions

Ordinary C++ exceptions remain part of the code being processed.

Failure lowering must preserve ordinary exception behavior around VixC operations.

For example:

```cpp
Value operation() fails Error
{
    Resource resource;

    ordinary_cpp();

    auto value =
        try next();

    return value;
}
```

`ordinary_cpp()` may throw.

The generated Failure machinery must not accidentally catch, transform, or suppress that exception unless the language explicitly defines such interaction.

## Exception-based backend strategy

A backend could theoretically represent VixC Failure using exceptions.

If such a strategy is explored, it must satisfy the same semantic lowering requirements.

The strategy would need to preserve:

- explicit Failure contract
- compatible propagation
- distinction from ordinary exceptions
- `noexcept`
- source-level diagnostics
- interaction with existing catches
- ABI behavior
- performance expectations

Using `throw` internally does not eliminate the need for semantic lowering.

It simply changes the backend implementation.

## Expected-like backend strategy

An expected-like representation is another possible backend strategy.

Conceptually:

```cpp
std::expected<T, E>
```

already represents:

```text
success(T)
failure(E)
```

Propagation could then be implemented with explicit branch and extraction.

This representation aligns well with value-based semantics but still leaves important lowering problems:

- expression placement
- value categories
- temporary lifetime
- error conversion
- generated function signatures
- interoperability
- source maps

Using `std::expected` does not remove the need for declaration and control-flow lowering.

## Generated result type

VixC could also generate its own result representation.

That may provide more control over:

- layout
- ABI
- compile-time dependencies
- diagnostics
- future Outcome states

It also creates implementation complexity and risks duplicating existing C++ facilities.

A generated type should only be chosen if evidence shows that existing representations cannot satisfy the semantic and engineering requirements adequately.

## Lowering and ABI

The representation selected for Failure can affect ABI.

Changing:

```cpp
T operation();
```

into a generated result type changes the concrete C++ function signature.

This matters for:

- external linkage
- existing libraries
- virtual functions
- callbacks
- function pointers
- binary compatibility
- exported APIs

Common lowering should preserve enough information for the backend to make ABI-aware decisions.

The language model must eventually define where Failure-aware declarations are allowed across native boundaries.

## Separate compilation

Failure-aware declarations may appear in headers while their definitions appear in other translation units.

Lowering must eventually support consistent representation across those units.

This requires stable knowledge of:

- declaration identity
- Failure contract
- generated signature
- ABI representation

A source-only local transformation is insufficient for a mature implementation.

The first vertical slice may remain single-source while this broader requirement is researched.

## Templates and lowering

Templates create another boundary.

A Failure-aware template may not have complete type information until instantiation.

For example:

```cpp
template <typename T>
T load() fails Error;
```

or:

```cpp
template <typename T>
T load() fails typename T::error_type;
```

Lowering must eventually determine which parts can happen before instantiation and which require instantiated semantic types.

String-based source rewriting is not sufficient for this model.

## Coroutines

Failure lowering must eventually consider interaction with coroutines.

A coroutine may suspend between creation and completion.

A recoverable failure may belong to the eventual completion rather than the initial function call.

This could affect where Failure outcome storage lives.

No coroutine-specific Failure lowering is currently defined.

The first implementation should avoid choices that prevent later integration.

## Async and cancellation

Future asynchronous semantics may introduce:

```text
success
failure
stopped
```

within one computation.

The lowering architecture should therefore remain capable of representing multiple completion channels.

Failure should not consume all non-success behavior.

This is one reason a generic boolean success flag may be too weak as a long-term semantic lowering model.

## Lowering diagnostics

Lowering should normally receive valid semantic IR.

Its diagnostics therefore focus on invariant failures and unsupported lowering states.

Examples include:

```text
invalid Outcome IR
Failure node missing its operand
FailurePropagation missing its operand
failure type provenance cannot be recovered
unsupported semantic IR
```

These diagnostics are useful during frontend development.

Long term, normal programmer mistakes should be diagnosed earlier.

A valid source program should not fail lowering because semantic information that could have been established earlier is missing.

## Current `FailureLowering`

The current implementation performs the first structural layer of lowering.

It validates:

- Outcome IR
- Failure IR
- FailurePropagation IR
- required failure type source ranges
- required operands
- nested Failure operations

Ordinary `CxxRegion` operands are accepted without interpretation.

Nested Failure constructs are recursively processed.

The implementation deliberately does not yet erase Failure IR into C++-specific operations.

That keeps the backend-independent boundary intact while declaration-level and control-flow lowering are still incomplete.

## Current limitation

The current lowering pipeline does not yet transform Failure propagation into an explicit backend-ready control-flow representation.

As a result, final C++ emission currently rejects:

```text
Failure
FailurePropagation
```

when they reach the backend unresolved.

This is preferable to silently inventing incorrect semantics.

The next implementation work should solve the missing semantic and lowering structure before enabling final emission.

## Required next step

Before Failure can be emitted end-to-end, VixC needs a representation of a complete failure-aware computation.

The parser and semantic layers must first establish:

```text
declaration
success type
failure type
body
```

IR construction must preserve that relationship.

Lowering can then transform the body while knowing the active computation outcome.

Only after that context exists can Failure and propagation be lowered correctly.

## Target lowering example

Given:

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

a backend-independent lowered form should conceptually preserve:

```text
computation:
    success type: User
    failure type: LoadError

body:
    evaluate read_record(id)

    if failure:
        propagate failure

    bind successful record

    evaluate record.valid()

    if false:
        construct LoadError
        produce failure

    evaluate make_user(record)
    produce success
```

This structure contains the essential semantics.

It still does not say whether the C++ backend uses `std::expected`, a generated result object, exceptions, or another representation.

That is the correct lowering boundary.

## Lowering tests

Failure lowering should eventually be tested with cases including:

- success-only computation
- explicit failure
- simple propagation
- nested propagation
- propagation inside conditionals
- propagation inside loops
- multiple propagation points
- side-effecting operands
- move-only success values
- move-only failure values
- reference results
- temporary-producing operands
- RAII objects before propagation
- RAII objects inside nested scopes
- native exceptions around propagation
- `noexcept`
- templates
- overloaded operations
- failure type conversions

The current tests cover structural Failure IR and source recovery.

More semantic control-flow tests are required before the lowering strategy can be considered stable.

## Performance questions

Lowering strategy affects runtime performance.

Experiments should measure:

- copies
- moves
- branch count
- result object size
- generated code size
- inlining behavior
- optimization quality
- exception-table overhead if applicable

The frontend should not assume that a semantically clean representation is automatically zero-cost.

Performance should be measured on generated native code.

## Compile-time questions

Backend representation also affects compile time.

A heavily templated generated representation may produce correct code but significantly increase native compilation cost.

Experiments should compare:

- standard expected-like types
- generated lightweight result types
- helper templates
- explicit generated control flow
- other candidate representations

Compilation performance is relevant because generated C++ still passes through the normal C++ toolchain.

## Generated-code readability

Generated C++ is not the primary source language, but readability still matters during development.

Readable generated output helps with:

- debugging the frontend
- validating lowering
- comparing backends
- investigating compiler diagnostics
- reporting compiler bugs

Lowering should therefore prefer deterministic and inspectable structures when they do not compromise semantics or performance.

## Source mapping

Every lowering transformation should preserve original source provenance.

When one VixC source construct generates multiple C++ regions, those regions can map back to the same original `SourceRange`.

For example, one:

```cpp
try operation()
```

may generate:

```text
temporary declaration
outcome check
failure propagation
success extraction
```

Each relevant generated region can retain provenance pointing to the original propagation expression.

This allows later diagnostics to remain connected to source semantics.

## Semantic provenance

Byte-level source mapping may eventually be insufficient.

Generated helper code may originate from a semantic construct without corresponding directly to one source substring.

Future lowering may therefore need provenance such as:

```text
generated node X originated from FailurePropagation Y
```

in addition to simple generated byte ranges.

The current implementation begins with `SourceRange` provenance.

The architecture should remain open to richer semantic provenance later.

## Lowering invariants

The current working invariants are:

1. semantic analysis has already validated source-level Failure legality
2. lowering does not redefine Failure semantics
3. Failure operands are evaluated once
4. propagation leaves the current successful path on failure
5. successful propagation exposes the successful value
6. C++ object lifetime and RAII must be preserved
7. value category and move semantics must be preserved
8. failure type compatibility is semantic, not textual
9. backend-specific representation remains outside common lowering
10. source provenance survives transformation

Any candidate lowering strategy should be evaluated against these invariants.

## First lowering milestone

The first complete lowering milestone is reached when this source:

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

can be transformed into backend-ready IR without relying on textual rewriting.

At that point, the lowered program must make explicit:

- the computation's success type
- its failure type
- explicit failure production
- propagation control flow
- successful value extraction
- normal body continuation
- source provenance

The C++ backend can then choose a concrete native representation while preserving those semantics.

That is the role of Failure lowering in VixC.
