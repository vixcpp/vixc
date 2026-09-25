# Choice and Match Research

This directory contains research for a possible VixC language model around explicit choice, alternative states, and pattern-based selection.

The goal is not to add another spelling for `if`, `switch`, or `std::visit`.

C++ already provides several mechanisms for expressing alternatives:

- `if`
- `switch`
- enums
- `std::variant`
- `std::optional`
- `std::expected`
- inheritance and virtual dispatch
- overloaded visitors
- templates and concepts

The research question is narrower:

> Can VixC provide a coherent language-level way to represent and consume semantic alternatives when ordinary C++ mechanisms become fragmented or lose important information?

This work is still exploratory.

No `choice` or `match` syntax should be considered stable.

## Motivation

Many programs operate on values that can be in one of several meaningful states.

For example:

```text
authenticated(user)
anonymous
rejected(reason)
```

or:

```text
success(value)
none
failure(error)
stopped
```

C++ can represent these states through types such as:

```cpp
std::variant<
    Authenticated,
    Anonymous,
    Rejected>
```

The representation is possible.

The larger question is whether the language can reason directly about those alternatives.

A useful model may need to answer:

- what alternatives exist
- whether they are complete
- whether one alternative is impossible
- whether all alternatives are handled
- whether a branch binds associated data
- whether a branch changes control flow
- whether nested alternatives compose coherently

These are semantic questions rather than storage questions.

## Relationship with Failure and Outcome

Choice and Match research is closely related to the Failure and Outcome work.

The current Outcome hypothesis distinguishes:

```text
success
none
failure
stopped
```

A future program may need to inspect those states explicitly rather than only propagate them.

For example, a computation may choose to recover from one failure instead of propagating it.

Conceptually:

```text
match result

success(value):
    use value

failure(error):
    recover from error
```

The exact syntax is not defined.

Choice and Match research should provide a general model that can potentially handle Outcome states without becoming specific only to Failure.

## Representation and matching are separate questions

There are two distinct problems.

The first is how a value represents alternatives.

The second is how code selects behavior based on the active alternative.

C++ already has several representation mechanisms.

For example:

```cpp
std::variant<A, B, C>
```

represents one active alternative.

An enum can represent a finite set of states.

A class hierarchy can represent alternatives through runtime polymorphism.

A result type can represent success or failure.

VixC should not invent a new representation merely because it is researching matching.

A language-level `match` operation could potentially work with existing C++ representations.

Whether VixC also needs its own `choice` declaration model is a separate question.

## The `choice` hypothesis

One possible direction is an explicit language concept for a closed set of alternatives.

Conceptually:

```text
choice ConnectionState
    connected(Connection)
    disconnected
    failed(NetworkError)
```

This is only semantic notation.

It is not proposed final syntax.

Such a declaration could mean that exactly one alternative is active at a time.

Each alternative could optionally carry data.

For example:

```text
connected(Connection)
```

carries a `Connection`.

```text
disconnected
```

does not require associated data.

```text
failed(NetworkError)
```

carries a failure value.

The important property would be that the frontend understands the complete set of alternatives.

## Why a closed set matters

A closed alternative set enables reasoning that is difficult when cases are distributed across arbitrary runtime conditions.

Suppose a value can be exactly one of:

```text
pending
ready(Value)
failed(Error)
```

A frontend that knows those states can determine whether a selection handles all possible alternatives.

For example:

```text
match state
    pending:
        ...

    ready(value):
        ...

    failed(error):
        ...
```

could be exhaustive.

If one branch is omitted, the frontend could know exactly which semantic alternative is missing.

This differs from an arbitrary chain of boolean conditions.

## The `match` hypothesis

A possible `match` construct would select behavior based on a known semantic alternative.

Conceptually:

```text
match state

ready(value):
    consume(value)

failed(error):
    report(error)

pending:
    wait()
```

Again, this is not final syntax.

The semantic requirements matter more than spelling.

A Match operation should potentially understand:

- the type being matched
- its possible alternatives
- data associated with each alternative
- whether cases overlap
- whether all required alternatives are handled
- whether some alternatives are unreachable

## Exhaustiveness

One of the strongest possible reasons for language-level matching is exhaustiveness checking.

Suppose:

```text
State =
    pending
    ready(Value)
    failed(Error)
```

and source handles only:

```text
pending
ready
```

A frontend could report that:

```text
failed
```

is not handled.

This is stronger than relying on a default branch that silently absorbs future states.

Exhaustiveness can make program evolution safer when new alternatives are introduced.

However, VixC must determine when exhaustiveness should be required and when partial matching is useful.

## Partial matching

Not every selection needs to handle every possible state.

For example, code may only care whether a value is in one specific state.

Conceptually:

```text
if state matches ready(value):
    consume(value)
```

A future language model may distinguish:

```text
exhaustive matching
partial matching
conditional destructuring
```

These should not necessarily use the same construct.

The research should avoid turning one `match` operation into a universal control-flow mechanism before the use cases are understood.

## Associated values

Alternatives often carry data.

For example:

```text
ready(Value)
failed(Error)
```

Matching should make those values available naturally.

Conceptually:

```text
ready(value):
    consume(value)
```

The binding `value` should have a semantic type derived from the selected alternative.

The frontend must eventually preserve:

- reference behavior
- value category
- constness
- ownership
- move semantics
- lifetime

A matching implementation must not introduce arbitrary copies.

## Alternatives without values

Some states carry no additional data.

For example:

```text
pending
cancelled
closed
```

These should not require artificial payload types.

A choice model must support both:

```text
alternative
```

and:

```text
alternative(T)
```

without making one form a special case in generated code semantics.

## Relationship with `enum`

C++ enums already represent finite named alternatives.

For example:

```cpp
enum class State
{
    pending,
    ready,
    failed
};
```

A VixC Match model should ideally work naturally with ordinary enums where no associated data is required.

It would be unnecessary to force programmers to rewrite every enum as a VixC-specific Choice merely to use structured matching.

This raises an interoperability question:

> Should `match` operate directly on ordinary C++ enums?

The likely direction is to investigate that before inventing a separate enum replacement.

## Relationship with `std::variant`

`std::variant` already represents one active value from a finite list of types.

For example:

```cpp
std::variant<
    Pending,
    Ready,
    Failed>
```

C++ programs commonly inspect it using:

```cpp
std::visit
```

or:

```cpp
std::get_if
```

A VixC Match operation could potentially provide language-level selection over `std::variant`.

That would preserve existing C++ representation while adding semantic matching.

The research must determine whether this can be done without coupling the language specifically to `std::variant`.

## Relationship with `std::optional`

An optional value has two semantic alternatives:

```text
some(T)
none
```

A future Match operation could potentially express:

```text
some(value)
none
```

over:

```cpp
std::optional<T>
```

However, VixC should not assume that every two-state type is semantically optional.

Representation must not replace meaning.

An adapter or recognized semantic relationship may be required.

## Relationship with `std::expected`

An expected value represents:

```text
value(T)
error(E)
```

A Match construct could potentially operate over it.

This is particularly relevant to Failure interoperability.

For example, ordinary C++:

```cpp
std::expected<User, LoadError>
```

already contains two alternatives.

VixC could theoretically match them without requiring the source to use VixC Failure declarations.

Whether this should happen automatically remains open.

## Relationship with inheritance

Runtime polymorphism can also represent alternatives.

For example:

```cpp
struct Event
{
    virtual ~Event() = default;
};

struct Connected : Event
{
};

struct Disconnected : Event
{
};
```

A pattern-matching system could theoretically select based on dynamic type.

That would introduce questions involving:

- RTTI
- virtual inheritance
- open class hierarchies
- third-party extensions

This is fundamentally different from matching a closed variant.

VixC should not assume one model can provide safe exhaustiveness over both closed and open hierarchies.

## Closed and open alternatives

A Choice model may need to distinguish between closed and open sets.

A closed set means the frontend knows every legal alternative.

For example:

```text
pending
ready
failed
```

An open set may allow future or external alternatives.

Closed sets enable strong exhaustiveness checking.

Open sets improve extensibility.

These properties conflict.

The language should not claim exhaustive matching over a model that permits unknown future alternatives.

## Default branches

C++ `switch` commonly uses:

```cpp
default:
```

A Match construct may also need a catch-all branch.

However, a catch-all can hide missing alternatives when the set evolves.

For example, adding:

```text
cancelled
```

to a Choice may silently enter the existing default branch.

VixC should investigate whether explicit exhaustive matching should discourage catch-all branches in some contexts.

The correct policy may depend on whether the programmer wants future alternatives to produce diagnostics.

## Pattern matching and values

Matching can range from simple alternative selection to deep structural destructuring.

A simple model may support:

```text
ready(value)
```

A richer model could theoretically support nested patterns such as:

```text
response(success(user(name, id)))
```

That rapidly expands the language surface.

The first research should remain focused on alternative selection and direct payload binding.

Deep structural patterns should only be introduced if real programs demonstrate a need.

## Guards

Some languages allow additional conditions on a pattern.

Conceptually:

```text
ready(value) if value.valid():
    ...
```

Guards can be useful.

They also complicate exhaustiveness because a syntactically present pattern may not match every value belonging to that alternative.

The first Match model should not assume guards are necessary.

Ordinary C++ conditions inside the branch may be sufficient initially.

## Match as statement or expression

A major open question is whether Match produces a value.

Statement form:

```text
match state
    ready(value):
        use(value)

    failed(error):
        report(error)
```

Expression form:

```text
auto message =
    match state
        ready(value):
            format(value)

        failed(error):
            format(error)
```

Expression Match provides powerful composition but requires a common result type and stronger control-flow semantics.

The first implementation may benefit from a statement-oriented subset.

The final decision should be driven by actual use cases.

## Match result typing

If Match is an expression, every reachable branch must contribute to a coherent result type.

For example:

```text
match state

ready(value):
    -> User

failed(error):
    -> ErrorMessage
```

raises a type question.

Possible models include:

- exact same type required
- ordinary C++ common type
- explicit conversion
- variant result
- no expression Match when types differ

This requires real semantic type information.

It should not be determined by textual source rewriting.

## Control flow inside Match

Branches may contain operations such as:

```cpp
return
```

```cpp
fail
```

```cpp
break
```

or future stopped/cancellation operations.

The frontend must understand how these affect branch completion.

For example, an expression Match could potentially have one branch that returns from the function and another that produces a value.

This requires control-flow reasoning beyond simple source substitution.

## Match and Failure handling

One important research direction is handling Failure through Match-like semantics.

Conceptually:

```text
match operation()

success(value):
    use(value)

failure(error):
    recover(error)
```

This could provide the missing explicit handling boundary in the Failure model.

However, Failure is currently represented as a computation outcome rather than necessarily as an ordinary source-level value.

The frontend must determine how an outcome becomes matchable.

Possible models include:

- explicit materialization of an Outcome value
- a dedicated handling expression
- Match directly over a computation
- another construct

This should be resolved together with Failure semantics.

## Match and `none`

The wider Outcome model includes:

```text
none
```

Choice and Match could provide a natural way to handle absence.

Conceptually:

```text
match lookup()

success(value):
    use(value)

none:
    use_default()
```

If Failure is also possible:

```text
success(value)
none
failure(error)
```

matching could make all three distinctions explicit.

This is one reason the Choice and Match work is related to Outcome research.

## Match and `stopped`

Future cancellation research may introduce:

```text
stopped
```

A matching system could potentially handle it independently:

```text
success(value)
failure(error)
stopped
```

This would preserve the semantic distinction rather than treating cancellation as generic Failure.

The current research does not define the exact syntax.

## Destructuring

A Choice alternative may carry structured data.

For example:

```text
connected(Connection)
```

The simplest Match form binds the complete `Connection`.

A deeper pattern model could destructure its fields.

For example, conceptually:

```text
connected(connection):
```

versus:

```text
connected({socket, address}):
```

The second form requires VixC to understand more about C++ object structure.

This should not be added unless it provides enough value to justify the semantic and parsing complexity.

## Ownership and Match

Matching must preserve C++ ownership behavior.

Consider a Choice carrying:

```cpp
std::unique_ptr<Resource>
```

A Match branch may:

- inspect it by reference
- move it out
- leave it untouched

The language needs clear rules about what matching itself does.

Pattern selection should not silently move values merely to determine the active alternative.

Future ownership research may provide stronger tools here.

## References

Associated data may be exposed by reference.

For example, matching an lvalue should not necessarily copy its payload.

The model may need behavior analogous to normal C++ value categories.

Questions include:

- matching an lvalue
- matching a const lvalue
- matching an rvalue
- binding by reference
- binding by value
- moving from a matched value

These semantics must eventually be explicit.

## Temporary lifetime

Consider matching a temporary:

```text
match make_state()
```

Any bound payload must remain valid throughout the branch where it is used.

The backend must preserve the temporary lifetime required by the language semantics.

Naive rewriting may create dangling references.

## Exhaustiveness and unreachable cases

A frontend that knows the complete alternative set can potentially detect both missing and impossible branches.

For example, if a value has only:

```text
ready
failed
```

then:

```text
pending:
```

may be unreachable or invalid.

Likewise, duplicate alternatives should be detectable.

This is one of the potential advantages of direct semantic representation.

## Nested Match

Matching may occur inside matching branches.

For example:

```text
match response

success(result):
    match result
        value(v):
            ...
        none:
            ...

failure(error):
    ...
```

The semantic model should compose without requiring special treatment for nesting.

The implementation should avoid global match state.

## Matching and ordinary `switch`

VixC should not replace `switch` where `switch` already expresses the program clearly.

For example:

```cpp
switch (state)
{
    case State::ready:
        break;

    case State::failed:
        break;
}
```

is ordinary C++ and should remain valid.

A new Match feature must justify itself through capabilities not provided coherently enough by existing `switch`.

Potential differences include:

- associated value binding
- exhaustiveness
- type-safe alternatives
- integration with Outcome
- non-integral alternatives

If those advantages are not meaningful in practice, adding Match would not be justified.

## Matching enums

A useful early experiment may involve ordinary C++ enums.

For example:

```cpp
enum class State
{
    pending,
    ready,
    failed
};
```

A Match frontend could potentially verify that every enumerator is handled.

This would test exhaustiveness without requiring a new Choice declaration syntax.

However, C++ enums can interact with casts and values outside named enumerators.

The exact semantic guarantee must be understood before claiming perfect closed-set exhaustiveness.

## Matching `std::variant`

Another useful experiment may involve:

```cpp
std::variant<A, B, C>
```

The set of alternatives is explicitly known by the type.

This provides a strong closed-set model.

VixC could potentially construct semantic alternatives from the variant type and compare a language-level Match operation against `std::visit`.

The experiment should measure whether Match materially improves:

- readability
- diagnostics
- exhaustiveness
- generated code
- tooling

## Choice declaration necessity

A major open question is whether VixC needs a `choice` declaration at all.

It may be possible for Match to operate entirely over existing C++ types such as:

- enums
- variants
- optionals
- expected-like types
- VixC Outcome

If that is sufficient, adding a new sum-type declaration syntax may unnecessarily duplicate C++.

A VixC Choice construct should only be introduced if it provides semantic capabilities that existing C++ type composition cannot provide coherently.

## Choice versus `std::variant`

A hypothetical Choice declaration may offer:

- named alternatives
- direct payload association
- stronger exhaustiveness metadata
- simpler diagnostics
- less visitor boilerplate

But `std::variant` already provides:

- closed alternatives
- value semantics
- standard-library integration
- mature implementation
- generic programming support

The research must determine whether syntax and semantic integration justify another type construct.

## Representation independence

If Choice becomes a language feature, its semantic model should not be defined as:

```text
Choice = std::variant
```

A backend may choose `std::variant` as a representation.

It may also choose another tagged representation.

The frontend should represent:

```text
alternative set
active alternative
payload type
```

independently from storage layout.

This follows the same principle used in Failure research.

## ABI

A new Choice type would have ABI implications.

Questions include:

- layout
- alignment
- tag representation
- destructor behavior
- copy/move operations
- exception guarantees
- interop with ordinary C++
- public API stability

This is another reason to prefer experimentation over prematurely adding a new type system feature.

Using existing `std::variant` avoids some language design work but still inherits standard-library ABI considerations.

## Templates

Choice and Match must eventually interact with templates.

Examples include:

```cpp
template <typename T>
void process(T value);
```

where `T` may be a matchable type.

Questions include:

- Can concepts constrain a type to be matchable?
- Can alternatives depend on template parameters?
- Can Match appear in templates before alternatives are known?
- When is exhaustiveness checked?
- How do dependent patterns work?

A mature design cannot depend only on source spelling.

## Concepts

C++ concepts may offer a way to describe matchable protocols.

For example, a future adapter could expose:

```text
set of alternatives
how to inspect active alternative
how to access payload
```

Whether such a protocol should be language-defined or library-defined is open.

The first experiments should avoid designing a large customization system before basic matching proves useful.

## Generic Match

A powerful but complex direction would allow Match over any type that exposes an alternative protocol.

This could support third-party result and variant types.

The cost is that exhaustiveness and payload semantics become dependent on customization metadata.

A smaller built-in set of supported types may be easier initially.

The correct extensibility model remains open.

## Diagnostics

Choice and Match could justify language support partly through diagnostics.

Potential diagnostics include:

```text
missing alternative
duplicate alternative
unknown alternative
unreachable alternative
payload binding type mismatch
non-matchable value
non-exhaustive Match expression
incompatible branch result types
```

These should reference source-level alternatives rather than generated C++ visitor machinery.

## Missing alternative

Given a closed set:

```text
pending
ready
failed
```

and a Match handling:

```text
pending
ready
```

the frontend could report:

```text
match does not handle alternative 'failed'
```

The diagnostic should ideally point to both:

- the Match construct
- the declaration of the missing alternative

This requires semantic provenance.

## Duplicate alternative

Given:

```text
ready(value):
    ...

ready(other):
    ...
```

the second case may be invalid because the first already covers the complete `ready` alternative.

The frontend should diagnose this before backend generation.

## Unreachable alternative

A case can be unreachable if a previous pattern already covers it.

This becomes more complex once guards or nested patterns exist.

The first Match model should keep patterns simple enough that reachability remains clear.

## Backend lowering

A Match construct may lower naturally into:

- `switch`
- `if`
- `std::visit`
- tag dispatch
- another native representation

The semantic IR should not commit to one of these too early.

Lowering should preserve:

- evaluation once
- branch selection
- payload binding
- exhaustiveness assumptions
- branch control flow
- source provenance

The backend can then choose appropriate C++.

## Evaluate once

Matching an expression must not evaluate it repeatedly.

For:

```text
match read_state()
```

the operation:

```cpp
read_state()
```

must execute once.

A backend cannot generate separate condition checks that each reevaluate the expression.

This is the same general lowering requirement already present in Failure propagation.

## Generated branch representation

If Match operates over an enum, a C++ backend may naturally use:

```cpp
switch
```

If it operates over `std::variant`, it may use:

```cpp
std::visit
```

or index-based branching.

If it operates over a generated Choice type, another representation may be appropriate.

The language meaning should remain stable across these strategies.

## Source provenance

Each generated branch should retain a relationship to its original Match case.

This matters when generated code produces native compiler diagnostics.

If a generated visitor body fails to compile because of ordinary user C++ inside one Match case, tooling should be able to relate the error back to that original branch.

## Match and ordinary C++ errors

The body of a Match case can contain ordinary C++.

For example:

```text
ready(value):
    unknown_function(value)
```

The failure to resolve `unknown_function` is an ordinary C++ semantic problem.

VixC does not need to duplicate the entire C++ type checker.

The source map should allow the native compiler diagnostic to point back to the original case body.

## Parser requirements

Match syntax introduces substantial parser requirements.

The parser must identify:

- the matched expression
- case boundaries
- alternative names
- optional payload bindings
- branch bodies
- optional catch-all behavior

This is significantly more structure than the first Failure tokens.

The syntax should therefore not be frozen before a parser prototype demonstrates that it coexists cleanly with real C++.

## Contextual syntax

Any new keywords should be evaluated for C++ compatibility.

Words such as:

```text
choice
match
case
when
```

may already appear as identifiers in existing C++ code.

Contextual keyword parsing may therefore be preferable to globally reserving new words.

The current Failure experiment already demonstrates why unconditional keyword introduction can create compatibility concerns.

Choice and Match should learn from that work.

## Preprocessor interaction

C++ preprocessing can split or generate syntax.

Questions include:

- Can Match cases be generated by macros?
- Can a macro expand to an alternative name?
- Can conditional compilation remove one branch?
- Is exhaustiveness checked before or after preprocessing?
- How are source locations represented through macro expansion?

The first lightweight VixC parser does not yet have a complete preprocessor model.

This constrains how ambitious early Match syntax can be.

## Relationship with a complete C++ frontend

Rich pattern matching requires increasingly detailed knowledge of C++ types and expressions.

This may increase pressure for deeper integration with a mature C++ frontend.

Possible directions include:

- progressively expanding VixC's parser
- using Clang semantic information
- restricting Match to structures VixC can understand safely
- using explicit adapters

The research should identify the smallest architecture that can provide correct semantics.

## Control-flow IR

Match may become another reason for a more explicit VixC control-flow IR.

For example:

```text
Evaluate value

BranchOnChoice value
    alternative A:
        ...
    alternative B:
        ...
    alternative C:
        ...
```

This representation could compose naturally with Failure propagation.

For example, a branch may itself:

```text
PropagateFailure
```

or produce another value.

The exact IR design remains open.

## Relationship with future ownership semantics

Pattern binding interacts strongly with ownership.

A Match operation may need to distinguish between:

```text
borrow payload
copy payload
move payload
```

The first implementation should preserve ordinary C++ semantics rather than invent an implicit ownership model.

Later ownership research may make those operations more explicit.

## Relationship with async and cancellation

Choice and Match may become useful when consuming asynchronous Outcome values.

For example:

```text
success(value)
failure(error)
stopped
```

can be handled as distinct states.

This provides one possible bridge between Failure, async, and cancellation research.

The Match feature should therefore remain general enough that it is not permanently tied only to enums or variants.

## Relationship with compile-time evaluation

Pattern matching can also be useful during compile-time computation.

However, the current research is runtime-oriented.

A future compile-time model may reuse the same semantic Match representation if alternatives are known statically.

No special compile-time syntax should be added during the first experiment.

## Runtime requirements

Choice and Match should not require a dedicated VixC runtime unless the semantics demonstrate a need.

Closed alternatives can normally be represented through ordinary native C++.

Matching can normally lower into ordinary control flow.

A runtime dependency should therefore not be introduced simply because Match is a language feature.

## Initial experimental targets

The first useful experiments should remain smaller than designing a complete Choice type system.

Potential starting points include:

1. exhaustive Match over a C++ enum
2. exhaustive Match over `std::variant`
3. payload binding without deep destructuring
4. statement-oriented Match
5. deterministic C++ lowering
6. precise missing-case diagnostics
7. source mapping for generated branches

These experiments can determine whether direct language support provides meaningful benefits.

Only after that should a dedicated `choice` declaration be considered.

## Questions to answer

The research needs answers to at least the following questions.

### Semantic questions

What exactly makes a type matchable?

What constitutes a closed alternative set?

Is Match always exhaustive?

Can partial matching use the same syntax?

How are payloads bound?

What happens when a branch does not complete normally?

Can Match produce a value?

How are branch result types combined?

### Type questions

Does Match operate directly on enums?

Does it operate directly on `std::variant`?

Does it operate on `std::optional` and `std::expected`?

Can third-party types participate?

Does VixC need a dedicated Choice type?

How are reference and move semantics preserved?

### Syntax questions

What keyword, if any, should introduce Match?

Should new words be contextual keywords?

What does a case look like?

How are payload bindings written?

Is a default branch allowed?

How is expression Match distinguished from statement Match?

### Exhaustiveness questions

When is exhaustive handling required?

Does a catch-all branch suppress future missing-case diagnostics?

Can the programmer explicitly request non-exhaustive matching?

How are enums with unnamed underlying values handled?

How are open class hierarchies handled?

### Lowering questions

How is the matched expression evaluated exactly once?

How are temporary lifetimes preserved?

How are payload references bound safely?

How does Match interact with Failure propagation?

Should Match lower into generic control-flow IR?

### Interoperability questions

How does Match work with existing C++ types?

Can ordinary C++ call APIs using a future Choice type?

Can VixC Match operate on third-party variant implementations?

How does matching work across translation units?

### Tooling questions

Can editors show missing alternatives?

Can refactoring update Match sites when a Choice changes?

Can a language server navigate from a case to its alternative declaration?

Can source maps keep generated visitor diagnostics understandable?

## What would justify Match

A Match feature would be justified if experiments demonstrate significant improvements such as:

- reliable exhaustiveness checking
- clearer handling of associated values
- simpler source than visitor boilerplate
- better diagnostics
- explicit semantic connection to Outcome
- stronger tooling information
- correct native lowering without substantial overhead

Syntax reduction alone would not be sufficient justification.

## What would argue against Match

The feature should be reconsidered if:

- safe integration requires too much C++ parsing complexity
- syntax conflicts significantly with existing C++
- `std::visit` and ordinary control flow already provide equivalent clarity
- interoperability requires pervasive adapters
- exhaustiveness cannot be defined reliably
- generated code introduces substantial compile-time overhead
- value-category and lifetime behavior become difficult to explain
- real programs do not become more coherent

The research should permit this outcome.

## What must not become accidental specification

The following should remain provisional until tested:

- the words `choice` and `match`
- whether Choice is a new type declaration
- whether Match is an expression
- whether Match is always exhaustive
- whether Match operates automatically on `std::variant`
- whether Match handles Outcome directly
- how payload bindings are written
- whether default branches exist
- whether alternatives are nominal or type-based
- how branch result types are determined

Prototype syntax is not specification.

## Relationship with the current frontend

Choice and Match should reuse the existing VixC architecture rather than introduce a separate processing path.

If implemented, the feature should pass through:

- source management
- lexing
- syntax
- semantic analysis
- semantic IR
- lowering
- backend generation
- source mapping
- diagnostics

The frontend should understand the semantic alternatives before the C++ backend chooses a concrete representation.

This is the same architectural principle being tested by Failure.

## Research priority

Choice and Match should not interrupt completion of the first Failure vertical slice.

Failure currently has a concrete missing declaration-level semantic and lowering path.

Choice and Match remain a later research area.

The purpose of this directory is to record the problem space early enough that Failure handling and Outcome design do not accidentally make future matching impossible.

The immediate objective is therefore not implementation.

It is architectural compatibility.

## Current position

The current hypothesis is that VixC may eventually benefit from a language-level selection mechanism over known semantic alternatives.

Such a mechanism could potentially improve:

- Outcome handling
- variant handling
- exhaustiveness
- diagnostics
- payload binding
- control-flow reasoning

It is not yet established that VixC needs a new Choice type.

It is not yet established that the word `match` should become source syntax.

The next meaningful evidence should come from experiments using existing C++ alternative representations before VixC introduces another permanent language construct.
