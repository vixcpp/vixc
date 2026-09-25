# Failure and Outcome: Problem

Recoverable failure in C++ is not one problem with one established language model.

It is represented through several mechanisms with different semantics, different control-flow behavior, different type information, different diagnostics, and different expectations about how callers should respond.

A C++ program may express unsuccessful computation through exceptions, error codes, sentinel values, nullable objects, status objects, `std::optional`, `std::expected`, custom result types, callbacks, or application-specific conventions.

Each mechanism can be appropriate in a particular context.

The research problem for VixC is not to prove that one of them is universally wrong.

The problem is that C++ does not give the programmer one explicit language-level distinction between the fundamentally different ways in which a computation can fail to produce its normal value.

VixC is investigating whether those distinctions can be made explicit enough that the frontend can reason about them directly.

## The underlying question

The central research question is:

> What are the distinct ways in which a computation can fail to produce its normal successful result, and which of those states should be represented explicitly by the language?

This question comes before syntax.

It also comes before selecting a C++ implementation such as `std::expected`, exceptions, or a generated result type.

If the semantic states are not defined first, implementation mechanisms begin to define the language accidentally.

## Successful completion is not the only possible completion

Consider a function:

```cpp
User load_user(int id);
```

The obvious result is successful completion with a `User`.

Real programs can also encounter conditions such as:

- the requested user does not exist
- the database cannot be reached
- the operation is cancelled
- a timeout expires
- the computation is deliberately stopped
- an invariant is violated
- memory access is invalid
- the program reaches an impossible state

These conditions are not necessarily the same category.

Some are expected parts of normal application behavior.

Some represent absence.

Some are recoverable operational failures.

Some indicate deliberate interruption.

Some indicate that the program itself is incorrect.

Treating all of them as one generic "error" loses information that affects program structure, diagnostics, control flow, and recovery.

## Existing C++ mechanisms encode different meanings

C++ already provides several mechanisms that can represent unsuccessful computation.

For example:

```cpp
User *load_user(int id);
```

may use `nullptr` to mean absence or failure.

```cpp
std::optional<User> load_user(int id);
```

can express presence or absence.

```cpp
std::expected<User, LoadError> load_user(int id);
```

can express success or an explicit error value.

```cpp
User load_user(int id);
```

may throw an exception.

Another program may return an integer status code.

Another may terminate the process when a condition is violated.

The problem is not lack of mechanisms.

The problem is that these mechanisms exist at different abstraction levels and do not establish one language-level vocabulary for computation outcomes.

The programmer must infer the intended meaning from the selected type, library, documentation, convention, and surrounding code.

## Absence and failure are different

Consider:

```cpp
std::optional<User> find_user(int id);
```

An empty optional may mean:

> There is no user with this identifier.

That is not necessarily a failure.

Now consider:

```cpp
std::expected<User, DatabaseError>
load_user(int id);
```

An unexpected `DatabaseError` may mean:

> The operation could not determine whether the requested user exists because the database failed.

These outcomes should not automatically become the same state.

Absence answers a question about the result.

Failure answers a question about the execution of the operation.

If both are represented through one undifferentiated mechanism, callers must recover the distinction through conventions or encoded values.

## Recoverable failure and programmer error are different

A recoverable failure is something the program expects may happen during valid execution.

Examples include:

```text
connection refused
file not found
authentication rejected
remote service unavailable
invalid external input
```

A programmer error represents violation of assumptions required for correct execution.

Examples include:

```text
dereferencing an invalid pointer
violating an internal invariant
accessing outside valid bounds
reaching a state declared impossible
breaking an API precondition
```

These categories have different engineering consequences.

Recoverable failures often belong in normal control flow.

Programmer errors usually require diagnostics, debugging, termination, or another mechanism that makes the defect visible.

Turning every programmer error into an ordinary recoverable value can hide defects.

Turning every operational failure into a fatal program error can make robust recovery impossible.

The language model should therefore avoid treating them as equivalent.

## Failure and interruption are different

An operation may stop without succeeding or producing a recoverable failure.

Cancellation is a common example.

Suppose an asynchronous operation is cancelled because the caller no longer needs the result.

That is not necessarily:

```text
success
```

and it is not necessarily:

```text
failure
```

The operation may simply have been stopped intentionally.

This distinction becomes important when reasoning about asynchronous systems, task lifetimes, cleanup, concurrency, and structured cancellation.

The first VixC model therefore reserves a separate semantic state for stopped computation.

Its complete language behavior is not yet defined.

## The current VixC outcome hypothesis

The current research model distinguishes four ordinary computation outcomes:

```text
success
none
failure
stopped
```

They are intentionally separate.

### `success`

The computation produced its normal successful result.

### `none`

The computation completed normally but no value exists.

This is intended for cases where absence is meaningful and distinct from operational failure.

### `failure`

The computation completed through a recoverable failure declared by its contract.

### `stopped`

The computation did not continue to ordinary completion because execution was intentionally stopped or cancelled.

This model does not include programmer errors as ordinary outcomes.

That exclusion is part of the hypothesis being tested.

## The first research slice

The first implementation focuses on:

```text
success
failure
```

The proposed source model is:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

This introduces three language concepts:

```cpp
fails
fail
try
```

`fails E` declares that the computation can produce a recoverable failure of type `E`.

`fail value;` produces that recoverable failure.

`try expression` evaluates another failure-aware computation and propagates a compatible failure.

The research question is whether these three concepts can form a coherent semantic system rather than merely shorter syntax around an existing result type.

## Why a library type alone may be insufficient

A type such as:

```cpp
std::expected<T, E>
```

can represent a successful value or an error value.

That is useful.

But representation and language semantics are not identical.

A language-level failure model may need to reason about:

- where failure is legal
- whether a function declares failure
- propagation
- failure compatibility
- control flow after failure
- unreachable code
- diagnostics
- composition with future asynchronous semantics
- interaction with cancellation
- source-level tooling

A library type cannot independently make the parser or semantic analyzer understand those rules.

The programmer can build conventions around the type, but those conventions remain outside the language unless tooling reconstructs them indirectly.

VixC is investigating whether some of those rules deserve direct frontend representation.

## Why exceptions alone may be insufficient

Exceptions already provide language-level propagation.

They solve an important class of problems.

The research question is whether every recoverable operational failure should necessarily have exception semantics.

Exception-based failure has properties involving:

- non-local control flow
- stack unwinding
- catch matching
- exception specifications historically used by C++
- exception-enabled or exception-disabled build environments
- interaction with APIs that expose errors as values

A VixC failure contract should not automatically inherit all exception semantics unless that is explicitly the intended model.

The language meaning should be defined independently.

A backend may later decide whether exceptions are an appropriate implementation strategy for some configuration, but the source semantics should not be derived from that choice.

## Why error codes alone may be insufficient

Error codes make control flow explicit but place significant responsibility on the programmer.

For example:

```cpp
auto result = read_record(id);

if (!result)
    return result.error();

auto record = *result;
```

This can be correct and understandable.

Repeated across many call layers, however, propagation becomes mechanical.

The programmer repeatedly expresses the same semantic operation:

> If this computation failed, propagate that failure. Otherwise use its successful value.

The proposed `try` operation investigates whether that repeated semantic pattern deserves first-class representation.

The objective is not only fewer lines of code.

The deeper question is whether explicit propagation semantics improve analysis, diagnostics, composition, and reasoning.

## Propagation is a semantic operation

Consider:

```cpp
auto record = try read_record(id);
```

The interesting part is not the spelling `try`.

The important semantic requirement is:

1. evaluate `read_record(id)`
2. determine its outcome
3. if successful, obtain its successful value
4. if it produced a compatible recoverable failure, propagate that failure
5. do not continue the surrounding success path after propagation

This is a control-flow operation.

Representing it explicitly gives the frontend information that would otherwise be hidden inside manually written branching or library helper machinery.

That information may later support better diagnostics and analysis.

## Failure contracts must belong to a scope

A token such as:

```cpp
fails LoadError
```

cannot simply affect every `fail` or `try` token appearing later in the file.

The contract belongs to a specific computation.

For example:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);
    return make_user(record);
}
```

The body must be analyzed under the `LoadError` failure contract.

A different function may have another contract:

```cpp
Config load_config() fails ConfigError
{
    // ...
}
```

Nested constructs may eventually introduce additional questions about scope.

The frontend therefore needs declaration-level structure rather than a token-rewriting model.

This requirement already affects parser architecture.

## Failure types require semantic compatibility

Consider:

```cpp
User load_user() fails LoadError
{
    auto data = try read_network_data();
}
```

Suppose `read_network_data()` can fail with:

```cpp
NetworkError
```

The frontend must eventually answer whether `NetworkError` can be propagated through a computation that declares `LoadError`.

Possible language models include:

- exact type equality
- implicit conversion
- explicit conversion
- declared mapping
- variant-like composition
- automatic union of failure types

The current research has not committed to all of these rules.

What is already clear is that the answer belongs to semantic analysis.

It should not be guessed from string spelling or deferred blindly to generated C++ template errors.

## Failure must preserve evaluation semantics

Any implementation of:

```cpp
auto value = try operation();
```

must preserve normal language properties such as evaluation order and lifetime behavior.

The generated representation must not accidentally:

- evaluate `operation()` twice
- move from a value unexpectedly
- extend or shorten lifetimes incorrectly
- reorder side effects
- silently copy move-only values
- change exception behavior of ordinary C++ inside the operand
- introduce references to destroyed temporaries

This is one reason Failure cannot be treated as simple textual substitution.

The frontend and backend must preserve semantics across translation.

## Generic propagation macros are not the language model

C++ projects frequently implement propagation using macros.

A conceptual example is:

```cpp
TRY(value, operation());
```

Macros can reduce repetition, but they have limitations as a semantic foundation.

They operate through preprocessing rather than through a dedicated frontend model.

Their behavior can be difficult to integrate with:

- precise diagnostics
- source locations
- expression semantics
- tooling
- control-flow analysis
- refactoring
- language composition

VixC may generate helper machinery internally if required, but the public language model should not be defined as a macro convention.

## Diagnostics are part of the problem

A language feature should improve the point at which errors can be understood.

For example:

```cpp
void log_message()
{
    fail NetworkError{};
}
```

If the function has no failure contract, VixC already knows the semantic problem.

The diagnostic should therefore explain that `fail` requires a failure-aware computation.

It should not generate invalid C++ and leave the programmer with a later error involving an implementation type.

Similarly, a propagation incompatibility should eventually be diagnosed in terms of the source-level failure contracts involved.

This is part of the value of representing failure semantics before backend generation.

## C++ compatibility constrains the design

VixC is not designed in isolation from C++.

Existing valid C++ must be considered when introducing syntax.

For example, C++ already contains:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

Using `try` for propagation creates an ambiguity that the frontend must handle deliberately.

The current parser treats a `try` followed by a block as native C++.

That is only the beginning of the compatibility problem.

Research must consider whether the syntax remains understandable and robust across realistic C++ source.

A language construct that works only in artificial examples is insufficient.

## The model must survive real C++ types

Failure semantics must eventually work with real C++ values, including:

- move-only types
- references
- `const` values
- templates
- generic functions
- overloaded functions
- user-defined conversions
- RAII objects
- coroutine-related types
- custom allocators
- incomplete types where legal
- platform-specific types

A model that only works with trivial scalar examples would not be suitable for VixC.

This is another reason the research must remain connected to real programs.

## The model must not require a runtime without evidence

It would be possible to introduce a dedicated VixC runtime representation for every outcome.

That would impose a permanent architectural dependency before demonstrating that one is required.

The first research direction therefore prefers semantics that can be lowered into ordinary native C++.

Runtime support should only be added when the language semantics require behavior that generated C++ and existing platform facilities cannot represent adequately.

Failure alone does not yet demonstrate such a requirement.

## Questions the research must answer

The Failure and Outcome work must answer several questions before the model can be considered stable.

### Semantic questions

What exactly constitutes recoverable failure?

Can a function declare more than one failure type?

Can failure types be composed?

How are nested failure contracts handled?

How is failure compatibility determined?

Does propagation permit implicit conversion?

How do references and move-only values behave across propagation?

Can constructors fail through this model?

Can destructors participate in recoverable failure?

How does the model interact with exceptions?

Can a failure-aware function also throw?

What happens in `noexcept` code?

### Outcome questions

When should absence use `none` rather than `failure`?

What operations introduce `none`?

Can `none` and `failure` coexist in the same computation?

What exactly does `stopped` mean?

Is stopped observable by callers?

How does stopped interact with cleanup?

How does stopped interact with asynchronous execution?

### Syntax questions

Is `fails E` the correct declaration syntax?

Is `fail value;` sufficiently clear?

Is `try expression` compatible enough with existing C++?

Can propagation participate naturally in larger expressions?

How does it behave with declarations, conditions, function arguments, and return expressions?

### Type-system questions

How does VixC discover the outcome contract of a called function?

How does this work across declarations and translation units?

How does it work with templates?

Can failure contracts participate in overload resolution?

Are failure contracts part of function type identity?

How are ABI boundaries affected?

### Backend questions

What C++ representation preserves the semantics with minimal overhead?

Should the generated representation use a standard type where possible?

How does generated control flow preserve RAII?

How are temporary lifetimes preserved?

How are native compiler diagnostics mapped back to VixC source?

Can the representation remain understandable when inspecting generated C++?

### Tooling questions

Can editors understand the declared outcomes of a function?

Can diagnostics identify unhandled outcomes?

Can refactoring preserve Failure semantics?

Can documentation tools expose the failure contract?

Can static analysis reason about propagation without reconstructing backend-specific patterns?

## Non-goals of the first experiment

The first Failure experiment is not trying to solve every error-handling problem in C++.

It is not currently trying to:

- remove exceptions from C++
- replace `std::expected`
- replace `std::optional`
- convert all APIs into failure-aware APIs
- make every function declare every possible machine-level failure
- turn programmer errors into recoverable values
- create a universal runtime error hierarchy
- redesign the entire C++ type system
- define cancellation completely
- define asynchronous execution completely

The experiment is narrower.

It asks whether recoverable failure can have a precise, composable language model that integrates naturally with ordinary C++.

## Criteria for a useful model

A Failure model is not successful merely because a small example compiles.

It should eventually satisfy several stronger properties.

The model should be understandable from source without knowing backend internals.

Semantic errors should be diagnosed before native code generation when VixC has enough information.

Propagation should preserve C++ evaluation and lifetime semantics.

The model should work with realistic C++ APIs and types.

Ordinary C++ should remain available where Failure semantics are not needed.

The backend representation should remain replaceable without changing source meaning.

Generated code should compile through existing native toolchains.

Source provenance should survive the translation.

The model should compose with future work rather than making cancellation, asynchronous execution, ownership, or pattern matching harder to define coherently.

## Current evidence

The current VixC implementation already demonstrates that Failure can be represented through distinct frontend layers.

The lexer recognizes `fails`, `fail`, and VixC propagation syntax.

The parser creates dedicated syntax nodes.

Semantic analysis distinguishes valid and invalid Failure contexts.

The frontend has explicit semantic Outcome modeling.

The IR contains dedicated `Outcome`, `Failure`, and `FailurePropagation` nodes.

Lowering and backend boundaries exist independently of syntax.

Source provenance is retained.

The remaining important gap is declaration-level semantic scope.

The parser does not yet connect:

```cpp
fails Error
```

to the complete function body in a way that lets semantic analysis keep that contract active naturally.

Until that is implemented, the Failure experiment is not yet a complete vertical slice.

## Research objective

The immediate objective is not to add more outcome states or more syntax.

It is to complete one coherent recoverable Failure path.

A real function should be able to declare a failure contract:

```cpp
User load_user(int id) fails LoadError
```

produce failure:

```cpp
fail LoadError{};
```

propagate failure:

```cpp
auto record = try read_record(id);
```

and preserve those semantics through parsing, semantic analysis, IR, lowering, C++ generation, native compilation, and diagnostics.

Once that path works in real programs, the evidence can be used to decide which parts of the model deserve to become stable VixC language semantics.
