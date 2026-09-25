# VixC Language Model

VixC is a programming-language frontend designed to introduce selected language semantics while preserving the C++ ecosystem underneath them.

The language model is intentionally narrow.

VixC does not attempt to redefine every part of C++, replace its type system, introduce a completely independent object model, or hide the native platform behind a new runtime abstraction.

A VixC program remains closely connected to C++.

The frontend intervenes where a concept requires language-level meaning that cannot be expressed coherently enough through ordinary library APIs, conventions, or source-level patterns alone.

The first such concept is recoverable failure.

## C++ remains the native foundation

VixC is designed to work with existing native C++ implementations.

Generated programs are intended to remain compatible with toolchains such as GCC, Clang, and MSVC.

Existing C++ remains available for:

- types
- classes
- templates
- functions
- expressions
- containers
- libraries
- platform APIs
- operating-system interfaces
- native dependencies

VixC should not introduce a second version of a C++ concept merely because VixC has a frontend.

If ordinary C++ already expresses something adequately, that code should remain ordinary C++.

For example:

```cpp
auto value = calculate(input);
```

does not need a VixC-specific expression model merely because it appears inside a VixC-processed source file.

The language boundary appears when VixC introduces additional meaning.

For example:

```cpp
auto value = try calculate(input);
```

contains a VixC semantic operation.

The call:

```cpp
calculate(input)
```

may remain ordinary C++.

The propagation behavior introduced by `try` belongs to VixC.

## Language semantics before representation

A VixC construct is defined by its meaning, not by the C++ code eventually generated for it.

This is a central rule of the language model.

Consider:

```cpp
fail error;
```

The language does not define this as shorthand for:

```cpp
return unexpected(error);
```

or:

```cpp
throw error;
```

or any other particular C++ implementation.

Instead, `fail` means that the current failure-aware computation completes with a recoverable failure.

The C++ backend must implement that meaning.

This distinction allows the backend representation to evolve without changing the source-level language contract.

It also prevents implementation details from becoming accidental language semantics.

## Ordinary C++ and VixC-owned syntax

VixC distinguishes between source that requires VixC semantic understanding and source that can remain ordinary C++.

The frontend currently preserves ordinary C++ using `CxxRegion`.

A `CxxRegion` records an original source range without requiring VixC to fully reconstruct the C++ grammar represented by that region.

This is an important architectural boundary.

VixC should understand a construct when VixC owns its semantics.

VixC does not need to understand every C++ construct merely to preserve and compile it.

For example:

```cpp
std::vector<User> users;
```

can remain ordinary C++.

So can:

```cpp
auto user = repository.find(id);
```

But:

```cpp
auto user = try repository.find(id);
```

requires VixC to understand the propagation operation introduced by `try`.

The language therefore extends selected parts of C++ source without requiring an independent replacement for the entire language.

## Minimal language surface

New syntax should not be added merely because a concept can be made shorter.

A language-level feature should exist only when the underlying semantic problem requires it.

The frontend should first determine whether ordinary C++ mechanisms are sufficient.

If a library type, function, template, or convention expresses the concept coherently, VixC should prefer normal C++.

A new language surface becomes justified when VixC needs to establish rules that must be understood across syntax, semantics, control flow, diagnostics, lowering, and tooling.

Recoverable failure is the first experiment under this rule.

## Computations and outcomes

The first VixC semantic model treats computation completion explicitly.

A computation can have different kinds of outcomes.

The current model distinguishes:

- `success`
- `none`
- `failure`
- `stopped`

These states represent different semantic meanings.

They should not be collapsed merely because one backend could encode several of them with the same C++ type.

### Success

`success` represents normal successful completion.

A function such as:

```cpp
User load_user();
```

normally produces a successful `User` value.

Success remains the default outcome.

### Failure

`failure` represents a recoverable problem declared by a computation.

For example:

```cpp
User load_user(int id) fails LoadError
{
    // ...
}
```

declares that the computation can complete successfully with `User` or produce a recoverable `LoadError`.

A recoverable failure is part of the function's language contract.

It is not equivalent to a programmer error.

### None

`none` represents absence of a value where absence is semantically distinct from failure.

The current IR reserves this state, but its complete source syntax and semantic rules are not yet defined.

VixC should not automatically treat absence as recoverable failure because those states answer different questions.

### Stopped

`stopped` represents computation that does not complete through ordinary success or recoverable failure because execution has been intentionally stopped.

The semantic category exists in the model, but its complete language behavior is not yet specified.

Cancellation is one future area where this distinction may become relevant.

## Programmer errors are not outcomes

A violated invariant, broken contract, invalid memory access, impossible state, or other programmer error should not automatically become an ordinary recoverable `failure`.

This distinction is deliberate.

Recoverable failure describes a condition the program is designed to represent and handle.

A programmer error indicates that assumptions required for correct execution have been violated.

Conflating the two weakens both error handling and diagnostics.

VixC therefore does not currently include programmer errors as a normal `OutcomeState`.

## Failure contracts

The first concrete outcome feature is declared with `fails`.

```cpp
User load_user(int id) fails LoadError
{
    // ...
}
```

The successful result remains:

```cpp
User
```

The additional recoverable outcome is:

```cpp
LoadError
```

The failure type is part of the semantic contract of the computation.

The contract must remain visible to semantic analysis so operations inside the body can be checked against it.

The source syntax should not be interpreted as a request for a particular generated result wrapper.

## Explicit failure

A failure-aware computation produces recoverable failure using `fail`.

```cpp
fail LoadError{};
```

The operand can be an ordinary C++ expression:

```cpp
fail make_error(code);
```

The semantic rule is that the current computation completes through its recoverable failure outcome using the supplied failure value.

The operation requires an active failure contract.

Therefore:

```cpp
fail error;
```

outside a failure-aware computation is invalid VixC.

This rule belongs to semantic analysis and should not be postponed until generated C++ compilation.

## Failure propagation

A recoverable failure can be propagated with `try`.

```cpp
auto user = try load_user(id);
```

Conceptually, `try` evaluates the operand computation.

If the operand succeeds, its successful value becomes the value of the expression.

If the operand produces a compatible recoverable failure, that failure is propagated from the current computation.

The propagation semantics are part of VixC.

The exact generated control flow is not.

This allows VixC to provide one semantic model while retaining freedom in backend implementation.

## Native C++ `try`

C++ already uses `try` for exception handling:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

VixC must preserve native C++ meaning where the source is using the existing C++ construct.

The current parser distinguishes a native `try` block from VixC propagation by recognizing `try` followed by a block as ordinary C++.

This area requires careful evolution because VixC should not accidentally reinterpret valid C++ merely because the same spelling appears in the source.

Compatibility with existing C++ syntax is a language constraint, not only a parser implementation detail.

## Failure type compatibility

A `try` operation eventually needs to establish whether the failure produced by its operand can be propagated through the enclosing failure contract.

For example:

```cpp
User load_user() fails LoadError
{
    auto record = try read_record();
}
```

requires semantic knowledge about the failure contract of `read_record()`.

The first frontend infrastructure does not yet implement a complete C++ type-resolution system.

Failure type compatibility is therefore a semantic responsibility that must be introduced as the frontend gains sufficient declaration and type information.

It should not be guessed from source spelling alone.

## Declaration scope

A failure contract belongs to a computation, not to every source construct that follows a `fails` token.

This means the frontend must understand the relationship between:

```cpp
User load_user() fails LoadError
```

and the corresponding function body.

The body must be analyzed with the failure contract active.

For example:

```cpp
User load_user() fails LoadError
{
    auto record = try read_record();

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

Both `try` and `fail` belong to the failure-aware scope introduced by the declaration.

The current parser recognizes these constructs individually but does not yet represent the entire declaration and body as one semantic scope.

That is a known limitation of the current frontend implementation.

The language model itself requires declaration-level scoping.

## Semantic IR

After semantic validation, VixC-owned language concepts are represented in semantic IR.

For Failure, the current IR contains:

```text
Outcome
Failure
FailurePropagation
```

`Outcome` represents the completion contract.

`Failure` represents explicit production of recoverable failure.

`FailurePropagation` represents propagation from another failure-aware computation.

The IR should retain language meaning while removing unnecessary syntax-level detail.

It should not contain generated C++ source as its semantic representation.

## Lowering

Lowering transforms semantic IR into forms suitable for backend implementation.

The lowering layer can make control-flow requirements more explicit and introduce synthesized internal operations where required.

However, lowering remains backend-independent unless a transformation is inherently target-specific.

For example, lowering can establish that a propagation operation requires:

- evaluation of an operand
- inspection of its outcome
- propagation of failure
- extraction of its success value

That does not require the common lowering stage to decide which C++ result type represents the operation.

The concrete representation belongs to the C++ backend.

## C++ backend semantics

The first VixC backend emits ordinary C++.

Its responsibility is to preserve the semantic meaning already established by the frontend.

For Failure, a future complete backend representation must preserve at least:

- the declared failure type
- successful completion
- explicit failure production
- propagation behavior
- evaluation order
- value semantics
- source provenance

The backend may use ordinary C++ machinery to implement these rules.

That machinery does not become part of the source-level language contract unless VixC explicitly specifies it.

## Runtime independence

A language feature does not automatically require a VixC runtime.

If a semantic operation can be represented directly in generated C++, VixC should not introduce runtime machinery solely for architectural symmetry.

Runtime support should be introduced only when the semantics require behavior that cannot reasonably be represented through generated native code and existing platform facilities.

The current Failure model therefore does not assume a runtime component.

## Source provenance

Source identity is part of the frontend model.

Language constructs retain original source ranges through syntax, semantic processing, IR, lowering, and backend generation where possible.

This matters because generated C++ is not the source language presented to the programmer.

When a later native compiler reports an error in generated source, VixC should eventually be able to relate that location back to the original construct.

Source provenance is therefore part of the design of the frontend rather than an optional debugging feature added after code generation.

## Diagnostics belong to the language layer

VixC should diagnose violations at the highest layer that understands them.

If `fail` appears outside a failure-aware computation, VixC knows why the program is invalid.

The frontend should therefore report that error directly instead of generating broken C++ and relying on a native compiler to produce an unrelated template or type error.

The same principle applies to future language features.

A semantic error should be reported by the semantic layer whenever VixC has enough information to understand it.

Native compiler diagnostics remain valuable for ordinary C++ and generated implementation details, but they should not replace diagnostics for VixC-owned semantics.

## Compatibility is not textual pass-through

Preserving C++ compatibility does not mean that VixC must treat every source file as uninterpreted text.

Compatibility means that existing C++ concepts remain usable and their established meaning is preserved unless VixC explicitly introduces new syntax and semantics.

Where VixC introduces a construct, the frontend must actually understand it.

Where VixC does not introduce additional meaning, normal C++ should remain available without unnecessary interference.

This boundary is more important than maximizing the number of tokens copied unchanged.

## Future semantic areas

The same language principles can be applied to future research areas.

Potential domains include:

- choice and pattern matching
- asynchronous execution
- cancellation
- ownership and lifetime semantics
- compile-time programming
- reflection
- composition

These are research directions, not automatically committed language features.

Each must first demonstrate that a language-level concept is needed.

The addition of new syntax should follow evidence from real programs and semantic requirements rather than an attempt to make VixC look syntactically different from C++.

## Language evolution

VixC should evolve through semantic contracts rather than syntax accumulation.

For each proposed language feature, the frontend should establish:

1. what problem cannot be expressed coherently enough with existing C++
2. the exact semantic meaning of the proposed construct
3. where that meaning is validated
4. how it is represented independently of a backend
5. what invariants lowering must preserve
6. what freedom remains available to backend implementations
7. how existing C++ continues to behave around the feature

Only after those questions have stable answers should the feature become part of the durable language surface.

## Current model

The current implementation establishes the first foundations of this language model.

VixC already has source management, diagnostics, lexical analysis, syntax nodes, semantic analysis, semantic IR, lowering infrastructure, a C++ backend, source mapping, an embedding API, and a command-line frontend.

Failure is the first feature exercising those layers.

The language model is not complete merely because the individual `fails`, `fail`, and `try` tokens can be parsed.

The first meaningful milestone is a complete failure-aware computation whose declaration, body, semantic contract, IR, lowering, generated C++, and source provenance all agree on one meaning.

That is the standard future VixC features should also satisfy.
