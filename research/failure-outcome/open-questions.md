# Failure and Outcome: Open Questions

The Failure and Outcome model in VixC is intentionally incomplete.

The current implementation is sufficient to establish the first frontend structure, but several semantic, syntactic, type-system, lowering, ABI, tooling, and interoperability questions remain unresolved.

These questions should remain explicit until evidence from implementation and real programs is strong enough to justify stable decisions.

The purpose of this document is to prevent temporary implementation choices from silently becoming permanent language rules.

## Language surface

The current experimental surface is:

```cpp
T operation(...) fails E
```

```cpp
fail error;
```

```cpp
auto value = try operation();
```

This surface is intentionally small.

Even these three constructs still require validation against realistic C++ code before they should be treated as stable.

## Is `fails E` the correct declaration syntax?

The current form is:

```cpp
User load_user() fails LoadError;
```

It has several desirable properties.

The successful type remains visually prominent:

```cpp
User
```

while the recoverable failure type is declared separately:

```cpp
LoadError
```

The syntax also avoids forcing a visible wrapper type around every successful result.

However, several questions remain.

Does `fails E` compose naturally with:

```cpp
const
noexcept
override
final
requires
attributes
trailing return types
```

For example:

```cpp
auto load() const noexcept fails Error -> User;
```

would require a clear grammar and a stable ordering of specifiers.

The syntax must be tested against real function declarations rather than only simple examples.

## Where does `fails` belong in a declaration?

Possible declaration forms may include:

```cpp
User load() fails Error;
```

or some placement after existing function specifiers.

The grammar must determine how Failure contracts interact with ordinary C++ declaration syntax.

This includes:

- member functions
- free functions
- templates
- constructors
- operators
- lambdas
- virtual functions
- trailing return types
- constrained functions

The placement should remain readable while minimizing ambiguity with existing C++.

## Is `fails` part of function type identity?

This is one of the most important unresolved questions.

Consider:

```cpp
User load() fails LoadError;
```

Does the Failure contract participate in the function type itself?

If it does, this affects:

- function pointers
- overload resolution
- virtual functions
- template deduction
- callbacks
- declaration matching
- ABI
- name mangling
- separate compilation

If it does not, the contract still needs a stable semantic association with the declaration.

Neither choice should be made accidentally from the first C++ backend representation.

## Can declarations differ only by Failure contract?

Consider:

```cpp
Value load() fails ErrorA;
Value load() fails ErrorB;
```

Should these represent distinct overloads?

The current model does not answer this.

Allowing it would make Failure part of overload identity.

Disallowing it would mean the Failure contract is not enough to distinguish declarations.

This decision has significant consequences for C++ interoperability and type identity.

## Must declarations agree exactly?

Consider:

```cpp
Value load() fails Error;
```

followed later by:

```cpp
Value load()
{
    // ...
}
```

Is this a mismatch?

Likewise:

```cpp
Value load() fails ErrorA;
```

and:

```cpp
Value load() fails ErrorB
{
    // ...
}
```

must eventually have a defined rule.

Possible models include:

- exact agreement required
- compatible contracts permitted
- one declaration may omit information under specific conditions
- the first declaration establishes the contract

The model must work across headers and separate translation units.

## Can Failure contracts be inferred?

The current syntax requires an explicit declaration:

```cpp
Value operation() fails Error
```

An alternative would be to infer Failure from the body.

For example:

```cpp
Value operation()
{
    fail Error{};
}
```

could theoretically cause the compiler to infer:

```text
failure(Error)
```

The current model does not do this.

Explicit contracts have important advantages for APIs, separate compilation, and reasoning at call sites.

Inference could reduce annotation but make signatures less informative and introduce instability when implementation details change.

This should remain an explicit research question rather than an accidental consequence of implementation.

## Is `fail` always a statement?

The current form is:

```cpp
fail error;
```

This treats failure production as a statement that terminates the current successful execution path.

Could `fail` ever be useful as an expression?

For example:

```cpp
return condition
    ? value
    : fail error;
```

or:

```cpp
auto value =
    condition ? result : fail error;
```

Making `fail` expression-like may improve composability but would complicate typing, control flow, parsing, and lowering.

The first model should remain statement-oriented until there is evidence that expression form is necessary.

## Is `try` the correct propagation syntax?

The current form is:

```cpp
auto value =
    try operation();
```

The main benefit is that propagation is visible exactly where control flow may leave the current success path.

However, C++ already owns the keyword:

```cpp
try
```

for exception handling.

The parser currently preserves native C++:

```cpp
try
{
    operation();
}
catch (...)
{
}
```

by recognizing `try` followed by a block as ordinary C++.

This may not be sufficient for a durable design.

The syntax needs testing across:

- nested expressions
- macros
- templates
- lambdas
- generic code
- exception syntax
- unusual formatting
- preprocessor boundaries

If ambiguity becomes too expensive, another propagation spelling may be required.

## Should propagation be explicit at every call site?

The current hypothesis requires:

```cpp
auto value =
    try operation();
```

rather than silently propagating failures from:

```cpp
auto value =
    operation();
```

This explicitness makes control flow visible.

It also adds syntax at every propagation point.

Real application experiments are needed to determine whether this remains readable at scale.

The question is not merely one of verbosity.

Explicit propagation may improve:

- code review
- control-flow reasoning
- diagnostics
- refactoring
- searchability

It may also become repetitive in failure-heavy code.

The tradeoff needs evidence.

## Can `try` appear anywhere an expression can appear?

The simplest current form is:

```cpp
auto value =
    try operation();
```

More complex forms include:

```cpp
consume(
    try operation());
```

```cpp
return transform(
    try operation());
```

```cpp
if (try check())
{
}
```

```cpp
auto result =
    combine(
        first(),
        try second(),
        third());
```

Supporting arbitrary expression placement requires careful lowering that preserves evaluation order, temporaries, and value categories.

VixC should not promise unrestricted expression placement until those transformations are proven correct.

## Does propagation require a failure-aware enclosing computation?

The current answer is yes.

For:

```cpp
auto value =
    try operation();
```

the enclosing computation must have a Failure contract capable of receiving the propagated failure.

An alternative model could permit local propagation into another construct or implicit wrapping.

The current architecture assumes propagation means:

```text
propagate from the current computation
```

This should remain explicit unless a broader handling construct requires another model.

## What exactly is a computation?

The current documentation uses the term computation to describe something that can produce outcomes.

For the first implementation, this mainly means a function-like body.

Future features may raise broader cases:

- lambdas
- coroutines
- tasks
- async blocks
- closures
- compile-time computations
- generators

The semantic model needs a precise definition of which constructs can own Outcome contracts.

## Can lambdas declare Failure?

A possible future form might be:

```cpp
auto loader = []() fails Error
{
    // ...
};
```

This raises questions about:

- lambda type identity
- conversion to function pointers
- generic lambdas
- captures
- generated call operators
- ABI

No syntax or semantics are currently committed.

## Can constructors fail?

A possible syntax would be:

```cpp
Widget() fails InitError;
```

This is difficult because constructors do not have an ordinary return type.

Questions include:

- how successful object construction is represented
- how partial construction behaves
- how base classes are initialized
- what happens to already-constructed members
- how factory APIs interact with constructor Failure
- what ABI representation is possible in C++

The first Failure slice should not attempt to solve this prematurely.

## Can destructors fail?

A possible form such as:

```cpp
~Widget() fails CleanupError;
```

would conflict with important assumptions around destruction and stack unwinding.

Destructors require especially careful interaction with ordinary C++ exception behavior.

There is currently no evidence that recoverable Failure should be supported directly from destructors.

This remains open, but it is not part of the first implementation target.

## Can operators declare Failure?

For example:

```cpp
Value operator+(const Value &) fails ArithmeticError;
```

If Failure contracts become part of ordinary callable semantics, operators naturally raise the same question.

The answer depends on function type identity, overload resolution, and expression typing.

No final rule exists yet.

## Can `main` fail?

A Failure-aware entry point such as:

```cpp
int main() fails StartupError;
```

would require a boundary between VixC Failure semantics and the platform process exit model.

Possible handling strategies include:

- generated top-level adapter
- conversion to exit code
- diagnostic printing
- application-defined handling

This should not be defined until Failure handling and application boundaries are understood.

## Failure type requirements

The current model uses:

```cpp
fails E
```

without yet defining all requirements on `E`.

Questions include:

- Must `E` be an object type?
- Can `E` be a reference?
- Can `E` be `void`?
- Can `E` be incomplete?
- Can `E` be abstract?
- Can `E` be move-only?
- Must `E` be destructible?
- Must `E` be movable?
- Can `E` be a union?
- Can `E` be dependent on template parameters?

These constraints should be derived from semantics and representation requirements rather than chosen arbitrarily.

## Must Failure values be movable?

A value-based backend may require moving failure values through propagation.

If `E` is neither movable nor copyable, some representations may become impossible.

However, making movability a source-level requirement merely because the first backend needs it would leak implementation constraints into the language.

The language must decide whether such a restriction is fundamental or backend-specific.

## Can the failure type be a reference?

For example:

```cpp
Value operation() fails Error&;
```

This introduces lifetime and ownership questions.

A propagated failure reference may outlive the object it references.

Unless a strong use case appears, reference Failure types may be undesirable.

The decision should still be semantic rather than an accidental parser restriction.

## Can one computation declare multiple failure types?

The first syntax uses one failure type:

```cpp
fails Error
```

A single C++ type can already represent multiple alternatives:

```cpp
using Error =
    std::variant<
        NetworkError,
        ParseError,
        StorageError>;
```

This may be sufficient.

An alternative language model could support:

```cpp
fails NetworkError, ParseError
```

or a dedicated Failure set.

Multiple language-level failure types would affect:

- propagation compatibility
- handling
- exhaustiveness
- declaration identity
- ABI
- diagnostics

The first model should test whether one explicit Failure type is enough before introducing a Failure-set type system.

## Should Failure types compose automatically?

Suppose:

```cpp
A first() fails ErrorA;
B second() fails ErrorB;
```

and:

```cpp
Result operation()
{
    auto a = try first();
    auto b = try second();
}
```

Could the compiler infer:

```text
fails ErrorA | ErrorB
```

for `operation`?

The current model requires an explicit enclosing contract.

Automatic composition could be convenient but would make API contracts dependent on implementation details and could cause failure sets to grow through call graphs.

This needs careful study.

## How is failure compatibility defined?

Consider:

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

When is propagation legal?

Possible rules include:

- exact type identity
- ordinary implicit C++ conversion
- explicit conversion only
- declared Failure conversion
- inheritance
- structural relation
- variant inclusion
- user-defined mapping

Each choice affects predictability, diagnostics, generic programming, and backend lowering.

This is one of the central unresolved semantic questions.

## Should ordinary implicit conversion be enough?

If C++ already allows:

```cpp
OperationError error =
    ReadError{};
```

should VixC automatically permit propagation?

This would integrate naturally with C++ conversions.

It could also make propagation behavior depend on surprising user-defined conversion constructors.

Failure conversion may deserve stronger explicitness than ordinary assignment conversion.

No final decision has been made.

## Should propagation conversion be explicit?

A stricter model could require handling or explicit transformation when failure types differ.

Conceptually:

```text
ReadError
    -> explicitly map
    -> OperationError
```

This provides clearer domain boundaries.

It also adds syntax and may make simple composition cumbersome.

Future Failure handling syntax may provide the right place to resolve this question.

## How are failure values handled?

The current slice defines:

```cpp
fail
try
```

but not a dedicated handling construct.

Eventually, a caller needs to recover from Failure rather than only propagate it.

Potential forms could involve:

- pattern matching
- choice expressions
- explicit outcome inspection
- result binding
- existing C++ control flow

Handling should likely be designed together with the choice and pattern-matching research rather than by adding a one-off `catch` equivalent prematurely.

## Should Failure handling reuse `catch`?

C++ already provides:

```cpp
catch
```

for exceptions.

Reusing it for value-based recoverable Failure could blur the distinction between ordinary exceptions and VixC Failure.

A separate mechanism may be clearer.

No decision should be made before the semantics of handling are understood.

## Relationship with `none`

The Outcome model reserves:

```text
none
```

as distinct from Failure.

Questions include:

- Which declarations can produce `none`?
- Is `none` associated with a type?
- How is absence produced?
- How is it propagated?
- Can `try` propagate `none`?
- Does absence require a different operator?
- Can a computation have both `none` and `failure(E)`?
- How is absence handled?

The Failure design should leave room for these answers.

## Relationship with `stopped`

The Outcome model also reserves:

```text
stopped
```

Questions include:

- Is stopped always cancellation?
- Can a computation stop for reasons other than cancellation?
- Does stopped carry data?
- Is stopped propagated automatically?
- Can it be handled?
- Is cleanup mandatory before stopped completion?
- How does it interact with async tasks?
- Can synchronous code produce stopped?

These questions belong primarily to future cancellation research.

Failure should not absorb stopped semantics.

## Should Outcome become a unified language construct?

One possible long-term direction is that Failure, absence, and cancellation are all parts of one general computation Outcome model.

Another possibility is that they remain separate features sharing some compiler infrastructure.

A single unified abstraction may improve consistency.

It could also become too abstract for ordinary programmers.

The implementation should not force a decision before enough features exist to compare both models.

## Failure and exceptions

The current model keeps VixC Failure distinct from C++ exceptions.

Several important questions remain.

Can a Failure-aware function throw ordinary exceptions?

Can a function both declare:

```cpp
fails Error
```

and contain:

```cpp
throw Exception{};
```

Yes may be the natural compatibility choice, but the interaction must be defined.

Can a VixC failure ever be caught by C++ `catch`?

The current semantics suggest no unless a backend intentionally exposes such a representation.

Should ordinary exceptions ever be converted automatically into Failure?

Probably not without an explicit boundary, but this remains to be specified.

## Failure and `noexcept`

Consider:

```cpp
Value operation() noexcept fails Error;
```

If Failure is implemented using value-based control flow, this may be perfectly coherent.

If one backend uses exceptions internally, that strategy must still preserve `noexcept`.

Questions include:

- Is `fails` orthogonal to `noexcept`?
- Is a Failure-aware function allowed to be `noexcept`?
- Does `noexcept` describe only ordinary C++ exceptions?
- Can backend representation affect whether a function is ABI-compatible with a normal `noexcept` function?

The semantic answer should not depend on one backend experiment.

## Failure and exception specifications

Historical C++ had dynamic exception specifications.

VixC should not reproduce their known problems merely because `fails E` is also a declaration-level effect.

The two concepts are different.

However, historical experience with exception specifications may provide useful evidence about:

- type identity
- declaration compatibility
- API evolution
- separate compilation
- generic code

This deserves deliberate study.

## Failure and `std::expected`

`std::expected<T, E>` is an obvious candidate representation for the first C++ backend.

Open questions include:

- Is it available in all targeted language modes?
- Does its interface create unnecessary generated template complexity?
- Does it preserve desired move/reference behavior?
- What are compile-time costs?
- What diagnostics result from generated code?
- Can it represent future `none` or `stopped` without additional layers?
- Does it impose ABI properties VixC should avoid?

Even if `std::expected` becomes one backend representation, the language semantics should remain independent.

## Should VixC generate its own result type?

A generated or runtime-provided type could give VixC complete control over representation.

Possible benefits include:

- tailored layout
- predictable generated API
- support for more Outcome states
- reduced dependency on standard-library implementation details
- improved source mapping

Possible costs include:

- duplicated library machinery
- maintenance burden
- ABI ownership
- compile-time overhead
- more generated code
- ecosystem friction

This choice requires measurement.

## Should Failure use exceptions internally?

An exception-based backend could provide natural stack unwinding and non-local propagation.

Potential benefits include:

- automatic cleanup
- no explicit result checks on every generated call
- mature C++ runtime support

Potential problems include:

- distinction from ordinary exceptions
- `noexcept`
- builds with exceptions disabled
- ABI
- runtime cost model
- hidden propagation
- typed Failure contract representation
- interaction with user `catch` blocks

This should be evaluated experimentally rather than ruled in or out by intuition.

## ABI questions

Failure can change the concrete representation of a function.

A source declaration:

```cpp
User load() fails Error;
```

cannot necessarily preserve the same ABI as:

```cpp
User load();
```

if both success and failure must cross the function boundary.

Questions include:

- Can Failure-aware functions be exported with C linkage?
- Can they override ordinary C++ virtual methods?
- Can ordinary C++ callers call them directly?
- How are function pointers represented?
- What symbols are generated?
- Is the representation stable between VixC versions?
- Is ABI stability a goal for early VixC?

These questions become critical before using Failure in public binary interfaces.

## Native C++ interoperability

VixC must call ordinary C++ functions.

Ordinary C++ must also eventually be able to call Failure-aware functions.

The boundary needs a clear representation.

Possible strategies include generated C++ signatures that ordinary C++ can use directly.

If VixC-specific metadata is required beyond the generated signature, headers or generated interface information may also be needed.

Interoperability must not require the entire dependency graph to be rewritten in VixC.

## How does VixC recognize existing result types?

Suppose an ordinary C++ API returns:

```cpp
std::expected<T, E>
```

Should VixC automatically treat it as:

```text
success(T)
failure(E)
```

Automatic recognition would improve interoperability.

It would also privilege specific library types and potentially conflate representation with semantics.

An explicit adapter mechanism may be safer.

The same question applies to:

- `std::optional`
- custom `Result`
- status types
- error codes

No adapter model has been chosen yet.

## Should libraries be able to declare Outcome adapters?

A future mechanism could allow a library to explain to VixC how a type maps to semantic outcomes.

Conceptually, an adapter could define:

```text
how to detect success
how to extract success
how to detect failure
how to extract failure
```

Such a system could integrate existing C++ result libraries without hard-coding them into the language.

However, it risks creating a complex protocol before the basic Failure model is stable.

This should remain future research.

## Templates

Templates are one of the most difficult unresolved areas.

Questions include:

- Can `fails E` depend on template parameters?
- Can a Failure contract be constrained?
- Does template deduction see the Failure contract?
- Can a concept require a callable to have a particular Failure contract?
- How are dependent failure types represented?
- When is propagation compatibility checked?
- How does SFINAE interact with Failure?
- How do constrained overloads interact with Failure?

The first implementation should avoid pretending source-range type spelling solves these questions.

## Concepts

A future generic interface might want to express:

```text
callable returning T and failing with E
```

Should VixC expose Failure contracts to C++ concepts?

If Failure contracts are part of the semantic type system, concepts may need access to them.

If they exist only as VixC metadata, generic programming across the boundary becomes more difficult.

This is related directly to function type identity.

## Function pointers

Consider:

```cpp
using Loader =
    User (*)();
```

Can it point to:

```cpp
User load() fails Error;
```

Probably not directly if the concrete ABI differs.

A Failure-aware function pointer type may be needed.

That again requires deciding whether Failure belongs to function type identity.

## `std::function`

Similar questions apply to:

```cpp
std::function<User()>
```

Can it hold a Failure-aware callable?

If not, what adapter is required?

If yes, where does the Failure outcome go?

The answer depends on the concrete interoperability model.

## Virtual functions

Consider:

```cpp
struct Base
{
    virtual Value load() fails Error;
};
```

and an override:

```cpp
struct Derived : Base
{
    Value load() fails DerivedError override;
};
```

Questions include:

- Must the failure type match exactly?
- Can it narrow?
- Can it widen?
- Is covariance meaningful?
- How does vtable ABI represent the contract?

This area should remain out of the first vertical slice.

## Lambdas and callable objects

If Failure becomes part of callable semantics, lambdas and `operator()` require equivalent rules.

Generic algorithms may accept callables whose failure behavior is relevant.

The language needs a coherent model rather than special rules only for free functions.

## Coroutines

Coroutines are especially significant because they separate function invocation from eventual completion.

For:

```cpp
Task<User> load() fails Error;
```

does the Failure contract describe:

- invocation of `load`
- completion of the returned `Task`
- both

If Failure represents eventual task completion, it may need integration with the coroutine promise type.

If it represents only synchronous invocation, its usefulness in async code becomes limited.

This should be studied together with VixC async research.

## Cancellation

Future cancellation may produce:

```text
stopped
```

rather than Failure.

Propagation syntax must eventually distinguish:

```text
propagate failure
propagate stopped
propagate both
```

One question is whether:

```cpp
try operation()
```

should propagate every non-success outcome or Failure only.

The current Failure experiment implicitly treats it as Failure propagation.

That may need refinement once other Outcome states become real.

## Should `try` mean Outcome propagation rather than Failure propagation?

If future computations can return:

```text
success
none
failure
stopped
```

then `try` could theoretically propagate all non-success states automatically.

That would make it a general Outcome operator.

Alternatively, each outcome kind could have distinct syntax or handling behavior.

The current implementation should avoid embedding a permanent assumption before this is understood.

## Handling multiple outcomes

A computation with:

```text
success(T)
none
failure(E)
stopped
```

will require some way to inspect or handle states.

Pattern matching may provide a natural form.

The future choice/match research should therefore be considered before Failure handling syntax is finalized.

## Control-flow representation

The current IR has high-level Failure nodes.

A complete lowering path likely needs more explicit control-flow representation.

Open questions include:

- Should VixC introduce a control-flow IR?
- Should Failure lower directly into a generic CFG?
- Should there be dedicated Outcome branches?
- Should lowering use SSA-like values?
- How much C++ structure should remain opaque?
- Can CxxRegion coexist safely with fine-grained control flow?

The answer affects much more than Failure.

## Is a complete C++ parser eventually necessary?

The current architecture preserves ordinary C++ as `CxxRegion`.

This keeps the first frontend small.

However, increasingly sophisticated transformations may require understanding more surrounding C++ structure.

For example, correct lowering of:

```cpp
consume(
    first(),
    try second(),
    third());
```

requires knowledge about expression boundaries and sequencing.

Possible long-term directions include:

- progressively richer VixC parsing
- integration with Clang
- another C++ frontend
- restricting where VixC constructs may appear
- structured source preservation around selected constructs

The correct balance is still unknown.

## How much C++ syntax must VixC understand?

VixC does not need to replace a full C++ compiler merely to introduce selected semantics.

But it must understand enough structure to transform its own constructs safely.

The minimum required subset depends on where VixC syntax is allowed.

Allowing VixC constructs everywhere increases parser requirements.

Constraining them to specific syntactic positions reduces implementation complexity but may weaken usability.

This is a central frontend design tradeoff.

## Preprocessor interaction

C++ preprocessing complicates source-level semantics.

Questions include:

- Can `fails`, `fail`, or `try` come from macros?
- Can macros surround parts of these constructs?
- Does VixC process before or after preprocessing?
- How are source ranges preserved through macro expansion?
- Can a macro named `fail` remain valid ordinary C++?
- How do conditional compilation branches affect parsing?

The current frontend operates on source text before integration with a real C++ preprocessing model.

This area requires significant research.

## Keyword compatibility

The current lexer treats:

```cpp
fail
fails
try
```

as dedicated tokens.

But valid C++ code may use identifiers such as:

```cpp
object.fail();
```

or:

```cpp
int fails = 0;
```

`try` is already a C++ keyword, but `fail` and `fails` are not.

If VixC makes them unconditional keywords, some existing C++ code becomes invalid under VixC.

Possible alternatives include contextual keywords.

For example, `fails` may only be recognized in declaration context and `fail` only in statement context.

This issue must be resolved before claiming broad C++ compatibility.

## Contextual keywords

Using contextual keywords could reduce compatibility breakage.

For example:

```cpp
service.fail();
```

could remain ordinary C++, while:

```cpp
fail Error{};
```

in statement position becomes VixC syntax.

Similarly:

```cpp
bool fails = false;
```

could remain valid.

Contextual parsing is more complex than unconditional keyword tokenization.

The compatibility benefit may justify that complexity.

## Preprocessor directives

The first lexer does not yet model the full range of C++ preprocessing syntax.

Source such as:

```cpp
#include <vector>
#define VALUE 42
#if ENABLED
#endif
```

must remain valid ordinary C++.

VixC needs a strategy for preserving preprocessing without falsely diagnosing unknown punctuation.

This is a prerequisite for realistic C++ source compatibility.

## Source rewriting versus structured emission

Ordinary C++ regions are currently preserved by source range.

One possible strategy is to retain as much original text as possible and only replace VixC constructs.

Another is to build richer structured IR and emit more code from syntax.

The first strategy preserves formatting and compatibility better.

The second provides more control over transformations.

Failure lowering may require a hybrid approach.

The correct boundary remains open.

## Formatting preservation

Should generated C++ preserve original formatting where possible?

This is not semantically required.

It may still matter for:

- generated code inspection
- debugging
- source maps
- compiler diagnostics
- developer trust

Ordinary `CxxRegion` preservation already supports exact text retention.

Generated Failure structures will necessarily introduce new formatting.

Deterministic output is more important than reproducing every original whitespace choice.

## Source maps

The current source map records:

```text
generated byte interval
original SourceRange
```

Open questions include:

- How are generated helper regions mapped?
- Can one source range map to multiple generated intervals?
- Can one generated interval depend on multiple source ranges?
- How are macro expansions represented?
- How are generated line directives handled?
- Should native compiler diagnostics be translated automatically by VixC or by Vix.cpp?

The initial mapping model is sufficient for experiments but may need richer provenance.

## Should VixC emit `#line` directives?

One possible way to improve native compiler diagnostics is generated:

```cpp
#line
```

directives.

Potential benefits include allowing GCC or Clang to report original filenames and line numbers directly.

Potential problems include:

- generated helper diagnostics becoming misleading
- complex mappings for one-to-many transformations
- platform differences
- interaction with debugging information
- generated code inspection

This should be compared with explicit external source-map translation.

## Debugger behavior

Generated C++ eventually produces native debug information.

Should a debugger show:

- generated C++ lines
- original VixC source
- both

Source mapping at compile diagnostics is only one part of the tooling problem.

A mature VixC frontend may need integration with debug information generation or compiler line directives.

This is outside the first Failure milestone but relevant to long-term usability.

## Separate compilation

The first experiments can operate on one source unit.

A real language frontend must support declarations across files.

Failure contracts then need to survive:

- headers
- modules
- generated declarations
- translation-unit boundaries
- libraries

Questions include how semantic contract information is communicated when the C++ backend only sees generated signatures.

This may require generated metadata or a representation fully encoded in C++ types.

## C++ modules

C++ modules create additional questions.

If Failure contracts appear in exported declarations:

```cpp
export User load() fails Error;
```

the contract must survive module interfaces.

If the generated C++ representation changes the declaration, VixC must preserve module compatibility across compilation units.

This is future work.

## Header interoperability

One desirable property would be generated C++ headers that ordinary C++ code can include.

For example, VixC may transform:

```cpp
User load() fails Error;
```

into a stable ordinary C++ declaration.

The exact representation determines how easy mixed VixC/C++ projects become.

This is one reason backend ABI should be studied early enough to avoid locking into a poor representation.

## Linkage

Failure-aware functions may have:

```cpp
extern "C"
```

or other linkage specifications.

If the concrete signature requires a C++ result type, C linkage may become problematic.

The language needs a rule for such boundaries.

Possible choices include:

- prohibit Failure under C linkage
- generate an ABI-compatible adapter
- define a C-compatible outcome representation

No decision exists yet.

## Performance

Failure should not assume performance characteristics without measurement.

Questions include:

- What is the size of the generated result representation?
- How many branches does propagation introduce?
- How well does the optimizer eliminate wrappers?
- What happens in hot loops?
- What happens with large failure types?
- How does code size compare with exceptions?
- How does branch prediction behave?
- What happens when failure is rare?
- What happens when failure is common?

Benchmarks should use realistic functions rather than trivial microexamples only.

## Compile time

Generated C++ may increase native compilation cost.

Questions include:

- How much template instantiation does the representation require?
- Does every Failure-aware function instantiate large helper machinery?
- Can generated code remain simple?
- Does source-map infrastructure affect compile time meaningfully?
- How does incremental compilation behave?

VixC should treat compile time as part of the developer experience.

## Binary size

Different Failure representations may affect binary size differently.

Exception tables, template instantiations, generated helpers, and inline branches all have different costs.

This should be measured across realistic applications.

## Optimization

The backend representation should allow native compilers to optimize successfully.

Questions include:

- Can successful paths inline across Failure boundaries?
- Can branch checks disappear when statically known?
- Can result wrappers be elided?
- Are moves optimized away?
- Does propagation inhibit vectorization or other optimization?
- Can error paths be outlined?

Generated code should be inspected at optimized levels.

## Zero-cost expectations

VixC should be careful with claims such as "zero-cost."

Even a semantically efficient representation may impose:

- extra branches
- larger return values
- code-size growth
- exception tables
- compile-time cost

The meaningful requirement is that overhead is measured, understood, and appropriate for the semantics provided.

## Error allocation

Should Failure require allocation?

The likely answer should be no as a semantic requirement.

A failure type may allocate internally if the user chooses.

The VixC mechanism itself should not require heap allocation unless a future representation proves it unavoidable.

This should be tested across candidate backends.

## Thread safety

Failure values may cross thread boundaries through user code.

The language itself should not silently impose synchronization.

Future async semantics may introduce additional constraints.

The first Failure model should preserve normal C++ type-based thread-safety expectations.

## Exceptions disabled

Some C++ projects compile with exceptions disabled.

If VixC Failure is represented through value-based control flow, such environments can still be supported naturally.

If any backend strategy relies on exceptions, VixC must define behavior when the native compiler disables them.

This is a significant interoperability consideration.

## RTTI disabled

Failure semantics should probably not require RTTI.

If a backend strategy uses runtime type inspection for Failure, it could become incompatible with RTTI-disabled builds.

Typed compile-time representation is likely preferable, but this should remain an implementation requirement rather than an unsupported assumption.

## Embedded environments

VixC may eventually be used in constrained native environments.

Questions include:

- Can Failure work without exceptions?
- Can it work without dynamic allocation?
- Can it work without RTTI?
- Can it work in freestanding C++?
- Does generated support require the full standard library?

These questions may influence representation choices even if the first implementation targets hosted desktop environments.

## Diagnostics

Several diagnostic questions remain open.

Should diagnostic codes become stable public API?

Should messages themselves be treated as stable?

Should VixC support structured fixes?

For example, if:

```cpp
fail Error{};
```

appears outside a Failure-aware function, could tooling suggest adding:

```cpp
fails Error
```

to the declaration?

Such automated fixes require enough semantic confidence to avoid encouraging the wrong design.

## Diagnostic notes

The current diagnostic model can emit Notes as separate diagnostics.

Should related information become a richer structured field instead?

For example:

```text
primary error
related declaration
related called function
suggested conversion
```

IDE integration would benefit from structured related locations.

This may justify expanding the public Diagnostic API later.

## Internal errors

Compiler implementation defects currently pass through the same diagnostic infrastructure as source errors.

A mature frontend may need a dedicated internal diagnostic category.

For example:

```text
internal compiler error
```

should be distinguishable from:

```text
invalid source program
```

This is especially important when VixC becomes user-facing.

## Error recovery

The parser and semantic analyzer need a strategy for continuing after invalid Failure constructs.

Questions include:

- When should parsing synchronize?
- Which malformed constructs create placeholder nodes?
- When should semantic diagnostics be suppressed due to previous syntax errors?
- How are multiple independent errors reported deterministically?

Good recovery matters for editor use.

## Tooling API

The current public API returns diagnostics and generated output.

Future tooling may require semantic information such as:

- Failure contract of a declaration
- type of a Failure value
- propagation target
- source-to-IR mapping
- available outcome states

Should this become part of a stable analysis API?

Exposing internal IR directly would create strong compatibility constraints.

A separate semantic query interface may be preferable.

## Language server integration

A future VixC language server could provide:

- diagnostics
- hover information
- Failure contract display
- go-to-definition
- propagation visualization
- semantic highlighting
- refactoring

This raises the question of how much C++ semantic information VixC owns versus delegates to existing C++ tooling.

Integration with clangd or another C++ language server may be more practical than rebuilding all native tooling.

## Clang integration

One possible long-term direction is integration with Clang for C++ parsing and semantic information.

Potential benefits include:

- full C++ grammar
- type system
- templates
- declarations
- macros
- diagnostics
- AST
- tooling ecosystem

Potential costs include:

- heavy dependency
- compile-time and memory cost
- version coupling
- portability
- loss of frontend independence
- architectural complexity

The current lightweight frontend allows semantic experimentation quickly.

The point at which richer C++ frontend integration becomes necessary remains open.

## GCC and MSVC integration

If VixC relies too heavily on Clang-specific infrastructure, preserving GCC and MSVC as native backends may become harder conceptually or operationally.

One possible model is to use Clang libraries only for frontend analysis while still emitting portable C++ for other compilers.

Another is to remain independent.

This is a major architectural decision and should be driven by actual frontend requirements.

## Runtime

The current Failure model does not require a VixC runtime.

Open questions include whether future Outcome features introduce shared runtime needs.

For example:

- cancellation state
- async scheduling
- stackless task metadata
- dynamic failure metadata
- reflection

Runtime infrastructure should be introduced only when semantics demonstrate a need.

Failure should not become the excuse for introducing a runtime prematurely.

## Standard library surface

Should VixC eventually provide standard support types associated with Failure?

Possibilities include:

```text
Outcome
Failure
Result
Error traits
conversion utilities
```

A standard surface may improve interoperability.

It may also duplicate C++ standard-library facilities.

The first research milestone should remain focused on language semantics before creating a broad standard library.

## Naming

The current research uses:

```text
Failure
Outcome
success
none
failure
stopped
```

These names are working semantic vocabulary.

They should remain open to revision if real programs show that another terminology is clearer.

Naming should reflect distinctions precisely rather than merely sounding familiar.

## Is `failure` the correct term?

Possible alternatives include:

```text
error
recoverable error
failure
fault
```

`error` is familiar but overloaded.

`failure` emphasizes unsuccessful computation.

The term should remain consistent across:

- syntax
- diagnostics
- IR
- documentation
- tooling

The current implementation uses Failure.

Changing it later becomes more expensive once public APIs stabilize.

## Is `Outcome` too broad?

`Outcome` currently groups:

```text
success
none
failure
stopped
```

This may become a useful general model.

It may also turn into an abstraction that is too broad for the actual language.

The research should test whether these states genuinely share enough semantics to justify one concept.

## Interaction with ownership research

Future ownership and lifetime work may influence Failure.

For example:

```cpp
auto resource =
    try acquire();
```

propagation may transfer or destroy resources.

An ownership model could provide stronger guarantees around these transitions.

Failure should therefore preserve enough semantic structure for future ownership analysis.

It should not encode resource behavior only in generated C++ text.

## Interaction with compile-time evaluation

Can Failure occur during compile-time computation?

For example, if VixC later has richer compile-time semantics:

```text
compile-time operation -> failure(E)
```

does that produce:

- a compile-time value
- a compiler diagnostic
- a recoverable compile-time branch

The current model is runtime-oriented.

The distinction may become important during compile-time/reflection research.

## Interaction with reflection

Reflection may expose Failure contracts.

A future program may want to ask:

```text
Does this function fail?
What is its failure type?
```

If Failure is semantic language information, reflection should potentially expose it.

That requires a stable representation beyond backend-specific C++ types.

## Interaction with pattern matching

Failure handling may naturally integrate with pattern matching.

For example, a future construct might distinguish:

```text
success(value)
failure(error)
none
stopped
```

If so, the shape of Failure handling should not be finalized independently from match semantics.

This is one reason the current research limits itself to production and propagation.

## Interaction with async

Async may make Outcome semantics much more important.

A task can potentially:

```text
succeed
fail
be cancelled
```

This maps naturally onto:

```text
success
failure
stopped
```

The current synchronous Failure model may therefore become the foundation of a broader computation model.

That possibility should be tested rather than assumed.

## Should Failure be restricted initially?

One practical strategy is to support Failure only in a constrained subset first.

For example:

- ordinary non-template functions
- value return types
- one Failure type
- propagation only in declaration initializers and standalone expressions
- no constructors
- no coroutines
- no virtual functions
- no exported ABI promises

This could allow semantics and lowering to stabilize before broadening the surface.

The risk is that experimental restrictions accidentally become permanent limitations.

Any restriction should therefore be documented as implementation scope rather than language principle unless intentionally specified.

## What constitutes the first complete experiment?

The first experiment should not be considered complete merely because:

```cpp
fail
```

and:

```cpp
try
```

parse.

A meaningful completion requires a real function:

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

to pass through:

- source management
- lexing
- parsing
- declaration-level Failure scope
- semantic analysis
- IR construction
- lowering
- C++ backend generation
- native compilation

The generated program must preserve the intended success and failure behavior.

## What evidence is required after the first experiment?

After the first complete vertical slice, VixC should test Failure with progressively harder programs.

At minimum:

- several independent Failure-aware functions
- nested calls
- multiple propagation points
- different failure domains
- move-only successful values
- move-only failure values
- RAII objects
- references
- templates
- exceptions
- `noexcept`
- loops
- conditionals
- recursive functions
- cross-file declarations
- existing C++ libraries

Only after this evidence should the model move toward stability.

## What would falsify the current design?

Research should allow the current model to fail.

Evidence against the design could include:

- `try` causing unacceptable C++ grammar conflicts
- Failure contracts becoming impossible to integrate with function types
- lowering requiring too much reconstruction of C++
- generated code introducing unacceptable compile-time cost
- performance significantly worse than existing approaches
- diagnostics becoming less clear than library-level solutions
- interoperability requiring pervasive wrappers
- templates becoming impractical
- source mapping becoming unreliable
- real code becoming less readable rather than more coherent

If these problems cannot be resolved without excessive complexity, VixC should revise the model rather than protect the syntax.

## What would support the current design?

Evidence in favor would include:

- clear source-level contracts
- concise but explicit propagation
- substantially better diagnostics
- predictable interaction with ordinary C++
- correct RAII and value semantics
- simple deterministic generated code
- competitive runtime performance
- acceptable compile-time cost
- straightforward interoperability
- useful tooling information unavailable from library conventions alone

The decision should be based on these properties rather than attachment to the current spelling.

## What must not become accidental specification?

Several current implementation choices are explicitly provisional.

These include:

- `fail`, `fails`, and `try` being unconditional lexer keywords
- failure type represented by source range
- IR construction currently occurring inside `Frontend.cpp`
- lowering primarily validating rather than transforming
- C++ backend currently rejecting unresolved Failure nodes
- source map containing one original range per generated interval
- first backend being C++
- one Failure type per computation
- `try` currently representing Failure propagation only

None of these should become permanent merely because they exist in the first implementation.

## Immediate questions

The next implementation phase should answer a smaller set of questions before expanding the language.

First, how should the parser represent a complete function declaration containing:

```cpp
fails E
```

and its body?

Second, how should semantic analysis associate the contract with exactly that body?

Third, how should semantic IR represent the complete failure-aware computation rather than disconnected Failure nodes?

Fourth, what backend-independent control-flow form is sufficient to lower:

```cpp
try expression
```

correctly?

Fifth, what first C++ representation can implement the semantics without locking the language to that representation?

Sixth, how can that representation preserve:

- RAII
- single evaluation
- move-only values
- source provenance

These questions are more important than adding another Failure syntax form.

## Research discipline

The Failure model should evolve through evidence.

For every proposed decision, the implementation should distinguish:

```text
semantic requirement
implementation convenience
backend constraint
temporary experiment
stable language contract
```

These categories are not interchangeable.

A limitation of the first C++ backend is not automatically a language limitation.

A convenient parser shortcut is not automatically syntax.

A test fixture is not automatically specification.

A generated representation is not automatically semantics.

Keeping those boundaries explicit is necessary if VixC is to become a durable frontend rather than a sequence of irreversible experiments.

## Current position

The current working model is intentionally narrow.

A computation can declare recoverable failure:

```cpp
T operation(...) fails E
```

produce failure:

```cpp
fail error;
```

and propagate failure:

```cpp
auto value =
    try operation();
```

The broader Outcome model distinguishes:

```text
success
none
failure
stopped
```

Programmer errors remain outside ordinary outcomes.

This is enough to build the first complete experiment.

It is not enough to declare the model finished.

The unanswered questions in this document are part of the implementation work, not documentation debt.

They are the boundaries that must be resolved before Failure and Outcome can become stable VixC language semantics.
