# Failure and Outcome Diagnostics

Diagnostics are part of the Failure language model.

A Failure feature is not complete merely because valid programs can be translated into C++.

The frontend must also recognize invalid Failure programs at the layer where their meaning is understood and explain the problem using the original VixC source.

The purpose of Failure diagnostics is therefore not to replace all diagnostics produced by GCC, Clang, or MSVC.

The purpose is to ensure that errors involving VixC-owned semantics are reported by VixC rather than leaking through as unrelated implementation errors from generated C++.

For example:

```cpp
void operation()
{
    fail NetworkError{};
}
```

should not become generated code that later produces a compiler error involving an internal result type.

VixC already knows the actual semantic problem:

```text
fail requires an active Failure contract
```

That is the diagnostic the programmer should receive.

## Diagnostic principle

A frontend stage should diagnose a problem when that stage has enough semantic information to explain it accurately.

Lexical problems belong to lexical analysis.

Structural problems belong to parsing.

Failure contract violations belong to semantic analysis.

Malformed semantic IR belongs to IR construction or lowering.

Backend representation failures belong to the backend.

Native C++ problems that VixC does not own remain the responsibility of the native compiler.

This separation matters because diagnostics should describe the programmer's source model rather than an internal implementation mechanism.

## Source-level terminology

Diagnostics for VixC-owned features should use the terminology of the language model.

For example, a diagnostic should prefer:

```text
'fail' requires a failure-aware computation
```

over:

```text
cannot construct generated outcome wrapper
```

when the actual problem is a source-level Failure contract violation.

Likewise:

```text
cannot propagate 'NetworkError' through a computation that fails with 'LoadError'
```

is more useful than a later C++ conversion failure involving generated implementation types.

Backend terminology should only appear when the problem genuinely belongs to backend generation.

## Diagnostic structure

VixC diagnostics are structured values.

A diagnostic contains:

- severity
- optional stable code
- human-readable message
- optional source range

The diagnostic engine stores them independently from presentation.

This allows the same diagnostic information to be consumed by:

- the `vixc` command-line tool
- Vix.cpp
- an editor
- an IDE
- tests
- another embedding application

The frontend should not embed terminal formatting or editor-specific behavior into the semantic layer.

## Severity

The current severity model contains:

```text
Note
Warning
Error
Fatal
```

### Note

A note provides additional information associated with another diagnostic or frontend event.

For example, a future failure compatibility diagnostic could use a note to point to the enclosing declaration:

```text
error: cannot propagate 'ReadError' through this computation
note: this computation declares recoverable failure as 'LoadError'
```

### Warning

A warning identifies suspicious but valid source.

Failure semantics should use warnings carefully.

A warning must not be used when the program violates a required language invariant.

For example, `fail` outside a Failure contract is an error, not a warning.

### Error

An error means the current source does not satisfy the language rules required to continue the requested frontend operation.

Most invalid Failure constructs are semantic errors.

### Fatal

A fatal diagnostic indicates that the frontend cannot continue processing reliably.

Fatal diagnostics should be reserved for conditions where later analysis cannot safely proceed.

A normal programmer mistake inside a valid source buffer should usually be an Error rather than Fatal.

## Stable diagnostic codes

VixC diagnostics may include stable codes.

The current implementation uses numeric families such as:

```text
VIXC1xxx
VIXC2xxx
VIXC3xxx
VIXC4xxx
VIXC5xxx
```

The existing code currently uses these families roughly according to frontend stage:

```text
VIXC1xxx  syntax and parsing
VIXC2xxx  semantic analysis
VIXC3xxx  lowering
VIXC4xxx  C++ backend
VIXC5xxx  frontend coordination and IR construction
```

This division is an implementation convention rather than a complete public diagnostic specification.

Before VixC diagnostics become stable API, the code namespace should be documented explicitly and collisions should be prevented.

A diagnostic code should identify a semantic condition rather than an incidental wording of the message.

The message may improve over time while the code remains useful to tooling and tests.

## Lexical diagnostics

The lexer is responsible for malformed lexical constructs.

Examples include unterminated literals.

For example:

```cpp
fail "network error;
```

the string literal itself is malformed before Failure semantics can be analyzed.

The lexer should report the malformed literal.

Semantic analysis should not attempt to infer Failure behavior from structurally invalid tokens.

The current lexer already reports errors for unterminated string and character literals.

## Unknown source bytes

The current lexer can represent an unrecognized byte as an `Invalid` token.

This area requires a stronger long-term rule.

Because VixC is intended to preserve ordinary C++, the frontend must be careful not to classify valid C++ punctuation as invalid merely because the initial lexer does not model it yet.

For example, ordinary C++ may contain operators or preprocessor syntax beyond the current first-generation token set.

A diagnostic should therefore only claim that source is invalid when VixC can distinguish:

```text
unsupported by the current frontend
```

from:

```text
invalid C++
```

VixC should not report false language errors for valid C++ it simply does not yet understand.

## Parsing diagnostics

The parser should diagnose malformed VixC syntax.

Examples include:

```cpp
fail;
```

when an operand is required.

```cpp
fail error
```

when the terminating semicolon is required.

```cpp
fails {
```

when the failure type is missing.

```cpp
try;
```

when a propagation operand is missing.

These are structural errors.

The parser does not need semantic type information to report them.

## `fail` without an operand

Given:

```cpp
fail;
```

the parser knows that the Failure statement does not contain the required expression.

The diagnostic should identify the `fail` construct and explain what is missing.

A useful form is conceptually:

```text
error: expected an expression after 'fail'
```

The source range should point at the malformed construct or the location where the expression was expected.

## Missing semicolon after `fail`

Given:

```cpp
fail error
```

the parser should report that the Failure statement requires termination.

For example:

```text
error: expected ';' after failure expression
```

The diagnostic should not describe this as a generic parser failure when the parser already knows it is inside a `fail` statement.

## Missing Failure type

Given:

```cpp
Value operation() fails
{
}
```

the parser should identify the missing type following `fails`.

A suitable message is conceptually:

```text
error: expected a failure type after 'fails'
```

Once declaration-level parsing exists, the diagnostic range can point directly at the incomplete specification.

## Missing propagation operand

Given:

```cpp
auto value = try;
```

the parser should explain that `try` requires a computation operand.

For example:

```text
error: expected an expression after 'try'
```

This is preferable to a later semantic or backend error because the source is structurally incomplete.

## Native C++ `try`

VixC must distinguish its propagation form from native C++ exception syntax.

For example:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

must not produce a Failure diagnostic merely because the first token is `try`.

If the parser cannot distinguish a source form reliably, it should avoid making a strong Failure-specific claim before enough structure is available.

Diagnostics must respect ordinary C++ compatibility.

## Semantic diagnostics

Semantic diagnostics are the most important part of the Failure model.

They explain situations where the syntax is structurally valid but its meaning is invalid.

Examples include:

```text
fail outside a Failure contract
try outside a Failure contract
invalid Failure contract
incompatible propagated failure type
incompatible explicit failure value
failure operation in a forbidden semantic context
```

These errors should be reported before lowering or backend generation.

## `fail` outside a Failure contract

Given:

```cpp
void operation()
{
    fail Error{};
}
```

the source is structurally valid as a `FailStatement`.

The semantic problem is that the enclosing computation does not declare recoverable failure.

The diagnostic should explain that relationship.

For example:

```text
error: 'fail' can only be used inside a failure-aware computation
```

A useful future diagnostic may include a suggestion such as:

```text
note: declare a recoverable failure contract with 'fails E' if this function is intended to propagate this failure
```

Such a note should only be offered when that suggestion is semantically appropriate.

VixC should not automatically recommend adding a Failure contract whenever the actual mistake may be an unintended `fail`.

## `try` outside a Failure contract

Given:

```cpp
Value operation()
{
    auto value =
        try load();
}
```

the enclosing computation has no Failure contract through which a recoverable failure can be propagated.

A diagnostic should explain that `try` requires an enclosing failure-aware computation.

For example:

```text
error: 'try' cannot propagate failure from a computation without an enclosing Failure contract
```

This should be reported by semantic analysis.

The backend should never receive this source as though it were semantically valid.

## Invalid Failure specification

A Failure specification may be syntactically present but semantically unusable.

Examples may eventually include:

```text
invalid failure type
incomplete type where completion is required
forbidden type category
conflicting declarations
dependent type that cannot be resolved in the current context
```

The current frontend only performs structural checks on the failure type range.

As semantic type information becomes available, diagnostics should move from source-range validation toward actual type-based validation.

## Explicit failure type mismatch

Given:

```cpp
Value operation() fails LoadError
{
    fail NetworkError{};
}
```

the frontend must eventually determine whether `NetworkError` is compatible with `LoadError`.

If it is not, the error belongs to semantic analysis.

A diagnostic should describe both types.

For example:

```text
error: cannot produce failure of type 'NetworkError' in a computation that fails with 'LoadError'
```

A note can point to the declaration of the active Failure contract.

For example:

```text
note: the enclosing computation declares 'fails LoadError' here
```

This is significantly more useful than allowing generated C++ to fail through an unrelated conversion error.

## Propagation type mismatch

Given:

```cpp
Data read_data() fails ReadError;
```

and:

```cpp
Value operation() fails LoadError
{
    auto data =
        try read_data();
}
```

if `ReadError` cannot be propagated through `LoadError`, semantic analysis should explain the mismatch.

For example:

```text
error: cannot propagate failure type 'ReadError' through a computation that fails with 'LoadError'
```

Additional information could point to:

- the called computation's Failure contract
- the enclosing computation's Failure contract
- an available explicit conversion or handling mechanism, if the language later defines one

The frontend should not report this using the backend representation type.

## Missing declaration-level scope

The current implementation has a known architectural limitation.

For:

```cpp
Value operation() fails Error
{
    fail Error{};
}
```

the parser currently recognizes `fails Error` and `fail Error{}` as separate constructs without representing the enclosing declaration and body as one failure-aware syntax scope.

As a result, semantic analysis currently reports `fail` as occurring outside an active Failure context when processing this complete source form.

This diagnostic reflects the current implementation state but is not the desired language behavior.

Once declaration-level Failure scope is implemented, this source should no longer produce that error.

Tests should distinguish temporary implementation limitations from intended stable diagnostics.

## Diagnostic recovery

A frontend should attempt to continue after recoverable syntax and semantic errors when doing so produces useful additional diagnostics without corrupting analysis.

However, continuing after an error must not cause cascades of misleading Failure diagnostics.

For example, if a Failure specification is malformed:

```cpp
Value operation() fails
{
    auto value = try load();
}
```

the frontend should avoid reporting many secondary messages such as:

```text
try outside Failure context
fail outside Failure context
unknown failure type
```

if all of them are consequences of the missing declaration type.

The parser or semantic layer should preserve enough error state to suppress diagnostics that depend on already-invalid information.

## Cascading diagnostics

A cascading diagnostic is an error that is technically observed later but is only a consequence of an earlier error.

VixC should avoid overwhelming programmers with such errors.

For example:

```cpp
Value operation() fails
{
    fail Error{};
}
```

if the missing type after `fails` prevents establishing the contract, the primary diagnostic is the malformed Failure specification.

Reporting an additional:

```text
'fail' can only be used inside a failure-aware computation
```

may be misleading because the programmer clearly attempted to declare such a computation.

Diagnostic recovery should recognize this distinction when enough information exists.

## Diagnostics and source ranges

Every Failure diagnostic should point to the smallest useful original source range.

For:

```cpp
fail error;
```

a diagnostic about the legality of `fail` can point to the complete statement or the `fail` keyword.

A diagnostic about the failure value type should generally point to:

```cpp
error
```

rather than the entire function.

A diagnostic about the enclosing Failure contract may point to:

```cpp
fails Error
```

or specifically:

```cpp
Error
```

depending on the issue.

Precise ranges are important for editor integration and for avoiding ambiguous diagnostics in large expressions.

## Primary and secondary ranges

The current `Diagnostic` structure contains one primary source range.

Future diagnostics may benefit from secondary source ranges.

For example:

```cpp
Value operation() fails LoadError
{
    auto data =
        try read_data();
}
```

a propagation type mismatch naturally refers to two places:

```text
primary:
    try read_data()

secondary:
    fails LoadError
```

and possibly a third declaration:

```text
read_data() fails ReadError
```

The current diagnostic model can approximate this with separate Note diagnostics.

A future structured diagnostic API may support related ranges explicitly.

## Notes as related information

Until multi-range diagnostics exist, notes can preserve useful context.

For example:

```text
error [VIXC2xxx]: cannot propagate 'ReadError' as 'LoadError'
note: 'read_data' declares recoverable failure as 'ReadError'
note: the enclosing computation declares recoverable failure as 'LoadError'
```

Each note should carry the relevant source range where available.

This allows CLI and editor clients to present the relationship clearly.

## Error wording

Diagnostic wording should describe:

1. what is wrong
2. which source concept is involved
3. what semantic requirement was violated

A message should avoid exposing implementation classes unless the error concerns internal frontend correctness.

For programmer-facing diagnostics:

```text
error: 'fail' requires a failure-aware computation
```

is better than:

```text
error: missing FailureContext
```

The second message describes the compiler implementation.

The first describes the language rule.

## Internal diagnostics

Some diagnostics indicate invalid internal frontend state rather than invalid source.

Examples include:

```text
invalid IR node reached lowering
Outcome IR has incompatible concrete type
FailurePropagation IR has no operand
unsupported IR node reached the backend
```

The current implementation reports such conditions through the same diagnostic engine.

That is useful during frontend development.

However, internal errors should eventually be clearly distinguishable from normal source diagnostics.

A user should not be expected to fix:

```text
IR node marked as Failure has an incompatible concrete type
```

by changing ordinary source code.

Future diagnostic design may need a separate internal-error category or a clear diagnostic prefix.

## IR diagnostics

IR construction should only receive semantically valid syntax.

If IR construction cannot represent a valid semantic construct, that generally indicates one of two situations:

- the frontend has an implementation defect
- the semantic model lacks required information

For example, the current frontend may report:

```text
failure statement has no active failure contract
```

during IR construction.

Long term, that condition should normally have been resolved during semantic analysis.

IR diagnostics should therefore become increasingly focused on invariant violations rather than source-language validation.

## Lowering diagnostics

Lowering receives validated semantic IR.

Diagnostics from lowering should identify problems such as:

```text
malformed IR
missing required semantic information
unsupported lowering state
internal invariant violation
```

Lowering should not rediscover normal source-language errors already understood by semantic analysis.

For example, whether `fail` is legal in the current function belongs to semantics.

Whether a valid Failure IR node contains the information required for lowering belongs to lowering.

## Backend diagnostics

The C++ backend should diagnose failures specific to translating valid lowered IR into C++.

Examples include:

```text
unable to recover preserved source text
unsupported backend representation
missing declaration-level representation required for Failure emission
source-map construction failure
```

The current backend intentionally rejects `Failure` and `FailurePropagation` when they reach final emission without declaration-level and control-flow lowering.

Messages such as:

```text
Failure IR requires declaration-level C++ lowering before emission
```

record an implementation boundary.

They are not intended to become the final programmer-facing diagnostic for valid Failure source.

Once the lowering path is complete, valid Failure constructs should not reach those rejection paths.

## Native compiler diagnostics

Generated C++ is still C++.

GCC, Clang, or MSVC may report errors for ordinary C++ code that VixC preserved.

For example:

```cpp
auto value =
    unknown_function();
```

may remain an ordinary C++ semantic error that the native compiler is best positioned to diagnose.

VixC does not need to duplicate the entire C++ semantic analyzer.

However, when the diagnostic points into generated code corresponding to VixC syntax, the frontend should eventually map that location back to original VixC source.

This is the purpose of generated source provenance.

## Source-map diagnostic translation

The C++ backend records generated-to-original mappings through `CxxSourceMap`.

A generated compiler diagnostic may identify:

```text
generated file
generated line
generated column
```

Vix.cpp or another integration layer can convert that position into a generated byte offset and query the corresponding original `SourceRange`.

The translated diagnostic can then point to the VixC source.

This mechanism is particularly important when the generated C++ representation of:

```cpp
try operation()
```

contains several lines of implementation code.

The programmer should not need to search through generated source to locate the original operation.

## Generated helper diagnostics

Some generated C++ diagnostics may originate in backend helper code with no direct source equivalent.

For example, a backend-generated support type could fail to instantiate because of a property of the user's type.

In such cases, a one-to-one source mapping may not exist.

The backend may need richer provenance such as:

```text
generated helper originated because of Failure contract at source range X
```

The current source map only associates generated intervals with original ranges.

Future diagnostic work may need semantic provenance in addition to byte mapping.

## Templates

C++ templates can produce long diagnostics even when the underlying problem is simple.

If VixC uses template-based implementation machinery for Failure, exposing those diagnostics directly would weaken the value of the frontend.

For example, an invalid failure value could otherwise result in pages of generated template instantiation output.

When VixC already understands the relevant type incompatibility semantically, it should report that error before generating such code.

Native template diagnostics should remain a fallback for ordinary C++ behavior that VixC does not model.

## Diagnostic quality and backend choice

Backend representation affects diagnostic quality.

A representation that is semantically correct but routinely produces incomprehensible native compiler diagnostics may still be a poor implementation choice.

Backend experiments should therefore evaluate:

- semantic correctness
- runtime cost
- compile-time cost
- generated code size
- source-map quality
- native compiler diagnostic behavior

Diagnostic quality is part of the engineering evidence used to compare backend strategies.

## Diagnostics for move-only values

Failure propagation must eventually handle move-only values correctly.

If an implementation accidentally requires copying:

```cpp
std::unique_ptr<Resource>
```

the programmer should ideally receive a diagnostic connected to the propagation semantics rather than an obscure generated helper error.

However, VixC should not hide legitimate C++ type-system constraints.

The frontend must distinguish:

```text
VixC lowering introduced an invalid copy
```

from:

```text
the user's ordinary C++ operation itself requires an invalid copy
```

This requires semantic type information and careful generated-source provenance.

## Diagnostics for references and lifetimes

Reference and lifetime mistakes can be especially difficult to diagnose after source translation.

If Failure lowering materializes a temporary and returns a reference to it, that is a compiler implementation defect rather than a source error.

If the user's source itself creates an invalid reference according to ordinary C++ rules, that remains a C++ semantic problem.

Tests must distinguish frontend-generated lifetime bugs from user source errors.

Backend diagnostics should never blame the user for invariants broken by generated code.

## Diagnostics and evaluation order

An incorrect lowering that evaluates a `try` operand twice may produce no compiler error at all.

This demonstrates that not every semantic failure is diagnosable through compiler diagnostics.

Some properties must be protected by tests and IR invariants.

Diagnostics complement semantic correctness.

They do not replace it.

## Warnings about ignored outcomes

A future language model may consider diagnostics for situations where a failure-aware result is ignored without propagation or handling.

For example:

```cpp
read_record(id);
```

when `read_record` has a Failure contract.

Whether this should be:

```text
valid
warning
error
```

is not yet specified.

C++ already has `[[nodiscard]]` and library-level conventions related to ignored result values.

VixC should not commit to a warning until the semantic relationship between calls and Failure contracts is fully defined.

## Unreachable code

Because `fail` terminates the current successful execution path, future control-flow analysis may identify source such as:

```cpp
fail Error{};
use_value();
```

as unreachable.

Whether VixC should warn about or reject unreachable code is a broader language-policy question.

The important requirement is that the control-flow model knows the statement after `fail` is not reached through the normal success path.

Diagnostics can then be layered on that semantic fact.

## Missing successful return

Failure-aware functions still have a successful result type.

For example:

```cpp
Value operation() fails Error
{
    if (condition)
        fail Error{};
}
```

If normal C++ semantics require a successful return on the remaining path, VixC must not interpret the existence of a Failure contract as automatically satisfying that requirement.

A future integrated semantic model may be able to explain:

```text
not all successful paths return a Value
```

Whether VixC or the native compiler owns that diagnostic depends on how much declaration and control-flow semantics VixC eventually models.

## Handling diagnostics

The first Failure slice defines production and propagation before dedicated handling syntax.

Once handling exists, diagnostics will need to cover questions such as:

```text
unhandled failure alternative
impossible handler
duplicate handler
handler type mismatch
failure transformed into incompatible type
```

Those diagnostics should be designed alongside the handling semantics rather than added independently.

Future choice and pattern-matching research may influence this area.

## Diagnostics for `none`

The wider Outcome model includes `none`.

Once source-level `none` semantics exist, diagnostics should preserve its distinction from `failure`.

For example, propagating absence through a Failure-only contract should not automatically report an error as though absence were a failure type.

The diagnostic vocabulary must reflect the semantic outcome that actually occurred.

## Diagnostics for `stopped`

Likewise, future cancellation or stopped-computation semantics should not reuse Failure diagnostics unless the language explicitly defines that relationship.

A message such as:

```text
cannot propagate failure
```

would be incorrect if the actual state is cancellation.

The Outcome distinctions must survive into diagnostic terminology.

## Diagnostic determinism

Given the same source, frontend configuration, and VixC version, diagnostics should be deterministic.

Their ordering should not depend on unstable container iteration, thread scheduling, or backend accidents.

This matters for:

- tests
- build logs
- editor integrations
- reproducible development workflows
- automated tooling

The current `DiagnosticEngine` preserves emission order.

Later parallel frontend work must preserve a deterministic user-visible ordering policy.

## Testing diagnostics

Tests should verify more than whether an operation simply failed.

A diagnostic test should eventually verify:

- severity
- stable code
- relevant source range
- primary message meaning
- absence of misleading cascades
- related notes when appropriate

Tests should avoid depending on every punctuation or wording detail unless that text is intended to be stable API.

Diagnostic codes and semantic conditions are better long-term anchors than exact sentence formatting.

## Current semantic diagnostic coverage

The current Failure semantic implementation already detects several important conditions.

It can reject:

```text
fail outside an active Failure context
try outside an active Failure context
malformed Failure semantic structures
invalid source ranges used by Failure analysis
```

The current parser also detects malformed `fail`, `fails`, and `try` structures.

The current lowering and backend layers report invariant failures when incomplete Failure IR reaches them.

This is enough to exercise the diagnostic architecture.

It is not yet the final diagnostic model.

## Missing diagnostic capabilities

Several important diagnostics still require implementation.

The frontend does not yet have enough semantic type information to diagnose:

```text
explicit failure value type mismatch
propagated failure type mismatch
Failure contract declaration mismatch
template-dependent Failure incompatibility
invalid conversion between Failure domains
```

The frontend also lacks complete declaration-level Failure scope, so some currently reported context errors are temporary consequences of incomplete parsing.

These gaps should remain explicit during research.

## Diagnostic milestone

The first meaningful Failure diagnostic milestone is reached when this source:

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

can be analyzed as one failure-aware computation.

Valid Failure operations should produce no semantic diagnostics.

An invalid operation inside the same body should produce a source-level diagnostic connected to the active `LoadError` contract.

For example:

```cpp
User load_user(int id) fails LoadError
{
    fail NetworkError{};
}
```

should eventually produce a direct Failure compatibility diagnostic rather than a generated C++ type error.

## Diagnostic stability criteria

Failure diagnostics should be considered mature when several conditions hold.

VixC-owned semantic mistakes are diagnosed before backend generation.

Messages use source-level Failure terminology.

Source ranges point to the relevant original construct.

Related declarations can be shown through notes or structured secondary ranges.

Generated C++ implementation details do not leak into normal semantic errors.

Native compiler diagnostics can be related back to original VixC source where generated code is involved.

Diagnostic recovery avoids unnecessary cascades.

Codes remain deterministic and suitable for tooling.

Ordinary C++ errors are still allowed to remain ordinary C++ errors when VixC has no additional semantic knowledge.

## Working diagnostic rule

The current diagnostic rule for Failure can be summarized as follows.

If VixC owns the semantic rule, VixC should diagnose its violation.

If VixC only preserves ordinary C++, the native C++ compiler remains responsible for ordinary C++ semantic diagnostics.

If an error originates in generated implementation code, VixC should preserve enough source and semantic provenance to connect that failure back to the source construct that caused the generated code to exist.

Diagnostics should therefore follow the same architecture as the language itself:

```text
source meaning first
representation second
```

That principle is essential if Failure is to become a real language feature rather than a source-rewriting convenience.
