# Failure and Outcome

Failure is the first language-level semantic model implemented by VixC.

The purpose of this work is not to introduce another C++ error wrapper. VixC is exploring whether recoverable failure can have a precise language meaning that remains explicit through parsing, semantic analysis, intermediate representation, lowering, and backend generation.

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

A computation declared with `fails E` can complete successfully or produce a recoverable failure of type `E`.

The exact C++ representation of that contract is intentionally not part of the source-level semantics.

## Failure contract

The `fails` construct declares the recoverable failure contract of a computation.

```cpp
User load_user(int id) fails LoadError
{
    return make_user(id);
}
```

The successful result of the computation is still `User`.

`LoadError` describes an additional recoverable completion state.

VixC treats this as semantic information rather than as text that should immediately be replaced with a C++ type.

The frontend records the contract so that later stages can reason about operations that produce or propagate failure.

## Producing failure

The `fail` construct explicitly completes the current failure-aware computation with its recoverable failure outcome.

```cpp
fail LoadError{};
```

The operand can be an ordinary C++ expression:

```cpp
fail make_error(code);
```

`fail` does not mean `throw`, `return`, `std::terminate`, or any other existing C++ operation.

Those mechanisms already have their own semantics.

The language meaning of:

```cpp
fail error;
```

is that the current failure-aware computation completes with the declared recoverable failure.

How that operation is represented in generated C++ belongs to lowering and the C++ backend.

## Propagating failure

The `try` construct evaluates another failure-aware computation.

```cpp
auto record = try read_record(id);
```

If `read_record(id)` completes successfully, its successful value becomes the value used by the surrounding expression.

If it produces a compatible recoverable failure, that failure is propagated through the current failure-aware computation.

The purpose is to make propagation part of the semantic model instead of requiring every program to manually repeat backend-specific checking and forwarding code.

## Outcome

VixC models completion explicitly.

The current outcome model distinguishes `success`, `none`, `failure`, and `stopped`.

The first Failure implementation focuses on `success` and recoverable `failure`.

`none` and `stopped` already exist as distinct concepts in the model, but their complete source syntax and language rules are not part of this first vertical slice.

A programmer error or contract violation is not an ordinary recoverable outcome.

This distinction is intentional. A condition that the application is expected to handle and a condition indicating that the program itself violated an invariant should not silently become the same category.

## Semantic representation

Failure is represented before the C++ backend is involved.

The semantic layer validates where Failure constructs are legal and establishes the active failure contract.

The IR then preserves the meaning using dedicated nodes:

- `Outcome`
- `Failure`
- `FailurePropagation`

`Outcome` represents the completion contract of a computation.

`Failure` represents explicit production of a recoverable failure.

`FailurePropagation` represents evaluation of another computation with propagation of its recoverable failure.

These nodes do not contain generated C++ fragments.

They describe what the program means.

## Relationship with C++

VixC does not replace ordinary C++ when no additional language semantics are required.

Expressions used by Failure constructs can remain ordinary C++:

```cpp
fail make_error(code);
```

```cpp
auto value = try read_value(stream);
```

The expression `make_error(code)` does not need a second VixC expression language merely because it appears after `fail`.

Likewise, `read_value(stream)` remains an ordinary C++ expression whose relationship with the Failure model is established by VixC semantics.

This allows VixC to introduce selected language concepts without attempting to replace the entire C++ language.

## Why representation is deferred

A recoverable failure could eventually be represented in C++ through several mechanisms.

For example, a backend could use a generated result type, explicit branching, helper functions, or another native representation.

Choosing one of those mechanisms before defining the semantic model would make the implementation mechanism define the language.

VixC instead keeps Failure explicit until the backend boundary.

The source says what should happen.

The backend decides how to preserve that behavior in ordinary C++.

## Source provenance

Every Failure construct retains its original `SourceRange`.

Ordinary C++ regions also retain their original ranges.

When the C++ backend emits source, `CxxSourceMap` records relationships between generated byte ranges and the corresponding original source.

Generated code does not need to have the same size or spelling as the original construct.

The mapping represents provenance.

This is intended to make it possible for diagnostics produced later by GCC, Clang, or MSVC to eventually be related back to the original VixC source.

## Current implementation

The current frontend already contains the infrastructure required for the Failure experiment: source management, structured diagnostics, lexical recognition of `fails`, `fail`, and `try`, syntax nodes, Failure semantic analysis, backend-independent Failure IR, common lowering, C++ backend infrastructure, source mapping, the public embedding API, and the `vixc` command-line frontend.

There is still one important architectural gap.

The parser currently recognizes the failure specification and the Failure operations independently.

For example:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);
    fail LoadError{};
}
```

contains a `FailureSpecification`, a `TryExpression`, and a `FailStatement`, but the current syntax tree does not yet represent the enclosing function declaration and body as one failure-aware scope.

Semantic analysis therefore cannot naturally keep `fails LoadError` active while analyzing the body.

Tests can currently create a Failure context explicitly and validate `fail` and `try` inside that context, but the complete source form above is not yet accepted end-to-end.

This must be solved in the syntax and semantic layers rather than hidden inside the backend.

## First complete milestone

The first Failure vertical slice is complete when VixC can accept a real failure-aware function such as:

```cpp
User load_user(int id) fails LoadError
{
    auto record = try read_record(id);

    if (!record.valid())
        fail LoadError{};

    return make_user(record);
}
```

and carry its meaning through the complete frontend.

The frontend must recognize the declaration-level Failure contract, validate the body under that contract, construct semantic IR, lower Failure and propagation operations, generate deterministic ordinary C++, and preserve source provenance.

The generated C++ can then be compiled by GCC, Clang, or MSVC.

The Failure semantics belong to VixC.

Native C++ compilation remains the responsibility of the existing C++ toolchain.
