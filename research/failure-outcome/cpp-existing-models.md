# Existing C++ Failure Models

C++ does not have one universal model for recoverable failure.

Instead, C++ programs represent unsuccessful computation through several language features, library types, conventions, and application-specific abstractions.

This is not necessarily a defect.

Different mechanisms solve different problems.

The purpose of this document is to examine the existing models before VixC commits to its own Failure semantics.

The question is not:

> Which existing C++ mechanism is bad?

The useful question is:

> Which semantic properties already exist in C++, which properties are missing, and which parts would VixC actually need to own?

The first VixC Failure model should reuse existing C++ semantics wherever possible rather than creating a parallel ecosystem without necessity.

## Error codes

One of the oldest and most portable C++ failure conventions is returning an error code.

A function may return an integer:

```cpp
int open_connection();
```

with an application-specific convention such as:

```cpp
0   // success
-1  // failure
```

or an enumeration:

```cpp
enum class LoadError
{
    none,
    not_found,
    permission_denied,
    io_error
};

LoadError load_config(Config &config);
```

The successful value may be written through an output parameter:

```cpp
LoadError load_config(Config &config);
```

A caller then handles the result explicitly:

```cpp
Config config;

const auto error =
    load_config(config);

if (error != LoadError::none)
{
    // handle failure
}
```

This model has useful properties.

Failure is visible in ordinary control flow.

No exception machinery is required.

The representation can be ABI-friendly and inexpensive.

The caller can decide exactly when and how an error is propagated.

The weaknesses are mostly semantic and compositional.

The return type may represent the error while the actual successful result is moved elsewhere.

The language itself does not distinguish a return value that represents success from one that represents failure.

Nothing requires callers to inspect the returned status unless additional mechanisms such as `[[nodiscard]]` are used.

Propagation is repetitive.

For example:

```cpp
auto error =
    first_operation();

if (error != Error::none)
    return error;

error =
    second_operation();

if (error != Error::none)
    return error;
```

The program repeatedly expresses the same semantic operation manually.

Error codes therefore provide a representation of failure, but not a dedicated language-level propagation model.

## `errno`

C and C++ APIs may communicate failure through a sentinel result combined with `errno`.

A simplified pattern is:

```cpp
const auto value =
    some_system_call();

if (value == -1)
{
    const int error =
        errno;
}
```

This model separates the ordinary function result from additional error information stored outside that result.

It remains important because many operating-system interfaces use this style.

For VixC, however, `errno` should be considered an interoperability case rather than a candidate language foundation.

Its semantics depend on external mutable state and API-specific sentinel values.

The caller must already know which return values indicate failure and when `errno` is meaningful.

VixC should be able to interact with such APIs without adopting this representation as its own Failure model.

## Sentinel values

Another common technique is reserving part of the ordinary result domain for failure.

For example:

```cpp
User *find_user(int id);
```

may use:

```cpp
nullptr
```

to represent absence or failure.

An integer function may use:

```cpp
-1
```

as a special value.

A string API may use an empty string.

This can be efficient and simple when the meaning is obvious.

The primary problem is semantic ambiguity.

For example:

```cpp
nullptr
```

might mean:

- no object exists
- allocation failed
- lookup failed
- the caller supplied invalid input
- the operation was cancelled

The type alone does not necessarily explain which interpretation is correct.

A sentinel also consumes part of the result domain unless the type already has a natural empty state.

For VixC, this model reinforces the need to distinguish representation from meaning.

An existing C++ API may use a sentinel internally while VixC still needs to know whether that sentinel means `none`, `failure`, or something else.

## Boolean success indicators

Some APIs return only whether an operation succeeded:

```cpp
bool save_file(const File &file);
```

The caller may write:

```cpp
if (!save_file(file))
{
    // operation failed
}
```

This model is compact but loses failure information.

The caller knows that something failed but not necessarily why.

It is suitable when no detailed recovery decision is required.

It is insufficient as a general VixC Failure model because VixC's first hypothesis includes typed recoverable failure:

```cpp
fails LoadError
```

The failure value is semantically relevant.

## Output parameters

C++ APIs sometimes combine a status result with one or more output parameters.

For example:

```cpp
Error parse(
    std::string_view input,
    Ast &output);
```

This avoids wrapping the successful result and failure state in a single return type.

It can also work well with existing C APIs.

However, it changes the shape of expressions.

Instead of:

```cpp
auto ast =
    parse(input);
```

the caller writes:

```cpp
Ast ast;

const auto error =
    parse(
        input,
        ast);
```

This matters for a proposed VixC propagation form such as:

```cpp
auto ast =
    try parse(input);
```

That expression naturally assumes that success and failure belong to one computation result that can be inspected and propagated.

Output-parameter APIs therefore require adaptation if they are to participate in that model.

## Exceptions

C++ has language-level exception handling.

A function can signal exceptional control flow with:

```cpp
throw error;
```

A caller can handle it with:

```cpp
try
{
    operation();
}
catch (const Error &error)
{
    // handle
}
```

Exceptions already provide several capabilities that are relevant to VixC research.

They provide non-local propagation.

They integrate with stack unwinding.

Automatic objects are destroyed as control leaves their scopes.

The propagation behavior is implemented by the language and runtime rather than manually repeated at every call boundary.

This makes exceptions the closest existing C++ feature to a language-level failure propagation mechanism.

However, the semantic model differs from the current VixC Failure hypothesis.

A declaration such as:

```cpp
User load_user() fails LoadError;
```

intends to make recoverable failure part of the visible computation contract.

Normal modern C++ exception specifications do not provide the same typed declaration contract.

A function can throw values that are not directly represented in its return type.

Callers do not necessarily know the complete set of exceptions from the function type alone.

Exceptions therefore provide propagation semantics without necessarily providing the explicit outcome contract being investigated by VixC.

## Exception propagation

Exception propagation also differs from value-style propagation.

When:

```cpp
operation();
```

throws, normal execution leaves the current expression and searches for an applicable handler.

By contrast, the proposed VixC operation:

```cpp
auto value =
    try operation();
```

expresses propagation explicitly at the call site.

This difference matters.

An explicit propagation operator exposes a control-flow boundary in source.

Exception propagation can cross multiple call frames without each call site containing an explicit propagation marker.

Neither model is automatically superior.

They make different tradeoffs concerning locality, verbosity, visibility, and abstraction.

VixC research should therefore avoid assuming that `try` propagation is simply exception handling under a different spelling.

## Stack unwinding and RAII

Exceptions have an important property that any VixC Failure lowering must respect: C++ object lifetime semantics.

Consider:

```cpp
Resource resource;

operation();
```

If `operation()` throws, normal stack unwinding destroys `resource`.

A value-based failure implementation must also preserve the correct lifetime of automatic objects when propagation leaves a scope.

For example:

```cpp
Resource resource;

auto value =
    try operation();
```

If `operation()` fails and VixC propagates that failure, `resource` must still be destroyed according to normal C++ lifetime rules.

Generated control flow cannot bypass RAII.

This is not only an implementation detail.

Correct interaction with C++ lifetimes is a requirement of the language model.

## `noexcept`

C++ allows a function to declare:

```cpp
void operation() noexcept;
```

If an exception escapes such a function, the program terminates.

A VixC failure contract is conceptually different.

A function such as:

```cpp
Value operation() fails Error;
```

declares a recoverable outcome.

If the backend uses exceptions internally, the relationship with `noexcept` becomes significant.

Questions include:

- Can a failure-aware function also be `noexcept`?
- Would an exception-based backend representation violate that declaration?
- Is VixC recoverable failure considered an exception for ABI purposes?
- Can ordinary exceptions still escape independently of VixC Failure?

These questions are another reason not to define VixC Failure as "exceptions with new syntax" before the semantic model is complete.

## `std::error_code`

The C++ standard library provides `std::error_code` as a type-erased error representation built around numeric values and error categories.

An API may use it through output parameters:

```cpp
std::filesystem::remove(
    path,
    error_code);
```

or as part of an application-specific result structure.

`std::error_code` is valuable for interoperability because it can represent errors originating from different domains while preserving their categories.

Its role is primarily representation.

It does not itself specify:

- whether a computation can fail
- how failure propagates
- whether callers must inspect it
- how it interacts with the successful result
- what source syntax should represent propagation

VixC could potentially use `std::error_code` as a failure type:

```cpp
Data read_file() fails std::error_code;
```

without making `std::error_code` the language model itself.

## `std::system_error`

`std::system_error` connects `std::error_code` with C++ exceptions.

For example, some standard-library APIs provide one overload that reports through `std::error_code` and another that throws `std::system_error`.

This demonstrates an important distinction for VixC research.

The same underlying operational condition can be represented through different control-flow mechanisms.

The error domain and the propagation mechanism are separate design choices.

VixC should preserve that distinction.

A failure type answers what recoverable problem occurred.

Propagation semantics answer how the failure travels through computations.

## `std::optional`

`std::optional<T>` represents either:

```text
T
```

or:

```text
no value
```

For example:

```cpp
std::optional<User>
find_user(int id);
```

can naturally represent a lookup where absence is expected.

A caller may write:

```cpp
auto user =
    find_user(id);

if (!user)
{
    // user does not exist
}
```

`std::optional` is important to the VixC Outcome research because it demonstrates that absence is often semantically different from recoverable failure.

The type has no error payload.

An empty optional does not explain why no value exists.

This can be exactly correct when absence itself is the meaningful result.

It can be insufficient when operational failure must be distinguished.

For example:

```cpp
std::optional<User>
load_user(int id);
```

cannot by itself distinguish:

```text
user does not exist
```

from:

```text
database unavailable
```

unless the application adds another convention.

This is one reason the VixC outcome hypothesis keeps `none` and `failure` separate.

## `std::expected`

`std::expected<T, E>` provides a standard value representation for either:

```text
success(T)
```

or:

```text
failure(E)
```

Conceptually:

```cpp
std::expected<User, LoadError>
load_user(int id);
```

makes the recoverable failure type visible in the function signature.

This is significantly closer to the proposed VixC Failure contract than error codes or exceptions alone.

A caller can inspect the result:

```cpp
auto result =
    load_user(id);

if (!result)
{
    handle(
        result.error());

    return;
}

User user =
    *result;
```

The type expresses both successful and unsuccessful value states.

## `std::expected` and VixC

A possible generated representation for:

```cpp
User load_user(int id) fails LoadError;
```

could eventually involve something conceptually similar to:

```cpp
std::expected<User, LoadError>
```

That does not imply that VixC Failure is semantically identical to `std::expected`.

`std::expected` is a library type.

VixC is investigating language rules around:

- declaration of failure
- explicit failure production
- propagation
- context validity
- diagnostics
- future composition with other outcomes
- tooling

The distinction matters.

The language could theoretically use `std::expected` as one backend representation while retaining richer semantic information before code generation.

Conversely, the backend could later use another representation without changing the source-level contract.

## Manual propagation with `std::expected`

Without language support, propagation remains explicit library-level code.

For example:

```cpp
auto result =
    read_record(id);

if (!result)
{
    return std::unexpected(
        result.error());
}

auto record =
    std::move(*result);
```

This code is not inherently wrong.

The VixC question is whether the repeated pattern represents a sufficiently stable semantic operation that it should be expressed directly:

```cpp
auto record =
    try read_record(id);
```

The value of such syntax would depend on more than brevity.

It would need to provide better semantic analysis, diagnostics, composability, and tooling.

## Monadic operations

Result-like types can provide operations that compose success and failure.

For example, an expected-like type may support operations conceptually equivalent to:

```text
transform
and_then
or_else
```

These operations can express pipelines without manually checking every intermediate state.

A chain may conceptually look like:

```cpp
load()
    .and_then(validate)
    .and_then(transform);
```

This is a useful compositional model.

It differs from imperative propagation syntax such as:

```cpp
auto value =
    try load();

auto checked =
    try validate(value);

return transform(checked);
```

The two styles are not mutually exclusive.

A VixC Failure model should not prevent ordinary result types from exposing functional combinators where they are useful.

Language propagation and library composition solve overlapping but not identical problems.

## Custom result types

Many C++ projects define their own result abstraction.

Conceptually:

```cpp
template <typename T, typename E>
class Result;
```

or:

```cpp
StatusOr<T>
```

or:

```cpp
Outcome<T, E>
```

Such types may support:

- success values
- error values
- propagation helpers
- assertions
- diagnostic metadata
- stack traces
- domain-specific status categories

This demonstrates that the C++ type system is capable of representing typed recoverable failure.

The fragmentation problem appears at the semantic interface.

Different libraries define different:

- method names
- propagation helpers
- conversions
- error accessors
- success tests
- failure composition rules

A VixC language feature should therefore not exist merely to create one more custom result container.

If VixC adds value, it should provide semantics that remain stable independently of the concrete result representation.

## Propagation macros

Many C++ codebases use macros to reduce repetitive result checking.

Conceptually:

```cpp
TRY_ASSIGN(
    value,
    operation());
```

or:

```cpp
RETURN_IF_ERROR(
    operation());
```

Macros can encode efficient and practical propagation patterns.

They are widely applicable because they can generate control flow at the call site.

However, preprocessing has limitations as a language model.

Macros do not naturally provide semantic nodes that the compiler frontend can reason about.

Diagnostics often refer to macro expansion details.

Expression composition can be constrained by macro shape.

Tooling must understand project-specific macro conventions.

Source transformations and refactoring become harder.

The VixC proposal:

```cpp
auto value =
    try operation();
```

investigates whether propagation should become a directly understood language operation rather than a convention reconstructed from macro expansion.

## Variants

`std::variant` can represent multiple possible states explicitly.

For example:

```cpp
std::variant<
    User,
    LoadError,
    NotFound>
load_user(int id);
```

This can encode multiple outcomes.

However, all alternatives are represented at the same type level.

The type itself does not declare that one alternative is successful, another represents absence, and another represents recoverable failure.

Those roles are conventions imposed by the application.

A generic sum type is therefore useful representation machinery but does not automatically provide an outcome model.

VixC may eventually use sum-like representations internally or in generated C++, but the language semantics should retain the distinction between the roles of the alternatives.

## Tagged unions and application-specific outcomes

Applications may define explicit tagged unions:

```cpp
struct LoadResult
{
    enum class Kind
    {
        success,
        not_found,
        error
    };

    Kind kind;
    // ...
};
```

This can make semantics considerably clearer than generic sentinel values.

It also demonstrates that many of the distinctions VixC is investigating can already be encoded manually in C++.

The research question is therefore not one of raw expressiveness.

C++ is expressive enough to encode many of these models.

The question is whether repeated domain-independent semantic patterns deserve direct frontend support.

## Callbacks

Asynchronous or event-driven C++ APIs often report results through callbacks:

```cpp
load_user(
    id,
    [](Result<User, Error> result)
    {
        // ...
    });
```

Another API may split success and failure callbacks:

```cpp
load_user(
    id,
    on_success,
    on_error);
```

The failure model then becomes coupled to asynchronous control flow.

This demonstrates a broader issue for VixC.

Failure semantics must eventually compose with execution semantics.

A model that works only for synchronous function returns may become difficult to extend to asynchronous operations.

This is one reason VixC keeps async and cancellation as separate research areas rather than prematurely encoding them into the first Failure implementation.

## Futures and promises

Libraries may represent asynchronous results through future-like objects.

Conceptually:

```cpp
Future<User>
load_user(int id);
```

Failure may then be stored inside the eventual result or propagated through exception mechanisms associated with the future.

Different libraries make different choices.

A future VixC async model may need to answer whether:

```cpp
fails E
```

describes the creation of the asynchronous operation, the eventual completion of that operation, or both.

The current synchronous Failure work should avoid assumptions that make this question impossible to answer later.

## Coroutines

C++ coroutines provide language support for suspendable computations but deliberately leave many semantic decisions to coroutine promise types and libraries.

A coroutine's behavior can depend on methods such as:

```cpp
initial_suspend()
final_suspend()
return_value()
unhandled_exception()
```

This provides significant flexibility.

It also means C++ does not define one universal coroutine error model.

Failure may be represented through:

- exceptions
- expected-like values
- promise state
- application-specific channels

VixC should therefore treat coroutine interaction as an integration problem rather than assuming one existing model.

Any future Failure plus async design must preserve normal coroutine semantics and ownership rules.

## Contracts and assertions

Assertions represent another form of abnormal execution:

```cpp
assert(pointer != nullptr);
```

Application code may also define contract-like macros or invariant checks.

These are semantically different from recoverable failure.

For example:

```cpp
if (!connection)
    fail NetworkError{};
```

may represent an expected operational condition.

By contrast:

```cpp
assert(index < values.size());
```

may represent a programmer assumption that should never be violated during correct execution.

This distinction supports the VixC hypothesis that programmer errors should not automatically become ordinary `failure` outcomes.

## Termination

C++ provides several ways to terminate execution, directly or indirectly.

Examples include:

```cpp
std::terminate();
std::abort();
```

Fatal process termination is not recoverable failure.

It does not return a failure value to the caller.

It ends ordinary program execution.

VixC should therefore avoid using termination as the semantic definition of `failure`, even though a particular unrecoverable condition may eventually lead an application to terminate.

Recovery policy belongs above the basic semantic distinction.

## Static error reporting

C++ also has failures that occur before runtime.

Examples include:

```cpp
static_assert(...);
```

constraint failures, substitution failures, parse errors, and type errors.

These are compiler diagnostics rather than runtime computation outcomes.

VixC Failure should not mix compile-time invalidity with runtime recoverable failure merely because both can be described informally as "errors."

The distinction between:

```text
the program is invalid
```

and:

```text
the program is valid and this operation may fail
```

is fundamental.

## Exceptions versus result values

The two broadest modern C++ approaches to recoverable failure are exception-style propagation and result-value propagation.

Exceptions move failure through control flow implicitly until a handler is found.

Result values make failure part of an explicit value representation and generally require callers to inspect or propagate that value.

A simplified comparison is:

| Property                                   | Exceptions          | Result values            |
| ------------------------------------------ | ------------------- | ------------------------ |
| Failure carried as ordinary return value   | No                  | Yes                      |
| Automatic non-local propagation            | Yes                 | No                       |
| Failure visible in ordinary return type    | Usually no          | Yes                      |
| Explicit propagation at each call site     | No                  | Usually yes              |
| Stack unwinding                            | Built into language | Uses normal control flow |
| Error type can be part of result signature | Not normally        | Yes                      |
| Works without exception runtime semantics  | No                  | Yes                      |
| Easy integration with value composition    | Indirect            | Yes                      |

This table is descriptive rather than a judgment about which model should be preferred.

Each model has contexts in which it is useful.

VixC is investigating whether it can combine explicit failure contracts with concise propagation while preserving normal C++ value and lifetime semantics.

## `std::expected` versus the proposed VixC surface

At the source level, these two programs may represent similar application intent.

Using an expected-like type:

```cpp
std::expected<User, LoadError>
load_user(int id)
{
    auto record_result =
        read_record(id);

    if (!record_result)
    {
        return std::unexpected(
            record_result.error());
    }

    auto record =
        std::move(*record_result);

    return make_user(record);
}
```

Using the current VixC hypothesis:

```cpp
User load_user(int id) fails LoadError
{
    auto record =
        try read_record(id);

    return make_user(record);
}
```

The important research question is not the reduction in lines.

The important questions are whether VixC gains semantic information from the second form and whether that information justifies a language feature.

Potential benefits include:

- direct knowledge that the function has a recoverable failure contract
- explicit propagation nodes in the syntax tree
- semantic validation of propagation
- source-level diagnostics
- backend-independent Failure IR
- future interaction with other outcome categories
- tooling that understands Failure without recognizing library-specific patterns

These benefits must be demonstrated rather than assumed.

## Representation can remain C++

A language-level Failure model does not require abandoning existing C++ representations.

The backend may be able to lower VixC Failure into existing mechanisms.

Possible implementation strategies include:

```text
std::expected-like representation
generated result type
specialized tagged union
explicit status plus storage
exceptions
another native C++ representation
```

The correct choice depends on semantic requirements, performance evidence, ABI concerns, and interaction with real C++ code.

The architecture should allow these decisions to change without changing source-level meaning.

## Interoperability with existing APIs

VixC cannot assume that every dependency will adopt VixC Failure.

Real applications will call APIs using:

- exceptions
- `std::expected`
- `std::optional`
- `std::error_code`
- status objects
- integer return codes
- pointers
- callbacks
- coroutines
- project-specific result types

A useful VixC Failure model must coexist with these APIs.

That does not necessarily mean automatic conversion from every model.

Automatic adaptation could erase important distinctions.

Instead, VixC needs clear boundaries where programmers can state how an existing C++ result maps into VixC semantics.

For example, absence from:

```cpp
std::optional<T>
```

should not automatically become `failure(E)`.

Similarly, any thrown exception should not automatically become a declared VixC failure value.

Interop must preserve meaning.

## What existing C++ already solves

The existing C++ ecosystem already provides strong mechanisms for many parts of the Failure problem.

C++ already has:

- deterministic object lifetime
- RAII
- exceptions and stack unwinding
- standard value containers
- sum types
- optional values
- expected-like success/error representation
- generic programming
- user-defined conversion
- concepts and constraints
- standard error categories
- native interoperability
- high-performance value semantics

VixC should build on these properties.

It should not reimplement them unnecessarily.

The strongest justification for a frontend feature lies where semantic information is currently distributed across conventions and cannot be expressed coherently enough to the frontend.

## What existing models do not provide uniformly

Across the mechanisms examined above, C++ does not provide one uniform language-level answer to questions such as:

- Does this computation declare recoverable failure?
- What failure type belongs to that contract?
- Is this source location explicitly propagating failure?
- Is a failure operation legal in this scope?
- Is the propagated failure compatible with the enclosing computation?
- Is absence distinct from failure?
- Is cancellation distinct from failure?
- Can tooling identify propagation without recognizing a library convention?
- Can diagnostics describe failure semantics before template instantiation or backend representation?

Individual libraries can answer some of these questions.

The answers are not universal across C++ code.

That gap is the area VixC is investigating.

## Implications for VixC

The existing C++ mechanisms suggest several constraints for the VixC design.

First, VixC should not create a custom result container merely to duplicate `std::expected`.

Second, Failure semantics should remain independent from one storage representation.

Third, propagation must preserve RAII and normal C++ lifetime behavior.

Fourth, existing exceptions must remain valid C++ and must not silently become VixC failures.

Fifth, absence should remain distinguishable from recoverable failure.

Sixth, interoperability with existing C++ error models must be explicit enough to preserve their meaning.

Seventh, the language feature must justify itself through semantic analysis, diagnostics, composition, and real program structure rather than syntax reduction alone.

## Current working hypothesis

The current VixC hypothesis is therefore intentionally small.

A computation may declare:

```cpp
T operation(...) fails E
```

produce recoverable failure:

```cpp
fail error;
```

and propagate recoverable failure:

```cpp
auto value =
    try operation();
```

The frontend understands these operations directly.

The backend remains free to use appropriate native C++ machinery to implement them.

This hypothesis does not claim that exceptions, `std::expected`, error codes, or other C++ models should disappear.

It asks whether a small language-level contract can provide a coherent semantic layer above those representations while remaining compatible with the ecosystem.

## Next research step

The next step is not to select the final backend representation immediately.

The first requirement is to complete the semantic path through a real failure-aware function.

Once declaration scope, failure production, propagation, IR, lowering, and generated C++ work together, VixC can compare concrete backend strategies using real programs.

That comparison should evaluate at least:

- semantic correctness
- generated code complexity
- object lifetime behavior
- move-only values
- references
- templates
- exception interaction
- code size
- runtime cost
- compile-time cost
- diagnostic quality
- interoperability with existing C++ APIs

Only then should an implementation mechanism become part of the stable backend strategy.

The language model should remain defined by the semantics being preserved, not by whichever C++ container happens to be easiest to generate first.
