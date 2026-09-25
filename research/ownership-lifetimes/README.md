# Ownership and Lifetimes Research

This directory contains research for possible VixC language semantics around ownership, borrowing, object lifetime, resource validity, and movement across computation boundaries.

The goal is not to replace C++ RAII, destructors, references, smart pointers, move semantics, or the existing object model.

C++ already has powerful lifetime mechanisms.

The research question is narrower:

> Can VixC make important ownership and lifetime relationships explicit enough for the frontend to reason about them without replacing ordinary C++ resource management?

This work is exploratory.

No ownership syntax, borrow syntax, region model, or static guarantee described here should be considered stable.

## Motivation

C++ gives programmers direct control over object lifetime.

That is one of its strengths.

A resource can be owned directly:

```cpp
Resource resource;
```

through dynamic ownership:

```cpp
std::unique_ptr<Resource> resource;
```

through shared ownership:

```cpp
std::shared_ptr<Resource> resource;
```

or accessed indirectly:

```cpp
Resource &resource;
```

```cpp
Resource *resource;
```

The language also provides:

- constructors
- destructors
- copy semantics
- move semantics
- references
- pointers
- storage duration
- temporary lifetime rules
- RAII
- smart pointers
- standard containers
- user-defined ownership types

The problem is not that C++ lacks lifetime mechanisms.

The difficulty is that important ownership relationships are often implicit.

A programmer may need to determine manually:

- who owns an object
- whether a reference remains valid
- whether a callback outlives captured state
- whether an asynchronous task can outlive a borrowed resource
- whether a moved-from object is still used
- whether returned references point to valid storage
- whether a view remains valid after mutation
- whether a resource has already been released
- whether a temporary outlives the operation that uses it

Some of these questions are already constrained by C++ rules.

Others depend on API conventions and program structure that the compiler may not model directly.

VixC research asks whether selected relationships could become explicit semantic information without forcing C++ into an entirely different ownership system.

## C++ ownership already exists

VixC should begin from the fact that C++ already expresses ownership through ordinary types and lifetimes.

For example:

```cpp
std::unique_ptr<Resource>
```

strongly suggests unique ownership.

```cpp
std::shared_ptr<Resource>
```

represents shared ownership through reference counting.

```cpp
std::span<T>
```

does not own its elements.

```cpp
std::string_view
```

does not own the characters it references.

```cpp
T &
```

normally refers to an object owned elsewhere.

These distinctions already matter.

VixC should not introduce new syntax merely to rename them.

The interesting question is whether the frontend can reason about relationships between those values across scopes and operations.

## Ownership is not lifetime

Ownership and lifetime are related but not identical.

Ownership answers:

> Which object or abstraction is responsible for keeping this resource alive and eventually releasing it?

Lifetime answers:

> During which part of program execution is this object or resource valid?

A value can be non-owning while still having a valid lifetime.

For example:

```cpp
void print(const std::string &text);
```

the reference does not own the string.

It is still valid for the duration required by the call.

Likewise, shared ownership can keep an object alive without identifying one unique owner.

VixC should avoid reducing all lifetime reasoning to one ownership category.

## Lifetime is not storage duration alone

C++ storage duration provides categories such as:

- automatic
- static
- thread
- dynamic

These are important but do not answer every validity question.

A dynamically allocated object may be destroyed while the pointer value remains.

A `std::vector` element may be invalidated after reallocation even though the vector itself remains alive.

A `std::string_view` may outlive the referenced string.

A reference captured by an asynchronous task may outlive the scope that created it.

The frontend therefore needs more than storage-duration categories if it is to reason about actual validity.

## RAII remains fundamental

RAII should remain the primary resource-lifetime foundation.

For example:

```cpp
File file(path);
```

can acquire a resource during construction and release it during destruction.

The normal C++ scope:

```cpp
{
    File file(path);

    use(file);
}
```

already provides deterministic cleanup.

VixC should preserve this model.

A future ownership feature should strengthen reasoning around RAII rather than replace destructors with another cleanup system.

## Deterministic destruction

C++ deterministic destruction is especially important to Failure and async research.

If control leaves a scope because of:

- normal return
- VixC Failure propagation
- ordinary exception
- future stopped completion

objects whose lifetimes end at that boundary must still be destroyed according to C++ rules.

Any ownership or lifetime semantics introduced by VixC must compose with this property.

## Resource ownership is broader than memory

Ownership research should not focus only on heap memory.

C++ resources include:

- file descriptors
- sockets
- locks
- database transactions
- memory mappings
- GPU resources
- operating-system handles
- temporary files
- processes
- threads
- coroutine frames
- network sessions
- custom protocol state

A useful ownership model should reason about resource lifetime generally rather than only about pointers.

RAII is valuable precisely because it already generalizes beyond memory.

## Borrowing

A borrowed value accesses a resource without owning its lifetime.

Ordinary C++ already has many borrowing forms:

```cpp
T &
const T &
T *
std::span<T>
std::string_view
```

and user-defined view types.

The semantic question is whether VixC should understand some concept of:

```text
borrowed from X
```

so that it can reason about whether the owner remains alive.

This does not necessarily require a new pointer or reference syntax.

## Borrow validity

Consider:

```cpp
std::string_view make_view()
{
    std::string text =
        "hello";

    return text;
}
```

The returned view refers to storage whose lifetime ends before the caller can use it.

This is a lifetime problem.

The type system communicates that `std::string_view` is non-owning, but the relationship between the view and the local string is not encoded directly in the function type.

A VixC lifetime model could potentially reason about such relationships.

The challenge is doing so without attempting to reimplement every C++ lifetime rule.

## References

C++ references are a central borrowing mechanism.

For example:

```cpp
const User &
find_user();
```

communicates that the function does not return a new `User` value.

It does not necessarily communicate:

- who owns the object
- how long the object remains valid
- whether another operation can invalidate it
- whether the reference may cross an async boundary

These properties often live in documentation.

VixC research should investigate which of them can become semantic contracts.

## Pointers

Raw pointers are more ambiguous.

A pointer can represent:

- unique ownership
- shared ownership
- borrowing
- optional reference
- array access
- iterator-like traversal
- memory-mapped data
- foreign ABI state

The pointer type alone does not establish one ownership meaning.

VixC should not assume:

```cpp
T *
```

means borrowed or owned universally.

Any ownership analysis involving raw pointers needs additional semantic information or conservative behavior.

## Smart pointers

Smart pointers provide stronger ownership signals.

For example:

```cpp
std::unique_ptr<T>
```

has clear ownership semantics.

A move transfers ownership.

Copying is disabled.

Destruction releases the resource according to the deleter.

A lifetime model can potentially use this existing type information directly.

There is little reason for VixC to invent another unique-owner pointer unless a concrete semantic need appears.

## Shared ownership

`std::shared_ptr<T>` allows several values to participate in object lifetime.

This complicates simple owner-borrower models.

A reference derived from shared ownership may remain valid if some other owner survives.

The frontend cannot always determine at compile time when the final owner disappears.

A general VixC ownership model must therefore handle cases where lifetime is dynamic.

It should not claim static guarantees it cannot prove.

## Weak ownership

`std::weak_ptr<T>` represents non-owning access to shared ownership state.

Validity must be checked dynamically.

This demonstrates another important distinction.

Some lifetime relationships can be proven statically.

Others are intentionally dynamic.

VixC should not attempt to turn every lifetime question into a compile-time guarantee.

A good language model can distinguish:

```text
proven safe
requires runtime validation
cannot be proven
```

rather than pretending one mechanism solves all cases.

## Move semantics

C++ move semantics already provide an ownership-transfer mechanism for many types.

For example:

```cpp
auto first =
    std::make_unique<Resource>();

auto second =
    std::move(first);
```

Ownership moves to `second`.

The moved-from object remains valid according to its type's contract, but its value may be unspecified or otherwise limited.

VixC should preserve ordinary move behavior.

The research question is whether the frontend can improve diagnostics when a moved-from value is used incorrectly in contexts where its state is known.

## Moved-from state

C++ does not generally make every use of a moved-from object invalid.

Many moved-from standard-library objects remain valid but with unspecified state.

Therefore, a simplistic rule:

```text
moved value can never be used again
```

would be incompatible with C++.

Any VixC analysis must understand the distinction between:

```text
ownership transferred
```

and:

```text
source object ceased to exist
```

They are not equivalent.

## Ownership transfer

Some resources have stronger transfer semantics.

For example:

```cpp
std::unique_ptr<T>
```

after move no longer owns the object.

That property is stronger than generic moved-from state.

VixC may be able to derive useful ownership facts from selected standard types or explicit semantic contracts.

It should not generalize those facts to every movable C++ type.

## Copy semantics

Copying may duplicate value while preserving independent ownership.

For example:

```cpp
std::string a =
    "hello";

std::string b =
    a;
```

both strings own their own storage.

Other types may implement copies that share underlying resources.

The frontend cannot infer ownership behavior from the existence of a copy constructor alone.

Ownership semantics may therefore require type-specific knowledge or explicit contracts.

## Lifetime relationships should be semantic

A useful lifetime model should represent relationships such as:

```text
view V depends on object O
```

or:

```text
task T borrows resource R
```

rather than reducing them immediately to implementation checks.

The backend does not need to represent these relationships at runtime if they have already served their compile-time purpose.

This makes ownership and lifetime a frontend concern in many cases.

## Regions

One possible research model is to associate values with lifetime regions.

Conceptually:

```text
region A:
    owner x
    borrow y from x
```

Then:

```text
y cannot remain valid beyond x
```

This is only semantic notation.

VixC has not selected a region syntax or formal region system.

The concept is useful because it describes lifetime ordering without requiring every resource to use one specific pointer type.

## Lexical regions

C++ scopes naturally create lexical lifetime regions.

For example:

```cpp
{
    Resource resource;

    use(resource);
}
```

the local object's lifetime belongs to that scope.

A reference to the object cannot remain valid after destruction.

Lexical regions are comparatively easy to reason about.

The challenge becomes harder when ownership escapes through:

- return values
- heap allocation
- callbacks
- containers
- coroutines
- shared ownership
- globals

A VixC model should probably begin with local and structured relationships before trying to solve every possible lifetime graph.

## Non-lexical lifetimes

A borrow may end before the lexical scope ends.

For example:

```cpp
auto &ref =
    object;

use(ref);

// ref is no longer used here
mutate(object);
```

A precise analysis could know that the effective borrow ended after `use(ref)`.

This is often called non-lexical lifetime reasoning in other language designs.

Whether VixC needs such precision depends on the restrictions it eventually wants to enforce.

A purely lexical model may be easier but reject safe C++ unnecessarily.

## Lifetime inference

Explicitly annotating every lifetime relation would make C++ significantly more verbose.

A useful VixC model should infer obvious relationships when possible.

For example, passing:

```cpp
const T &
```

to a function for the duration of the call does not need a new annotation in most code.

Explicit syntax should be reserved for relationships the frontend cannot infer or for API boundaries where the contract matters.

## Lifetime annotations

Possible future annotations could describe that:

- a returned view borrows from an argument
- a callback borrows from its owner
- a task cannot outlive a resource
- a field references another field's storage
- a handle owns a resource

No syntax has been selected.

The first research step is to identify which relationships are actually difficult enough to justify annotations.

## Return lifetime

Consider:

```cpp
std::string_view prefix(
    const std::string &text);
```

A human may understand that the returned view probably refers to `text`.

The function type does not state that relationship explicitly.

A lifetime contract could conceptually express:

```text
return borrows from text
```

This would allow the caller to reason about validity.

For example:

```cpp
auto view =
    prefix(text);
```

would remain valid while the relevant storage in `text` remains valid.

The exact mutation and invalidation rules would still depend on the underlying type.

## Borrowing from multiple arguments

A returned value may borrow from more than one input.

For example, a function might return a view into either of two buffers.

Conceptually:

```text
return borrows from A or B
```

This is harder to represent statically.

The language may need conservative lifetime union semantics.

The complexity of such cases is one reason to keep the first experiment small.

## Member lifetimes

A member can borrow from another object.

For example:

```cpp
struct Parser
{
    std::string_view input;
};
```

The `Parser` may be valid only while the referenced input storage remains valid.

The type itself does not necessarily own that input.

A VixC lifetime model could potentially express:

```text
Parser lifetime <= input lifetime
```

This becomes important when objects are stored, returned, or moved.

## Self-referential structures

Some types contain references or pointers to their own internal storage.

Moving such objects can invalidate those references unless carefully implemented.

C++ permits these designs.

A static lifetime model must therefore avoid assuming movement is harmless for all borrowed relationships.

Self-referential types are a difficult advanced case and should not define the first ownership experiment.

## Container invalidation

Lifetime validity is not determined only by owner destruction.

Operations can invalidate references while the owner remains alive.

For example:

```cpp
std::vector<int> values;
```

a reference into `values` may become invalid after reallocation.

Likewise:

```cpp
std::string_view
```

into a string may become invalid after mutation.

This means:

```text
owner is still alive
```

is necessary but not always sufficient.

VixC must distinguish object lifetime from reference stability.

## Invalidation semantics

To reason precisely about views into containers, the frontend would need to know which operations invalidate which references.

That information is type-specific.

Potential approaches include:

- built-in knowledge for selected standard types
- library annotations
- conservative assumptions
- no static guarantee for mutation-sensitive borrows

A universal hard-coded model would not scale to the entire C++ ecosystem.

An extensible semantic contract may eventually be required.

## Iterators

C++ iterators are another borrowing abstraction.

Their validity depends on the container and operation performed.

For example, different standard containers have different invalidation rules.

A VixC ownership model should not treat iterators as simple pointers with one universal lifetime rule.

Iterator validity is a useful test of whether the model is truly compatible with C++ rather than only with trivial references.

## Views

Modern C++ increasingly uses view types.

Examples include:

```cpp
std::span
std::string_view
std::ranges::subrange
```

and custom range views.

These types are often non-owning.

Their safety depends on the lifetime and invalidation behavior of underlying storage.

VixC should examine these real types before inventing a new borrowing abstraction.

## Temporaries

Temporary lifetime is a common source of subtle errors.

For example:

```cpp
std::string_view view =
    make_string();
```

may leave `view` referring to destroyed storage.

Any lifetime analysis must understand enough of ordinary C++ temporary behavior to avoid false confidence.

VixC should not claim a stronger lifetime model while leaving temporary relationships opaque.

## Lifetime extension

C++ has specific rules where binding references can extend temporary lifetime.

These rules are nuanced.

A VixC model should preserve them rather than invent a separate general rule around references.

The frontend may eventually rely on a deeper C++ semantic engine for this class of analysis.

## Function parameters

Parameter passing already communicates some ownership information.

For example:

```cpp
void consume(
    std::unique_ptr<Resource> resource);
```

strongly indicates transfer of ownership.

```cpp
void inspect(
    const Resource &resource);
```

suggests borrowing for the call.

```cpp
void maybe_store(
    Resource *resource);
```

is ambiguous.

The frontend may be able to derive useful defaults from strongly expressive types while requiring additional contracts for ambiguous APIs.

## Passing by value

Passing an owning value by value may move or copy ownership according to ordinary C++ overload resolution.

For example:

```cpp
void store(
    std::unique_ptr<Resource> resource);
```

the caller normally transfers ownership:

```cpp
store(
    std::move(resource));
```

VixC should not obscure this ordinary C++ mechanism.

## Escaping references

A major lifetime boundary is whether a borrowed value escapes the call.

For example:

```cpp
void inspect(
    const Resource &resource);
```

may only use the reference during the call.

Another function may store it:

```cpp
void register_resource(
    const Resource &resource);
```

If the function retains the reference, the caller needs a stronger lifetime guarantee.

The signature alone does not communicate that behavior.

A lifetime contract could potentially distinguish:

```text
borrowed only for call
```

from:

```text
borrow escapes into object or task
```

This may provide meaningful value.

## Callback captures

Callbacks are one of the clearest lifetime problems in modern C++.

Consider:

```cpp
void start()
{
    Resource resource;

    register_callback(
        [&resource]
        {
            use(resource);
        });
}
```

If the callback survives after `start()` returns, the captured reference becomes invalid.

The compiler cannot determine this from lambda syntax alone because the lifetime behavior depends on `register_callback`.

A semantic contract on callback ownership could make the relationship analyzable.

## Callback lifetime contracts

An API may conceptually state that a callback is:

```text
invoked only during the call
```

or:

```text
stored until unregister
```

or:

```text
owned by returned subscription
```

These are lifetime semantics.

If VixC had a way to understand them, callback capture safety could be analyzed more accurately.

This is a strong candidate area for ownership research because ordinary C++ signatures often omit this information.

## Async boundaries

Asynchronous work makes lifetime relationships even more important.

For example:

```cpp
Resource resource;

start_async(
    [&resource]
    {
        use(resource);
    });
```

If the task can outlive the current scope, the reference is unsafe.

A structured concurrency model could prevent this by ensuring the task completes before `resource` is destroyed.

This shows that async structure itself can solve some lifetime problems without requiring a separate borrow syntax.

Ownership and async research should therefore be designed together.

## Structured lifetime

A child computation may borrow state from its parent safely if the language guarantees:

```text
child lifetime <= parent scope lifetime
```

For example:

```text
parent scope
    owns Resource

    child task
        borrows Resource

    child resolved

parent scope exits
```

This is a semantic lifetime relation.

Structured concurrency could provide it naturally.

Detached tasks break this relation and would require ownership transfer or longer-lived state.

## Detachment and ownership

If a task is explicitly detached, any borrowed parent state must remain valid independently.

That may require:

- moving ownership into the task
- shared ownership
- static storage
- another explicit lifetime mechanism

A VixC frontend could potentially diagnose detachment while borrowed state remains tied to the parent.

This is one of the clearest possible benefits of integrating ownership with async semantics.

## Failure propagation and ownership

Failure propagation changes control flow.

Consider:

```cpp
Resource resource;

auto value =
    try operation();

use(resource);
```

If `operation()` fails, control leaves the successful path.

`resource` must be destroyed.

The current Failure research already treats this as a lowering requirement.

An ownership model could strengthen analysis by ensuring all active owned resources have valid cleanup paths.

## Failure values and ownership

Failure values can own resources too.

For example:

```cpp
struct Error
{
    std::unique_ptr<DiagnosticData> data;
};
```

Then:

```cpp
fail make_error();
```

must transfer ownership correctly.

Propagation must not require copying the failure.

This is another reason VixC Failure semantics need to preserve ordinary C++ move behavior.

## Match and ownership

Future Match semantics may expose payloads from alternatives.

For example:

```text
ready(resource)
```

If the matched value owns `resource`, the branch must define whether the binding:

- borrows
- copies
- moves

the payload.

This cannot be decided safely by syntax rewriting alone.

Ownership research may provide the semantic vocabulary needed by Match.

## Choice payload ownership

A future Choice type may contain ownership-bearing alternatives.

For example:

```text
open(Resource)
closed
failure(Error)
```

Changing the active alternative may destroy the previous payload.

The frontend and backend must preserve normal C++ destruction semantics.

A Choice abstraction should not invent a second ownership model.

## Return by value

Returning by value is often the safest ownership boundary.

For example:

```cpp
Resource make_resource();
```

the result is transferred as a value.

Modern C++ copy elision and move semantics already support efficient ownership transfer.

VixC should not encourage borrowing where value semantics are simpler and safer.

Ownership annotations should justify themselves where value semantics are not appropriate.

## Return by reference

Returning references requires lifetime guarantees.

For example:

```cpp
const Config &
config();
```

may be safe if the referenced object has static or owner-bounded lifetime.

The caller needs to know that guarantee.

This is a natural place where semantic lifetime information could improve APIs.

## Static lifetime

Objects with static storage duration are broadly long-lived.

A reference to such an object does not have the same escape concerns as a reference to a local automatic object.

However, static lifetime introduces other problems:

- initialization order
- destruction order
- mutable global state
- thread safety

Lifetime safety alone does not make static state good design.

VixC ownership research should remain focused on validity rather than broader architecture policy.

## Thread lifetime

A thread can outlive the scope that created it.

C++ `std::jthread` improves this through RAII joining and stop-token integration.

This is useful evidence for VixC.

It shows that ownership and cancellation can be expressed through ordinary C++ types.

VixC should study whether language-level structured task lifetime provides value beyond these existing abstractions.

## `std::jthread`

`std::jthread` owns a thread and joins it during destruction.

This creates a structured resource relationship:

```text
jthread object lifetime
    owns thread lifetime
```

It also integrates cooperative stop requests.

This is conceptually close to several async research goals.

A VixC design should learn from this rather than inventing unrelated semantics.

## Locks

Locks are ownership-bearing resources.

For example:

```cpp
std::lock_guard<std::mutex> guard(mutex);
```

owns the lock state for its lifetime.

The destructor releases it.

Failure, exceptions, and ordinary scope exit naturally release the lock through RAII.

Any VixC control-flow transformation must preserve this.

Ownership semantics should not require a special lock model for ordinary RAII code.

## Transactions

Database or application transactions often use RAII-like wrappers.

However, transaction commit may be explicit while destruction rolls back.

This shows that resource ownership does not always mean successful finalization.

A lifetime model should distinguish:

```text
resource valid
resource ownership
resource semantic completion
```

These are different concepts.

## Manual resource protocols

Some C APIs use explicit acquisition and release:

```cpp
Handle *handle =
    open_handle();

close_handle(handle);
```

VixC could potentially improve safety around these APIs through RAII wrappers.

That may be better than adding general language syntax for every manual resource protocol.

The research should prefer ordinary C++ abstraction when it solves the problem cleanly.

## Foreign resources

FFI boundaries often expose pointers or handles with ownership conventions defined externally.

Examples include:

```text
caller owns
callee owns
borrowed until next call
valid until context destruction
must be released with function X
```

These contracts are excellent candidates for explicit metadata.

A future VixC ownership system may need annotations at foreign API boundaries more than inside idiomatic RAII C++.

## Ownership annotations for FFI

A C function might conceptually be annotated as:

```text
returns owned handle
```

and another as:

```text
consumes handle
```

while another returns:

```text
borrowed pointer tied to context
```

This information could allow VixC to reason about native resources without changing the underlying ABI.

No annotation syntax has been chosen.

## Double release

Manual ownership APIs can fail through double release.

For example:

```cpp
close(handle);
close(handle);
```

A frontend that understands ownership consumption could diagnose this.

However, ordinary RAII C++ already prevents many such bugs by moving release responsibility into destructors.

VixC should not recreate a linear type system merely to solve problems idiomatic C++ already avoids.

## Use after release

Similarly:

```cpp
close(handle);
use(handle);
```

could be diagnosed if the frontend understands that `close` invalidates the handle.

This again requires API semantic information.

The language cannot infer such behavior from an arbitrary function name.

## Nullability

Nullability is related to validity but separate from ownership.

A pointer may be:

```text
owned and nullable
borrowed and nullable
owned and non-null
borrowed and non-null
```

A future VixC model should not conflate:

```text
can be null
```

with:

```text
owns the object
```

These are independent properties.

## Optional references

C++ often represents optional borrowing with:

```cpp
T *
```

because references cannot be null.

Other abstractions may use:

```cpp
std::optional<std::reference_wrapper<T>>
```

A future language surface might improve this area, but ownership research should not automatically expand into redesigning nullability.

The semantic dimensions should remain separate.

## Aliasing

Multiple references may refer to the same object.

C++ permits broad aliasing patterns.

A strict exclusive-borrow model would therefore reject many existing C++ programs.

If VixC investigates borrowing restrictions, it must decide whether they are:

- optional stronger guarantees
- local analysis
- required language semantics

Compatibility strongly argues against silently imposing a completely different aliasing model on ordinary C++.

## Mutable aliasing

Two mutable references to the same object can be legal C++ depending on usage.

Data races and iterator invalidation introduce separate constraints.

A Rust-like rule forbidding multiple mutable aliases would be a major change to the C++ programming model.

VixC should not adopt such a rule without a very strong reason.

The initial research should focus on lifetime validity rather than global alias exclusivity.

## Borrow checking

The phrase "borrow checker" describes many possible systems.

VixC should avoid assuming that ownership research implies reproducing Rust's exact model.

Potential goals range from:

- detecting obvious escaping references
- checking callback lifetimes
- checking structured task borrowing
- validating FFI ownership contracts
- stronger local alias analysis

These are very different levels of ambition.

The first experiment should target a concrete class of bugs.

## Compatibility versus stronger mode

One possible direction is that ordinary C++ remains accepted while VixC offers stronger semantic contracts in selected code.

For example, an API could explicitly declare a lifetime relation that VixC then verifies.

This would preserve compatibility while allowing stronger guarantees where the programmer opts into them.

Whether this requires syntax, attributes, metadata, or types remains open.

## Attributes

C++ attributes may provide one possible annotation mechanism.

Conceptually:

```cpp
[[...]]
```

could describe ownership or lifetime contracts without changing core grammar significantly.

Potential benefits include compatibility with existing parsing infrastructure.

Potential drawbacks include verbosity, discoverability, and interaction with compilers that ignore unknown attributes.

VixC should compare attributes with dedicated syntax before deciding.

## Existing annotations

Compilers and tools already use ownership-related annotations in some ecosystems.

Static analyzers may understand:

- lifetime annotations
- ownership attributes
- nullability
- SAL-like contracts
- thread-safety annotations

VixC research should study those approaches before designing a new annotation system.

The problem may already be partially solved at tooling level.

## Clang lifetime analysis

Clang and related tooling already perform several lifetime and dangling-reference checks.

This is important evidence.

VixC should not duplicate mature static analysis merely to claim ownership support.

The relevant question is whether VixC semantic features such as Failure and structured async create new lifetime relationships that existing C++ tooling cannot see clearly.

## Static analysis versus language semantics

Not every useful diagnostic needs to become a language rule.

A static analyzer can warn about likely problems without making programs invalid.

This may be the right level for some ownership issues.

Language semantics should be reserved for guarantees that need to influence:

- typing
- control flow
- API contracts
- lowering
- interoperability
- runtime behavior

The research should distinguish analyzable style guidance from actual semantic contracts.

## Hard errors versus warnings

Lifetime analysis inevitably encounters uncertainty.

A strict rule may reject safe code.

A permissive rule may miss real bugs.

Possible policy categories include:

```text
provably invalid -> Error
suspicious but not proven -> Warning
unknown -> accept conservatively
```

The exact policy should depend on the semantic feature.

VixC should avoid making uncertain whole-program assumptions into hard errors.

## Separate compilation

Lifetime contracts become especially valuable across translation units.

If a function's implementation is unavailable, the caller can only reason from its declaration.

For example, a declaration might need to communicate:

```text
return borrows from parameter 1
callback is retained
argument ownership is consumed
```

These relationships must be part of a stable interface if the frontend is expected to enforce them separately.

## ABI independence

Ownership and lifetime contracts often do not need to change ABI.

For example, a reference remains:

```cpp
const T &
```

at the native boundary.

Additional semantic metadata could exist only for frontend analysis.

This is attractive because stronger checking can potentially be introduced without changing generated calling convention.

However, metadata must still survive headers, modules, and package boundaries.

## Metadata representation

If lifetime contracts do not appear directly in C++ types, VixC may need a way to preserve them across separate compilation.

Possible mechanisms include:

- generated annotations
- attributes
- side metadata
- modules
- encoded helper types
- source declarations processed by VixC

No representation has been selected.

This is a future architecture question.

## Templates

Templates make lifetime semantics significantly harder.

For example:

```cpp
template <typename T>
auto view(T &value);
```

the returned lifetime relationship may depend on the behavior of `T`.

Generic wrappers may preserve or transform borrowing.

A lifetime model must eventually work with dependent types without relying on string matching.

This likely requires deeper C++ semantic information.

## Concepts

C++ concepts may help describe ownership properties.

For example, generic code could require something conceptually equivalent to:

```text
OwningResource
BorrowedView
StableReferenceContainer
```

However, concepts operate through compile-time predicates over types and expressions.

They do not automatically encode lifetime relationships between individual values.

Concepts may complement, but not replace, lifetime semantics.

## Constness

Constness is not ownership.

A:

```cpp
const T &
```

is non-owning and read-only through that reference.

A:

```cpp
const std::unique_ptr<T>
```

still owns its resource.

Ownership research should not conflate mutability restrictions with lifetime responsibility.

## Interior mutability

A const object may still reference mutable external state or use internal synchronization.

This further demonstrates that ownership and mutability are different semantic dimensions.

VixC should preserve ordinary C++ const semantics.

## Memory safety

Ownership and lifetime analysis can prevent some memory-safety bugs.

However, VixC should not claim complete memory safety while ordinary C++ remains available.

Unsafe operations such as:

- raw pointer arithmetic
- invalid casts
- manual allocation
- undefined behavior
- foreign APIs

remain possible.

A stronger lifetime model may reduce certain bug classes without turning VixC into a memory-safe language in the absolute sense.

## Unsafe boundaries

If VixC eventually introduces stronger guarantees, it may need a way to identify code where those guarantees cannot be proven.

Possible approaches include:

- conservative escape to ordinary C++
- explicit unsafe regions
- trusted annotations
- no claim beyond analyzed constructs

No unsafe syntax is currently proposed.

The need should be demonstrated first.

## Diagnostics

Potential ownership and lifetime diagnostics include:

```text
reference escapes the lifetime of its owner
borrowed value captured by a task that may outlive it
returned view refers to local storage
resource used after ownership transfer
resource consumed more than once
callback retains reference to local state
detached computation borrows parent-owned object
borrow may be invalidated before next use
```

These diagnostics should use source-level ownership relationships rather than internal analysis terminology where possible.

## Returned local reference

A simple diagnostic target is:

```cpp
const Value &
make_value()
{
    Value value;
    return value;
}
```

A mature C++ compiler already diagnoses many forms of this problem.

VixC should not duplicate such checks unless its additional semantic context improves them.

The stronger opportunities lie where lifetime relationships cross VixC-specific constructs.

## Failure diagnostic example

Consider:

```cpp
const Value &operation() fails Error
{
    Value value;

    if (condition)
        fail Error{};

    return value;
}
```

The invalid returned reference is ordinary C++ lifetime behavior.

Failure itself is not the source of the lifetime bug.

The frontend should preserve enough source structure that the native compiler or lifetime analysis can diagnose the actual problem.

VixC should not blame Failure simply because it appears in the function.

## Async diagnostic example

A more VixC-specific future diagnostic could be:

```text
borrowed 'resource' cannot escape through detached computation
```

if the frontend knows that:

```text
task lifetime may exceed resource lifetime
```

This is the kind of cross-feature semantic value that may justify ownership research.

## Match diagnostic example

If Match moves an owned payload in one branch and later source attempts to use the original owning value, the frontend may eventually reason about that movement.

However, ordinary C++ move semantics already determine much of this behavior.

VixC should add diagnostics only where its Match semantics introduce information not otherwise visible.

## Tooling

Ownership information could improve editor tooling.

Potential features include:

- show owner of a borrowed value
- show lifetime dependency
- show where ownership moves
- show task captures that escape
- show invalidation points
- show resource cleanup path

Such tooling may be useful even where the language does not enforce a hard rule.

This again suggests separating semantic metadata from mandatory restrictions.

## Visualizing ownership

A developer debugging a complex lifetime problem may benefit from seeing relationships such as:

```text
view -> buffer
task -> parent scope
callback -> subscription
handle -> runtime context
```

This kind of tooling requires stable semantic relationships in the frontend.

It does not necessarily require new syntax for every relationship.

## IR representation

Most ownership and lifetime information may belong to semantic analysis rather than executable IR.

For example:

```text
borrow A from B
```

may exist only for validation.

However, some ownership operations affect lowering directly.

Examples include:

- explicit move
- ownership transfer into async task
- destruction point
- generated task frame storage
- future Choice payload movement

The IR should represent semantic operations that affect code generation while allowing purely analytical lifetime facts to remain in semantic metadata.

## Destruction in IR

A high-level frontend may eventually need to reason about scope exit and destruction explicitly.

Failure and stopped completion already create multiple exit paths.

A control-flow IR may need to know which objects remain live at each point.

This is not necessarily the same as generating destructor calls manually.

The C++ backend may still rely on normal C++ scope semantics.

The frontend needs enough information to avoid transformations that violate lifetime.

## Control-flow analysis

Lifetime is strongly connected to control flow.

A value may be valid on one branch and moved or destroyed on another.

For example:

```cpp
if (condition)
{
    consume(
        std::move(resource));
}

use(resource);
```

Whether the final use is semantically appropriate depends on the resource type and moved-from contract.

A precise ownership model therefore needs path-sensitive analysis for some cases.

This should not be attempted casually.

## Loops

Loops complicate ownership because values may be moved, reacquired, or invalidated across iterations.

For example:

```cpp
for (...)
{
    if (condition)
        owner =
            acquire();
}
```

A strong static model needs fixed-point reasoning.

This is another reason to begin with a narrow set of guarantees.

## Exceptions

Ordinary C++ exceptions alter control flow and trigger destruction.

Ownership analysis must account for exceptional exits where relevant.

A resource may be destroyed during stack unwinding even when normal source flow would continue.

The frontend should preserve normal exception semantics.

VixC should not require exceptions to become Failure merely to reason about lifetime.

## `noexcept`

`noexcept` affects exceptional control flow but not ordinary ownership transfer.

A lifetime model should understand it only where exception paths matter.

It should not use `noexcept` as a proxy for resource safety.

## Placement new and manual lifetime

C++ permits explicit object lifetime management through:

```cpp
placement new
```

explicit destructor calls, unions, allocators, and low-level storage manipulation.

These features make complete static lifetime analysis difficult.

VixC must decide whether:

- such code remains ordinary C++ outside stronger guarantees
- analysis becomes conservative
- explicit trusted boundaries are required

Compatibility means these techniques cannot simply disappear.

## Unions

C++ unions allow manual management of active object lifetime.

Modern code may use `std::variant` instead, but unions remain valid.

A general lifetime system must avoid making incorrect assumptions about which member is alive.

This is likely outside the first ownership experiment.

## Custom allocators

Allocator-aware types complicate assumptions about where storage comes from and how it is released.

Fortunately, value-level ownership semantics can often remain independent from allocator implementation.

VixC should prefer reasoning about object ownership contracts rather than raw allocation internals where possible.

## Arenas

Arena allocation changes lifetime relationships significantly.

Objects may all remain valid until the arena is destroyed.

Individual object destruction may be omitted or separated from storage reclamation.

A borrow tied to an arena can outlive local variables that created the object.

This is another reason lifetime must be related to semantic owners rather than stack scope only.

## Pools

Object pools may recycle storage.

A pointer can refer to memory that still exists but now contains a different logical object.

Simple address lifetime is therefore insufficient.

Generation counters and handle abstractions are common solutions.

VixC should not assume every stable address implies stable object identity.

## Handles

Many systems use handles rather than direct pointers.

A handle may remain syntactically valid after the underlying resource is destroyed but fail dynamically when used.

Ownership and validity may therefore be separate.

Static lifetime analysis can help only if the handle API exposes enough semantics.

## ECS and game systems

Entity-component systems often use IDs or handles whose validity is dynamic.

A strict reference-based lifetime model may not apply directly.

This reinforces the need for VixC ownership research to remain general enough for native application domains rather than assuming every resource is pointer-based.

## Network resources

A socket object may be alive while the remote connection is already closed.

This demonstrates another important distinction:

```text
object lifetime
```

is not the same as:

```text
resource operational validity
```

Ownership research should focus on whether the local object and resource ownership are valid, not whether an external system remains healthy.

Operational Failure belongs elsewhere.

## Ownership versus Failure

These concepts must remain separate.

A resource can be owned correctly and still fail operationally.

For example:

```text
socket object valid
network request fails
```

Likewise, a resource can be invalid due to lifetime misuse without any recoverable external Failure.

Mixing these categories would weaken both models.

## Ownership versus `none`

Absence also differs from ownership.

An optional owning pointer:

```cpp
std::unique_ptr<T>
```

can represent:

```text
owns T
```

or:

```text
owns nothing
```

The absence of an owned value does not itself describe a lifetime error.

The Outcome and ownership models may interact but should remain semantically distinct.

## Ownership versus stopped

Stopped computation may release owned resources as scopes unwind.

That does not make ownership part of cancellation.

The async model determines why control is stopping.

The ownership model ensures resources remain valid and are cleaned up correctly during that path.

## Borrowing across Failure

A borrowed reference valid before:

```cpp
try operation();
```

should remain valid afterward if its owner remains alive and no intervening operation invalidates it.

Failure propagation itself should not invent new invalidation.

However, because propagation may leave the current scope, borrows whose owners are local must not escape through generated Failure machinery.

Correct RAII lowering generally preserves this property.

## Borrowing across suspension

Suspension is more difficult.

A borrow held across an async suspension remains live while the computation is inactive.

The owner must remain valid for that entire duration.

This is one of the strongest motivations for integrating lifetime analysis with structured async ownership.

## Thread migration

If a suspended computation resumes on another thread, lifetime validity alone may not guarantee safety.

A borrowed object may have thread-affinity requirements.

This suggests that future async semantic analysis may need capabilities beyond lifetime.

Ownership research should not claim to solve scheduler-safety automatically.

## Lifetime and reflection

A future reflection system may expose ownership or lifetime contracts.

For example, tooling could ask:

```text
Does this return borrow from parameter 1?
Does this callable consume parameter 2?
```

If lifetime contracts become part of the semantic model, reflection may eventually need access to them.

This should not influence first syntax prematurely.

## Compile-time analysis

Ownership semantics are primarily compile-time information.

Most relationships should not require runtime tracking if the frontend can prove them.

However, some models such as shared ownership or weak handles are inherently dynamic.

VixC should not add runtime checks merely to force dynamic ownership into a static model.

## Runtime tracking

A stronger optional runtime mode could theoretically instrument:

- use after move
- dangling borrows
- invalid handles
- ownership transfer

But runtime instrumentation has cost and cannot detect every issue.

This belongs more naturally to diagnostics or sanitizer-style tooling than to baseline language semantics unless proven necessary.

## Sanitizers

AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer, and related tools already catch many runtime lifetime and concurrency bugs.

VixC should work well with them.

Static ownership semantics should complement sanitizers rather than attempt to replace them.

For example:

```text
compile-time contract
+
runtime sanitizer
```

may provide stronger development feedback than either alone.

## Diagnostic provenance

If a lifetime diagnostic involves generated code from Failure or async lowering, the frontend should report the original source relationship.

For example:

```text
borrowed resource is captured by a child computation that may outlive this scope
```

should point to:

- the borrow or capture
- the owner declaration
- the escaping task operation

Generated coroutine frame fields should not appear as the primary explanation.

## Source mapping

Most ownership diagnostics should ideally occur before backend emission.

Source maps remain important for ordinary native compiler diagnostics arising from generated code.

If generated code accidentally creates a lifetime bug, that is a compiler defect.

Tests should catch it.

The programmer should not be expected to debug internal generated temporaries.

## Real-program targets

Ownership research should be tested against realistic C++ patterns.

Useful examples include:

- returning `std::string_view`
- spans into containers
- callbacks capturing local references
- async tasks borrowing parent state
- detached work
- `std::unique_ptr` movement
- shared ownership
- iterator invalidation
- resource handles
- file and socket RAII
- database transactions
- custom arenas
- foreign C APIs
- game-engine handles

A model that works only for local references would not justify language changes.

## First useful experiment

The first ownership experiment should be much narrower than a complete ownership system.

One promising target is structured async borrowing.

For example:

```text
parent scope owns resource
child task borrows resource
child cannot escape parent scope
```

Then test an invalid case:

```text
parent scope owns resource
child borrows resource
child is detached
parent scope exits
```

The frontend should be able to identify that the borrowed resource may not remain valid.

This experiment has several advantages.

It directly connects to VixC async research.

It addresses a real C++ lifetime problem.

It does not require replacing pointers, references, or RAII.

It tests whether semantic lifetime relationships provide value beyond existing types.

## Another useful experiment

A second experiment could involve returned views.

For example:

```cpp
std::string_view prefix(
    const std::string &text);
```

with a semantic contract:

```text
return borrows from text
```

Then test calls where:

- `text` clearly outlives the view
- the view escapes beyond `text`
- a temporary is passed
- the underlying string is invalidated

This would reveal how much C++ semantic information VixC needs before lifetime contracts become practical.

## What should not be the first experiment

The research should not begin by attempting to enforce a complete ownership type system across all C++.

That would immediately encounter:

- raw pointers
- unions
- placement new
- custom allocators
- shared ownership
- templates
- callbacks
- foreign code
- manual lifetime
- existing aliasing patterns

The resulting complexity would obscure whether there is a smaller useful contribution.

## Relationship with existing C++ tooling

Before adding language syntax, VixC should compare its target diagnostics with:

- GCC warnings
- Clang warnings
- clang-tidy
- Clang static analyzer
- sanitizers
- lifetime-analysis proposals and experiments

If existing tooling already provides a robust answer, VixC may only need integration rather than new semantics.

The language should own only what needs to compose directly with VixC-specific features.

## Potential semantic vocabulary

The research may eventually need concepts such as:

```text
owner
borrow
consume
escape
lifetime dependency
invalidate
detach
```

These are conceptual terms.

They are not proposed source keywords.

A language-level surface should be introduced only after experiments demonstrate which relationships need to be expressed explicitly.

## Open semantic questions

The ownership research must eventually answer questions such as:

- What exactly does VixC mean by ownership?
- Which C++ types imply ownership automatically?
- Which types are treated as borrows?
- Can arbitrary user-defined types declare ownership semantics?
- Can a value borrow from multiple owners?
- How are dynamic shared lifetimes represented?
- Can borrows cross async suspension?
- Can borrows enter detached tasks?
- How are invalidation rules represented?
- How much can be inferred?
- Which relationships require explicit annotations?
- Which violations are errors versus warnings?

These questions should remain open until concrete experiments provide evidence.

## Open syntax questions

No ownership syntax is currently selected.

Possible implementation mechanisms include:

- no new syntax
- attributes
- API metadata
- contextual keywords
- type traits
- concepts
- dedicated lifetime annotations

The correct surface depends on the semantic information that cannot already be obtained from C++.

## Open interoperability questions

A useful ownership model must coexist with ordinary libraries.

Questions include:

- How are third-party lifetime contracts supplied?
- Can headers expose VixC metadata without requiring VixC consumers?
- Can ordinary C++ ignore the metadata safely?
- How are contracts distributed in binary packages?
- How do modules preserve them?
- Can foreign C APIs be annotated externally?

These are important if ownership semantics are to work beyond one source file.

## Open template questions

Templates remain especially difficult.

The research needs to understand:

- dependent lifetime contracts
- ownership traits
- generic returned views
- forwarding references
- perfect forwarding
- variadic APIs
- generic callbacks
- allocator-dependent lifetime

Any design that cannot handle ordinary modern C++ templates would have limited usefulness.

## Perfect forwarding

Perfect forwarding can preserve value category through generic code:

```cpp
std::forward<T>(value)
```

A VixC ownership model must not interfere with this.

It also means ownership transfer may be dependent on template instantiation.

The frontend will eventually need real semantic type and value-category information.

## Rvalue references

Rvalue references do not automatically mean ownership.

For example:

```cpp
T &&
```

may represent:

- temporary object
- forwarding reference
- explicit movable source
- reference returned by another object

A lifetime model should not use syntax alone to infer ownership.

## `std::move`

`std::move` does not move anything by itself.

It changes value category and allows move operations to be selected.

A VixC diagnostic saying:

```text
ownership moved here
```

must therefore refer to the actual consuming operation, not merely to the appearance of `std::move`.

This is another example where real C++ semantic analysis is required.

## Lifetime of coroutine frames

If VixC async uses C++ coroutines, local state that survives suspension becomes part of the coroutine frame.

The frame owns or references that state according to transformed semantics.

Lifetime analysis must therefore understand that lexical locals can outlive the active call stack.

This makes coroutine integration an important future test.

## Capturing `this`

A common asynchronous lifetime problem is capturing:

```cpp
this
```

in work that may outlive the object.

For example:

```cpp
start_async(
    [this]
    {
        use_member();
    });
```

If the task escapes object lifetime, the pointer becomes invalid.

A structured ownership model could diagnose this when task lifetime exceeds the owning object's guaranteed lifetime.

This is a practical and valuable target.

## `shared_from_this`

C++ applications often solve escaping `this` lifetime by capturing shared ownership.

For example:

```cpp
auto self =
    shared_from_this();
```

then moving `self` into the callback.

This demonstrates that existing C++ can already solve the problem.

VixC's contribution would be identifying when the unsafe borrow escapes and helping make the required ownership transfer explicit.

## Cycles

Shared ownership can create cycles.

A lifetime model that recommends shared ownership automatically could introduce leaks.

Therefore, VixC diagnostics should not blindly suggest:

```text
use shared_ptr
```

whenever a borrow escapes.

Ownership design remains an application decision.

The frontend should explain the violated lifetime relation rather than prescribe one universal repair.

## Leaks

Memory leaks are ownership failures, but static detection is difficult in general.

RAII already prevents many leaks.

VixC should not promise complete leak detection.

Its strongest opportunities are semantic relationships it already understands through VixC constructs.

## Double ownership

Two independent unique owners referring to the same raw resource can cause double release.

The compiler cannot infer this from arbitrary constructor calls without API contracts.

Explicit ownership metadata at foreign or low-level boundaries may help.

Again, this is more promising than replacing ordinary RAII.

## Ownership and API design

One potential benefit of explicit ownership semantics is better API documentation.

A function declaration could make clear whether it:

```text
borrows
consumes
retains
returns borrowed
returns owned
```

This information is useful even before enforcement.

A VixC semantic model could therefore improve both diagnostics and interface understanding.

## Documentation tooling

If ownership contracts become structured metadata, documentation generators could display them.

For example:

```text
parameter buffer: borrowed for call
parameter callback: retained
return value: borrows from buffer
```

This is more precise than relying on prose conventions.

Tooling value may help justify semantic contracts even when enforcement is conservative.

## Ownership and language identity

VixC should not become a language whose primary identity is:

```text
C++ with a borrow checker
```

unless research eventually proves that direction is necessary.

The current project has broader goals around coherent C++ application semantics.

Ownership is one research area among Failure, choice, async, compile-time behavior, and composition.

The design should therefore solve concrete integration problems rather than importing another language's identity wholesale.

## What would justify ownership semantics

Direct VixC ownership and lifetime semantics would be justified if experiments demonstrate substantial improvements such as:

- preventing borrowed state from escaping structured async scopes
- detecting invalid callback captures
- making returned-view lifetimes explicit across API boundaries
- improving diagnostics around resource transfer
- allowing tooling to explain owner-borrower relationships
- composing cleanly with Failure and cancellation
- preserving ordinary C++ RAII and interoperability

The value should come from semantic coherence, not from adding ownership vocabulary for its own sake.

## What would argue against language-level ownership

The direction should be reconsidered if:

- existing C++ types and static analysis already solve the target problems sufficiently
- useful guarantees require replacing too much ordinary C++
- annotations become pervasive
- templates make contracts impractical
- false positives overwhelm real diagnostics
- third-party libraries require extensive manual metadata
- the model conflicts with common aliasing patterns
- compatibility requires so many escape hatches that guarantees become meaningless

In that case, ownership may remain primarily a tooling concern.

## What must not become accidental specification

The following are explicitly provisional:

- whether VixC has ownership keywords
- whether VixC has explicit borrow syntax
- whether references are treated as borrows automatically
- whether raw pointers receive ownership meaning
- whether lifetime annotations use attributes
- whether borrowing follows lexical regions
- whether non-lexical lifetime inference is required
- whether aliasing restrictions exist
- whether moved-from use is restricted
- whether ownership violations are Errors
- whether detached tasks require ownership transfer
- whether lifetime metadata affects ABI

Implementation experiments must not silently freeze these decisions.

## Relationship with frontend architecture

Ownership and lifetime analysis should use the same VixC frontend architecture.

Relevant information may pass through:

- source management
- syntax
- semantic declarations
- type information
- control-flow analysis
- semantic metadata
- diagnostics

Most ownership facts should probably not require backend-specific IR.

Where ownership affects code generation, such as moving values into async frames, the semantic information should remain explicit through lowering.

The backend should not infer ownership from generated text.

## Dependency on richer C++ semantics

Unlike the first Failure experiment, serious lifetime analysis quickly requires deeper understanding of C++ types and expressions.

Important information includes:

- declaration identity
- reference types
- value categories
- constructors
- destructors
- move operations
- templates
- temporary materialization
- storage duration
- overload resolution

The current lightweight VixC parser does not provide all of this.

Ownership research may therefore be one of the strongest arguments for eventual integration with a mature C++ semantic frontend.

That decision should still be driven by concrete requirements.

## Research priority

Ownership and lifetime work should not interrupt completion of the first Failure vertical slice.

It should also remain coordinated with async research.

The most promising initial ownership questions appear where VixC-specific computation semantics create new lifetime relationships that ordinary C++ syntax alone does not expose clearly.

Structured async borrowing is a strong example.

## Immediate research questions

Before designing syntax, the ownership work should answer a smaller set of questions.

First, which lifetime bugs are not already handled adequately by existing compilers, static analyzers, sanitizers, and idiomatic RAII?

Second, which of those bugs arise directly from VixC features such as Failure, async, cancellation, or future Match semantics?

Third, can task structure provide useful lifetime guarantees without a general borrow checker?

Fourth, what semantic information must an API expose for the frontend to know whether a borrow escapes?

Fifth, can returned-view relationships be represented without changing ABI?

Sixth, how can third-party C++ libraries participate without being rewritten?

Seventh, how much type-system information is required before the analysis becomes reliable?

These questions should be answered before introducing ownership syntax.

## Research discipline

Ownership and lifetime work can easily expand into a redesign of C++.

VixC should distinguish:

```text
existing C++ semantic rule
additional static analysis
VixC-specific semantic contract
library convention
runtime validity check
backend implementation detail
```

These layers are different.

For example:

```text
std::unique_ptr moves ownership
```

is already ordinary C++ behavior.

```text
this child task cannot outlive the parent resource it borrows
```

could be a VixC-specific semantic guarantee.

```text
this raw pointer probably dangles
```

may be a static-analysis warning.

The language should only own the second category when evidence supports it.

## Current position

The current direction is not to replace C++ ownership.

RAII, destructors, references, smart pointers, value semantics, and move semantics remain the foundation.

The research focuses on relationships that ordinary type syntax often does not communicate clearly enough, especially across:

- asynchronous computation
- callbacks
- returned views
- resource transfer
- separate API boundaries

The strongest near-term hypothesis is that structured computation lifetime can provide meaningful ownership guarantees without requiring VixC to impose a universal new ownership model on C++.

No ownership syntax has been selected.

No borrow checker has been committed.

No new pointer model has been proposed.

The next useful evidence should come from small experiments that connect lifetime analysis to real VixC computation semantics while preserving ordinary C++ as much as possible.
