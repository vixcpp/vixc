# Failure and Outcome Semantics

This document defines the current semantic model being investigated for recoverable Failure in VixC.

It describes what Failure constructs mean independently of their C++ representation.

The purpose is to establish semantic rules before choosing how those rules are implemented by the C++ backend.

The current language surface under investigation is:

```cpp
T operation(...) fails E
```

```cpp
fail error;
```

```cpp
auto value = try operation();
```

These constructs describe recoverable failure as part of a computation's contract.

They are not defined as aliases for exceptions, `std::expected`, error codes, macros, or another existing C++ mechanism.

Those mechanisms may participate in backend implementation or interoperability, but they do not define the semantics described here.

## Semantic objective

The Failure model should allow the frontend to answer questions that are difficult to answer reliably when failure exists only as a library convention.

For a given computation, VixC should eventually be able to determine:

- whether recoverable failure is part of its contract
- which failure type belongs to that contract
- whether a `fail` operation is legal at a source location
- whether a `try` operation is legal at a source location
- what control flow occurs after explicit failure
- what control flow occurs after propagated failure
- whether the propagated failure is compatible with the enclosing contract
- which source construct caused a failure-related semantic error

These rules must be established before backend generation.

## Computation outcome

A computation has a set of possible semantic outcomes.

The current model distinguishes:

```text
success
none
failure
stopped
```

These states are intentionally different.

They should not be collapsed simply because one concrete backend representation could store several of them in the same object.

The first implemented Failure slice currently focuses on:

```text
success
failure
```

The presence of `none` and `stopped` in the wider Outcome model does not mean their complete language semantics are already defined.

## Success

`success(T)` represents ordinary successful completion with a value of type `T`.

For a normal function:

```cpp
User load_user();
```

the primary computation outcome is conceptually:

```text
success(User)
```

VixC does not require special syntax for ordinary success.

Normal C++ expressions and `return` behavior continue to provide the successful value unless a future semantic feature requires additional rules.

Success is the default outcome of a computation.

## Recoverable failure

`failure(E)` represents completion through a recoverable failure value of type `E`.

A failure-aware computation declares that possibility explicitly:

```cpp
User load_user(int id) fails LoadError
{
    // ...
}
```

Its conceptual outcome set is:

```text
success(User)
failure(LoadError)
```

This does not require the source-level function type to be written as a wrapper such as:

```cpp
Result<User, LoadError>
```

or:

```cpp
std::expected<User, LoadError>
```

The concrete representation is deferred to lowering and backend generation.

The semantic contract remains explicit regardless of representation.

## Failure contract

A declaration of the form:

```cpp
T operation(...) fails E
```

introduces a Failure contract for the computation represented by that declaration.

The contract contains at least:

```text
success type: T
failure type: E
```

The successful result remains the normal result of the computation.

The `fails E` specification adds recoverable failure to the set of legal outcomes.

Conceptually:

```text
operation : () -> success(T) | failure(E)
```

This notation describes semantic outcomes only.

It is not intended to define ABI representation or generated C++ function type syntax.

## Contract scope

A Failure contract applies to the body of the declaration that introduces it.

For example:

```cpp
User load_user() fails LoadError
{
    fail LoadError{};
}
```

the `fail` statement is valid because it appears inside the scope of the `LoadError` Failure contract.

The following is not semantically equivalent:

```cpp
User load_user() fails LoadError
{
}

void other()
{
    fail LoadError{};
}
```

The second function does not inherit the contract merely because a `fails LoadError` declaration appeared earlier in the source file.

Failure contracts are lexical and semantic properties of computations, not global parser state.

## Nested scopes

The semantic model must support scoped Failure contexts.

Conceptually, semantic analysis maintains an active Failure contract while analyzing a failure-aware computation.

When entering:

```cpp
T operation(...) fails E
```

the analyzer establishes a context containing at least the declared failure type.

When leaving the computation, that context is removed.

This model allows nested semantic constructs to eventually establish their own contracts without corrupting an enclosing contract.

The current `SemanticContext` represents this idea using a Failure context stack.

The complete parser-to-semantic declaration integration is still being implemented.

## `fail`

The statement:

```cpp
fail expression;
```

explicitly produces the recoverable failure outcome of the current computation.

If the active contract is:

```cpp
T operation(...) fails E
```

then:

```cpp
fail expression;
```

conceptually performs:

```text
evaluate expression
produce failure(E)
terminate the current successful execution path
```

The expression must eventually be semantically compatible with `E`.

The first frontend implementation validates the existence of the active Failure contract and operand structure, but complete C++ type compatibility is not yet implemented.

## `fail` requires a Failure context

The following source is invalid:

```cpp
void operation()
{
    fail Error{};
}
```

There is no active recoverable Failure contract.

The frontend should diagnose this directly.

The source must not be translated into some arbitrary backend failure representation and left for the native C++ compiler to reject.

The semantic error belongs to VixC because VixC owns the meaning of `fail`.

## `fail` does not return normally

After:

```cpp
fail error;
```

the current successful execution path does not continue.

For example:

```cpp
Value operation() fails Error
{
    fail Error{};

    use_value();
}
```

`use_value()` is not reached through the successful path following that `fail`.

The exact lowering required to represent this control-flow property has not yet been finalized.

The semantic rule itself is independent of the backend.

## Failure operand evaluation

The operand of `fail` is an expression.

For example:

```cpp
fail make_error(code);
```

The expression must be evaluated according to normal C++ expression semantics.

A backend implementation must not accidentally change observable behavior by:

- evaluating the operand more than once
- evaluating it after control flow has already changed
- reordering required side effects
- changing move behavior
- introducing an unexpected copy
- extending or shortening temporary lifetimes incorrectly

The Failure operation adds outcome semantics around the expression.

It does not redefine ordinary C++ evaluation rules inside the expression.

## `try`

The expression:

```cpp
try operation()
```

represents conditional propagation of the operand computation's recoverable failure.

Conceptually:

```text
evaluate operation()

if outcome is success(value):
    produce value

if outcome is failure(error):
    propagate error from the enclosing computation
```

The successful value becomes the value of the `try` expression.

A recoverable failure does not become an ordinary value at that source location.

It leaves the current successful execution path through the enclosing Failure contract.

## Example of propagation

Given:

```cpp
Record read_record(int id) fails LoadError;
```

and:

```cpp
User load_user(int id) fails LoadError
{
    auto record =
        try read_record(id);

    return make_user(record);
}
```

the successful case behaves conceptually like:

```text
read_record(id)
    -> success(record)

try
    -> record

make_user(record)
    -> success(user)
```

The failure case behaves conceptually like:

```text
read_record(id)
    -> failure(error)

try
    -> propagate failure(error)

load_user(id)
    -> failure(error)
```

The statement following the propagation point is not executed on that failure path.

## `try` requires a Failure context

A propagation operation needs an enclosing computation capable of receiving the propagated failure.

Therefore:

```cpp
void operation()
{
    auto value =
        try load();
}
```

is invalid if `operation` does not have an appropriate Failure contract.

The frontend should report this as a semantic error.

A propagation construct cannot silently invent a Failure contract for its enclosing function.

## Propagation compatibility

Propagation is only valid when the failure produced by the operand is compatible with the Failure contract of the enclosing computation.

Consider:

```cpp
Data read_data() fails ReadError;
```

and:

```cpp
Result process() fails ProcessError
{
    auto data =
        try read_data();
}
```

The frontend must eventually determine whether:

```text
ReadError
```

can be propagated as:

```text
ProcessError
```

The exact compatibility rule is not yet finalized.

Possible models include:

- exact type identity
- implicit conversion
- explicit declared conversion
- failure-set composition
- another language-defined relation

The first implementation must not pretend this question is solved by comparing source strings.

Compatibility belongs to semantic type analysis.

## Propagation must evaluate once

The operand of `try` must be evaluated exactly according to ordinary expression semantics.

For:

```cpp
auto value =
    try operation();
```

a backend must not lower the construct into code that evaluates:

```cpp
operation()
```

multiple times.

This matters when the operand:

- performs I/O
- mutates state
- consumes a move-only value
- increments a counter
- allocates a resource
- acquires a lock
- has any other observable side effect

The outcome inspection must apply to the result of one evaluation.

## Value category preservation

Failure propagation must eventually define how successful values preserve normal C++ value semantics.

This includes cases involving:

```cpp
T
T&
const T&
T&&
```

and move-only types.

A naive lowering can accidentally copy a value that should be moved, return a reference to temporary storage, or change overload selection.

The semantic model therefore requires successful propagation to preserve the behavior expected from the operand's successful value.

The exact implementation rules remain part of future type and lowering work.

## Lifetime preservation

VixC Failure semantics must preserve C++ object lifetime and RAII behavior.

Consider:

```cpp
Value operation() fails Error
{
    Resource resource;

    auto value =
        try second_operation();

    return value;
}
```

If `second_operation()` fails, propagation leaves the current success path.

`resource` must still be destroyed according to normal C++ lifetime rules.

A generated implementation cannot bypass required destructors merely because failure propagation is represented through generated control flow.

This requirement applies equally to explicit `fail`.

## Failure is not an exception

The semantic operation:

```cpp
fail error;
```

is not defined as:

```cpp
throw error;
```

The semantic operation:

```cpp
try operation()
```

is not defined as a `try`/`catch` expression.

A C++ backend could theoretically use exception machinery if that representation were proven to preserve the VixC contract, but the source semantics do not assume it.

Ordinary C++ exceptions remain a separate mechanism.

## Failure is not `return`

The semantic operation:

```cpp
fail error;
```

is also not defined as ordinary C++:

```cpp
return error;
```

The successful return type and the failure type are conceptually distinct parts of the computation contract.

A backend may eventually generate a wrapper return representation and use `return` internally.

That does not make VixC Failure semantically identical to C++ return.

## Failure is not `std::expected`

A function such as:

```cpp
User load_user() fails LoadError;
```

could potentially be represented by a backend using an expected-like C++ type.

The semantic contract remains:

```text
success(User)
failure(LoadError)
```

rather than:

```text
this function is semantically defined as std::expected<User, LoadError>
```

This distinction preserves backend freedom and prevents a library representation from defining the language.

## Failure is not absence

A recoverable failure means that the computation could not produce its normal result because a declared recoverable problem occurred.

Absence is conceptually different.

For example:

```text
find user
```

may complete normally and determine that no matching user exists.

That can be represented conceptually by:

```text
none
```

A failed database connection is different:

```text
failure(DatabaseError)
```

The current Failure slice does not yet introduce source syntax for `none`, but the distinction is preserved in the wider Outcome model.

## Failure is not cancellation

A computation may eventually be stopped because its result is no longer required.

That condition is conceptually different from:

```text
failure(E)
```

The broader Outcome model therefore reserves:

```text
stopped
```

for future work involving cancellation and interrupted computation.

The first Failure model does not define `stopped` semantics completely.

Failure implementation should avoid assumptions that would make the distinction impossible later.

## Failure is not programmer error

The Outcome model does not treat programmer errors as ordinary recoverable failure.

For example, these conditions are not automatically:

```text
failure(E)
```

simply because they represent something going wrong:

- invalid memory access
- violated internal invariant
- impossible state reached
- broken precondition
- corrupted internal structure
- undefined behavior

A valid program may declare that network access can fail.

That does not mean a null pointer dereference should become a normal network failure value.

The distinction between recoverable failure and program invalidity is fundamental.

## Contract violation

A contract violation represents a condition that valid execution was required not to produce.

Its handling may eventually involve:

- diagnostics
- assertions
- termination
- dedicated contract machinery
- development instrumentation

It should not automatically flow through `fails E`.

The Failure channel represents recoverable conditions declared by the computation.

## Ordinary exceptions remain possible

The current Failure model does not state that failure-aware functions are forbidden from using ordinary C++ exceptions.

For example:

```cpp
Value operation() fails Error
{
    ordinary_cpp();
}
```

`ordinary_cpp()` may use existing C++ exception behavior.

The exact interaction between uncaught C++ exceptions and VixC Failure contracts is not yet fully specified.

This remains an open semantic question.

The model should not silently reinterpret every exception as `failure(E)`.

## `noexcept`

The interaction between Failure and C++ `noexcept` requires explicit definition.

For example:

```cpp
Value operation() noexcept fails Error;
```

raises several questions.

If Failure is implemented through ordinary value-based control flow, the declaration may be compatible with `noexcept`.

If a backend were to use exceptions internally for Failure, it would have to preserve the externally visible `noexcept` semantics.

The language model should therefore not make backend choices that accidentally constrain the meaning of `noexcept`.

No final rule is specified yet beyond preserving normal C++ exception guarantees.

## Constructors

The current Failure model does not yet define whether constructors may use:

```cpp
fails E
```

For example:

```cpp
Widget() fails InitError;
```

would raise questions about object initialization, partial construction, destructor behavior, and generated C++ representation.

This remains an open question.

The first Failure slice should focus on ordinary functions before expanding the contract to constructors.

## Destructors

Recoverable failure from destructors is even more constrained by existing C++ semantics.

The current model does not define constructs such as:

```cpp
~Widget() fails Error;
```

This is not part of the first Failure experiment.

Destructor semantics should not be added until interaction with stack unwinding, cleanup, termination, and ordinary C++ destruction rules is understood.

## Failure contracts and function identity

The model has not yet committed to whether:

```cpp
fails E
```

is part of function type identity.

That question affects:

- overload resolution
- function pointers
- templates
- virtual functions
- ABI
- separate compilation
- declaration matching

For the current frontend experiment, the contract is semantic information associated with a computation.

A durable language specification will need a stronger answer before Failure is considered complete.

## Declaration consistency

Multiple declarations of the same function must eventually agree on their Failure contract according to language-defined rules.

For example:

```cpp
User load_user(int id) fails LoadError;
```

followed by:

```cpp
User load_user(int id)
{
    // ...
}
```

raises a declaration consistency question.

Likewise:

```cpp
User load_user(int id) fails OtherError;
```

cannot simply be accepted without determining whether the declarations are compatible.

The current frontend does not yet implement cross-declaration semantic resolution.

This belongs to future declaration analysis.

## Templates

Failure contracts must eventually compose with templates.

For example:

```cpp
template <typename T>
T load() fails LoadError;
```

or a failure type dependent on template arguments.

The model should not rely on string-based failure type comparison because that would fail for dependent types, aliases, substitutions, and other ordinary C++ type behavior.

Failure compatibility must ultimately operate on semantic type information.

## Generic code

Generic code raises an additional question.

A template may call an operation whose Failure contract depends on the instantiated type.

The frontend may need to reason about whether the enclosing computation's outcome contract also becomes dependent.

This is not part of the first vertical slice, but the first implementation should avoid representation decisions that make generic Failure semantics impossible later.

## Overload resolution

The current model does not define whether Failure contracts participate directly in overload resolution.

For example:

```cpp
Value load(int) fails ErrorA;
Value load(long) fails ErrorB;
```

ordinary C++ parameter types already distinguish these overloads.

More difficult questions appear if two declarations differ only by Failure contract.

That behavior is not yet specified.

VixC should not invent an answer in backend generation.

## Control-flow analysis

Because `fail` and propagated `try` can terminate the current successful execution path, they eventually need to participate in control-flow analysis.

This may affect diagnostics involving:

- unreachable statements
- missing successful returns
- initialization
- definite assignment
- cleanup
- future ownership semantics

The first implementation does not yet provide a complete control-flow graph.

The semantic model nevertheless requires Failure operations to be represented as control-flow relevant rather than as ordinary expressions with ignored side effects.

## `try` inside expressions

The proposed propagation syntax is intended to behave as an expression when the operand succeeds.

For example:

```cpp
auto value =
    try load();
```

A future implementation may also need to consider forms such as:

```cpp
consume(
    try load());
```

or:

```cpp
return transform(
    try load());
```

The semantics should remain:

```text
success -> expression value
failure -> propagate from enclosing computation
```

However, expression composition raises requirements around evaluation order, temporary lifetimes, sequencing, and backend lowering.

The first implementation should validate these carefully before promising unrestricted placement.

## Native C++ `try`

C++ already defines:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

The VixC propagation spelling uses the same token:

```cpp
try operation()
```

The semantic models are different.

The frontend must preserve native C++ exception syntax.

The current parser treats `try` followed by `{` as ordinary C++.

That rule is only an initial compatibility mechanism.

A complete design must verify that the new expression form does not create unacceptable ambiguity with existing C++ constructs.

## Evaluation order

VixC does not introduce a new global evaluation-order model.

Ordinary C++ sequencing rules remain applicable to the C++ expressions surrounding Failure operations.

A backend must preserve those rules.

Consider:

```cpp
consume(
    first(),
    try second(),
    third());
```

If such placement becomes legal, the generated representation must preserve the evaluation guarantees of the accepted source language model.

This can become difficult if propagation requires introducing statements and temporaries.

Expression lowering must therefore be based on semantic correctness rather than textual convenience.

## Side effects

Propagation must not duplicate or discard observable side effects.

For example:

```cpp
auto value =
    try read_and_increment(counter);
```

the function call must not occur twice because the backend separately checks failure and extracts success.

Likewise, side effects that occur before a propagated failure must remain observable when required by C++ semantics.

The Failure operation changes completion flow.

It does not grant the backend permission to reorder arbitrary user code.

## Move-only values

A successful computation may produce a move-only type:

```cpp
std::unique_ptr<Resource>
open_resource() fails OpenError;
```

Then:

```cpp
auto resource =
    try open_resource();
```

must be capable of transferring the successful value without requiring an illegal copy.

The same requirement applies to arbitrary user-defined move-only types.

The backend representation and lowering strategy must preserve this property.

## References

A successful computation may involve reference types or expressions producing references.

Failure propagation must avoid materializing unintended copies or references to temporary storage.

The exact semantic rules for reference-returning failure-aware functions remain to be specified, but ordinary C++ reference behavior must not be silently changed.

## Destruction on propagation

Consider:

```cpp
Result operation() fails Error
{
    Resource first;
    Resource second;

    auto value =
        try next();

    return make_result(value);
}
```

When `next()` produces failure, both `first` and `second` must be destroyed according to normal C++ scope-exit rules.

The backend must therefore implement propagation using control flow that respects destruction semantics.

This requirement strongly constrains unsafe forms of source rewriting.

## Failure value lifetime

The failure operand may itself involve temporaries:

```cpp
fail make_error();
```

The generated representation must preserve the lifetime necessary to transfer the failure value into the enclosing computation's failure outcome.

The backend cannot retain references to temporary failure objects beyond their valid lifetime unless the source semantics themselves require such behavior.

## Source-level explicitness

Failure production is explicit:

```cpp
fail error;
```

Propagation is also explicit:

```cpp
try operation();
```

The current hypothesis intentionally makes propagation visible at the source location where it may leave the successful execution path.

This differs from ordinary C++ exception propagation, which does not require every propagating call site to contain a marker.

Whether this explicitness remains desirable in large real programs is part of the research.

## Failure handling

The current Failure slice defines production and propagation before defining dedicated handling syntax.

A caller may eventually need to inspect or transform a failure rather than propagate it.

The syntax and semantics for that operation are not yet fixed.

Potential handling mechanisms should be evaluated alongside future work on choice and pattern matching rather than introduced prematurely as another isolated construct.

The first vertical slice can still demonstrate Failure semantics through declaration, production, and propagation.

## Conversion between failure types

Automatic conversion between different failure types is not yet specified.

For example:

```cpp
NetworkData fetch() fails NetworkError;
```

inside:

```cpp
Data load() fails LoadError
{
    auto data =
        try fetch();
}
```

could theoretically require a mapping:

```text
NetworkError -> LoadError
```

That mapping might be:

- implicit through C++ conversion
- explicitly declared
- performed by handling syntax
- prohibited without matching failure types

No final decision has been made.

The implementation should preserve enough semantic information to support a deliberate rule later.

## Multiple failure types

The first syntax uses one declared type:

```cpp
fails E
```

That type could itself represent multiple domain-specific alternatives through ordinary C++ types.

The language has not yet committed to syntax such as:

```cpp
fails E1, E2
```

or automatic type unions.

Before adding multiple language-level Failure alternatives, VixC should test whether a single semantic failure type combined with ordinary C++ type composition is sufficient.

## Failure transformation

A computation may need to transform one failure domain into another.

Conceptually:

```text
NetworkError
    -> LoadError
```

Automatic propagation cannot always perform that transformation correctly.

The programmer may need an explicit handling boundary.

The semantics for such transformation remain open.

This is related to future choice and pattern-matching research.

## Outcome composition

The wider Outcome model anticipates computations that may eventually contain more than:

```text
success
failure
```

For example:

```text
success(T)
none
failure(E)
```

or:

```text
success(T)
failure(E)
stopped
```

The language must eventually define how operations compose these states.

The first Failure implementation should therefore avoid assuming that every non-success state is represented by one generic error branch.

## Outcome invariants

The current semantic hypothesis has several invariants.

Every ordinary computation permits successful completion unless a future construct explicitly defines otherwise.

A Failure contract adds recoverable failure.

Failure has a declared type.

Failure production requires an active Failure contract.

Failure propagation requires an active enclosing Failure contract.

Programmer errors are not ordinary Outcome states.

Backend representation must not alter these properties.

## Semantic representation in the current frontend

The semantic layer currently uses `OutcomeModel` to represent the possible outcomes of a computation.

The first model supports success-only computation and failure-aware computation.

A failure-aware model records:

```text
failure specification range
failure type range
```

The source ranges retain provenance while the frontend still lacks a complete semantic C++ type representation.

The model distinguishes:

```text
Success
None
Failure
Stopped
```

at the semantic level.

Only the appropriate states are enabled by the current construction path.

## IR representation

After semantic analysis, Failure semantics are represented in IR.

The current IR contains:

```text
Outcome
Failure
FailurePropagation
```

`Outcome` represents the computation's completion contract.

`Failure` represents explicit recoverable failure production.

`FailurePropagation` represents propagation of a recoverable failure from another computation.

The IR retains source ranges and failure type provenance.

It does not encode generated C++ source as its semantic representation.

## Relationship between semantic Outcome and IR Outcome

The semantic layer determines whether a source construct is meaningful.

The IR represents the validated semantic operation for later transformation.

These are related but distinct responsibilities.

The semantic Outcome model should not become backend code.

The IR Outcome should not repeat source-level validation that belongs to semantic analysis.

As the implementation matures, the mapping between these representations should remain explicit.

## Lowering semantics

Common lowering receives semantic IR that should already be valid.

Its role is to prepare those operations for backend processing.

For Failure, lowering may eventually transform a high-level propagation operation into more explicit control-flow operations.

Conceptually:

```text
evaluate operand
inspect outcome
branch on failure
extract success value
```

This remains backend-independent as long as it does not choose a concrete C++ storage representation.

The current lowering implementation primarily validates Failure IR structure.

More complete control-flow lowering is still required.

## Backend requirements

A backend implementing Failure must preserve the semantic contract.

For:

```cpp
T operation() fails E
```

the backend must represent at least:

```text
success(T)
failure(E)
```

For:

```cpp
fail error;
```

the backend must produce the failure outcome and leave the successful execution path.

For:

```cpp
auto value =
    try operation();
```

the backend must evaluate once, propagate failure, and expose the successful value.

The backend must also preserve normal C++ object lifetimes and observable evaluation behavior.

## Backend freedom

The semantics intentionally leave the backend freedom to choose representation.

Possible strategies include:

- expected-like value representation
- generated tagged result type
- specialized return storage
- exceptions
- another native mechanism

No strategy is currently declared to be the permanent implementation.

The backend may evolve as long as the source-level semantics remain unchanged.

## Runtime requirements

The Failure semantic model does not currently require a dedicated runtime.

If correct Failure behavior can be represented through generated native C++, the frontend should prefer that simpler dependency model.

A runtime should only become part of Failure semantics if future evidence demonstrates that native lowering alone cannot provide the required behavior coherently.

## Diagnostics

Semantic violations should be reported at the semantic level.

Examples include:

```text
fail outside a Failure contract
try outside a Failure contract
missing Failure operand
missing propagation operand
invalid Failure specification
incompatible propagated failure type
```

The last item requires type information that the current frontend does not yet possess.

Diagnostics should refer to original source ranges.

Generated C++ implementation details should not be exposed when VixC already understands the source-level error.

## Declaration-level semantic requirement

The current implementation has an important missing piece.

The parser recognizes:

```cpp
fails Error
```

as a `FailureSpecification`.

It also recognizes:

```cpp
fail error;
```

and:

```cpp
try operation()
```

as separate syntax nodes.

However, the parser does not yet represent the complete function declaration and body as a syntax structure that owns the Failure specification.

As a result, the semantic analyzer cannot naturally perform:

```text
enter Failure context
analyze body
leave Failure context
```

for real source code.

This is the next semantic integration requirement.

## Required declaration behavior

For:

```cpp
Value operation() fails Error
{
    auto value =
        try next();

    if (!value.valid())
        fail Error{};

    return value;
}
```

semantic analysis should behave conceptually as follows.

First, analyze the declaration and establish:

```text
success type: Value
failure type: Error
```

Then enter the function body with:

```text
active Failure context: Error
```

While that context is active:

```cpp
try next()
```

can be validated against the enclosing contract.

Likewise:

```cpp
fail Error{};
```

can be validated against the same contract.

When the body ends, the Failure context is removed.

A later unrelated function must not inherit it.

## First semantic milestone

The first Failure semantics milestone is reached when the frontend can correctly analyze:

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

as one coherent computation.

Success requires more than parsing the three new keywords.

The frontend must establish one declaration-level Failure contract and apply it consistently to the body.

The same semantic information must then survive into IR and lowering.

## Semantic tests required

Before the Failure model becomes stable, tests should cover at least:

- success-only computation
- valid Failure declaration
- `fail` inside a matching Failure contract
- `fail` outside a Failure contract
- `try` inside a matching Failure contract
- `try` outside a Failure contract
- nested Failure contexts
- independent neighboring functions
- failure type compatibility
- propagation from incompatible failure type
- move-only successful values
- move-only failure values
- reference results
- temporary lifetimes
- destructor execution during propagation
- side effects in propagation operands
- native C++ `try` blocks
- ordinary exceptions inside failure-aware functions
- template instantiation
- multiple declarations of the same function

Not all of these are implemented yet.

They represent semantic requirements that should be resolved before claiming a durable Failure model.

## Semantic stability criteria

The Failure model should not be considered stable until several conditions are satisfied.

A programmer must be able to understand Failure behavior without knowing generated C++ details.

The frontend must diagnose invalid Failure use directly.

Propagation must preserve ordinary C++ evaluation and lifetime semantics.

The model must work with non-trivial real C++ types.

Failure contracts must be scoped to the computation that declares them.

Failure type compatibility must be based on semantic type information.

Generated representation must remain replaceable.

The model must remain compatible with ordinary C++ code around it.

The design must leave room for future `none`, `stopped`, async, cancellation, ownership, and choice semantics without forcing all of them into a generic error channel.

## Current semantic contract

The current working contract can be summarized as follows.

Given:

```cpp
T f() fails E
```

the computation may complete with:

```text
success(T)
failure(E)
```

Given:

```cpp
fail e;
```

inside that computation:

```text
evaluate e once
require e to be compatible with E
produce failure(E)
leave the current success path
```

Given:

```cpp
try g()
```

inside that computation:

```text
evaluate g() once

if g() -> success(v):
    expression result is v

if g() -> failure(e):
    require e to be compatible with E
    produce failure(E) from the enclosing computation
    leave the current success path
```

Programmer errors remain outside this ordinary Outcome model.

`none` and `stopped` remain distinct semantic categories whose complete language rules are future work.

This is the semantic foundation the current VixC Failure experiment is intended to test.
