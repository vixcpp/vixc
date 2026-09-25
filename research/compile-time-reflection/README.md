# Compile-Time and Reflection Research

This directory contains research for possible VixC language semantics around compile-time computation, program introspection, metadata, generation, and reflection.

The goal is not to replace C++ templates, `constexpr`, `consteval`, concepts, macros, or existing compiler capabilities.

C++ already provides powerful compile-time mechanisms.

The research question is narrower:

> Can VixC provide a more coherent model for asking questions about programs and performing compile-time work without creating another disconnected metaprogramming system?

This work is exploratory.

No reflection syntax, compile-time block, metadata model, or generation mechanism described here should be considered stable.

## Motivation

C++ supports substantial compile-time programming.

A programmer can use:

```cpp
constexpr
```

```cpp
consteval
```

```cpp
if constexpr
```

templates, concepts, type traits, parameter packs, overload resolution, and compiler-specific facilities.

These mechanisms make sophisticated compile-time systems possible.

The difficulty is not lack of power.

The difficulty is that compile-time intent can be distributed across several different mechanisms.

For example, answering questions such as:

```text
What fields does this type contain?
Which functions have this property?
Does this declaration have this annotation?
Generate behavior for every member satisfying a condition.
Validate an application structure before runtime.
```

may require a mixture of:

```text
templates
traits
macros
generated code
external tools
compiler extensions
build-time scripts
```

VixC research asks whether selected forms of compile-time introspection and transformation can be represented more coherently inside the frontend.

## Compile time is not one phase

The phrase:

```text
compile time
```

can refer to several different activities.

Examples include:

- parsing source
- resolving declarations
- type checking
- evaluating constant expressions
- template instantiation
- semantic validation
- code generation
- external source generation before compilation
- build-system configuration

These are not interchangeable.

A VixC compile-time model should define which phase owns a computation and what semantic information is available there.

For example, asking:

```text
What members does this type contain?
```

requires resolved type information.

Asking:

```text
What tokens appear in this source file?
```

is a different operation at a different abstraction level.

The language should not call both of them reflection without distinguishing their semantics.

## Compile-time computation and reflection are separate

Compile-time computation means evaluating something before ordinary runtime execution.

Reflection means obtaining structured information about program entities.

They often work together, but they are not the same feature.

For example:

```cpp
consteval int answer()
{
    return 42;
}
```

is compile-time computation without reflection.

Likewise, a reflection mechanism could theoretically expose metadata that is later consumed at runtime.

VixC should keep these concepts separate enough that each has a precise role.

## Existing C++ compile-time programming

C++ already provides several compile-time facilities.

### `constexpr`

`constexpr` allows values and functions to participate in constant evaluation when language requirements are satisfied.

For example:

```cpp
constexpr int square(int value)
{
    return value * value;
}

constexpr int result =
    square(4);
```

This is ordinary C++ and should remain ordinary C++.

VixC does not need another syntax merely to evaluate arithmetic before runtime.

### `consteval`

`consteval` requires a function invocation to produce a constant expression in contexts governed by the C++ language rules.

For example:

```cpp
consteval int identifier()
{
    return 7;
}
```

This already provides a strong source-level distinction between ordinary execution and immediate compile-time execution.

VixC should understand what semantic gap remains before introducing another compile-time function category.

### `if constexpr`

C++ can select branches during compilation:

```cpp
if constexpr (condition)
{
    // ...
}
else
{
    // ...
}
```

This is a useful and established mechanism.

VixC should not introduce another conditional syntax merely because the branch is evaluated at compile time.

### Templates

Templates provide compile-time abstraction and specialization.

They can express:

- generic algorithms
- type transformations
- compile-time recursion
- structural composition
- dispatch
- generated declarations through instantiation

Templates are extremely powerful.

They can also make simple intentions difficult to recognize when the implementation depends on several layers of specialization and type machinery.

VixC research should focus on semantic gaps rather than trying to replace templates broadly.

### Concepts

Concepts allow generic code to state requirements on template arguments.

For example:

```cpp
template <typename T>
concept Sized =
    requires(T value)
    {
        value.size();
    };
```

Concepts improve the expression of compile-time constraints significantly.

A future VixC reflection model should compose with concepts rather than duplicate them.

### Type traits

C++ type traits allow compile-time questions about types.

For example:

```cpp
std::is_integral_v<T>
```

or:

```cpp
std::is_move_constructible_v<T>
```

These are forms of structured compile-time introspection.

They demonstrate that many useful reflection-like questions can already be represented through library interfaces.

The limitation is that arbitrary program structure is not uniformly exposed this way.

### Preprocessor macros

The preprocessor can inspect and transform tokens before normal C++ parsing.

For example:

```cpp
#define FIELD(name) int name;
```

Macros remain useful for conditional compilation, portability, repetitive declarations, and build integration.

However, the preprocessor generally operates without semantic type information.

It does not know that a token sequence represents:

```text
a class
a field
a function
a type
an overload
a template
```

unless the programmer manually encodes those distinctions.

A semantic reflection model should operate at a richer level than token substitution.

## Reflection should expose meaning, not source spelling alone

A useful reflection system should operate on semantic program entities.

For example:

```cpp
using UserId = int;
```

creates a semantic relationship that is more meaningful than merely observing the source tokens:

```text
using
UserId
=
int
;
```

Likewise, a member function has information such as:

- declaration identity
- name
- type
- parameter types
- return type
- attributes
- access
- qualifiers
- enclosing declaration

A reflection system should expose the information required by the language contract rather than requiring users to reconstruct semantics from strings.

## Reflection is not text parsing

A reflection API based primarily on source text would make semantic programming fragile.

For example:

```text
Does this field have type "int"?
```

should not depend on comparing a source string to:

```text
"int"
```

because equivalent C++ types may have different spellings through:

- aliases
- qualification
- templates
- namespace aliases
- dependent names

Reflection should work with semantic identities where possible.

Source spelling can remain available for diagnostics and tooling.

It should not become the primary semantic representation.

## Reflection entities

A future VixC reflection model may need a semantic representation of program entities.

Potential categories include:

```text
type
declaration
function
parameter
field
enumerator
template
namespace
attribute
```

These names are provisional.

The first experiment should not expose every possible C++ entity.

A smaller model may be easier to validate.

## Reflection values

A reflected entity is not necessarily an ordinary runtime object.

For example, reflecting a field may produce compile-time metadata representing:

```text
declaration identity
field type
field name
```

The language must decide whether reflected entities are:

- dedicated compile-time values
- opaque frontend handles
- ordinary values of library types
- template arguments
- another category

This decision affects the entire reflection model.

## Semantic identity

Reflection needs stable identity.

Two references to the same declaration should be recognizable as the same semantic entity even if they originate from different source locations.

For example:

```cpp
struct User
{
    int id;
};
```

reflection over `User::id` should identify that declaration rather than merely returning:

```text
"id"
```

as a string.

String names are useful for presentation.

They are insufficient for semantic identity.

## Reflection and source locations

Reflected declarations may also expose source provenance.

For example, tooling or compile-time diagnostics may need to know where a declaration originated.

VixC already has:

```cpp
SourceLocation
SourceRange
```

for frontend provenance.

A future reflection model could build on the same source identity system.

However, source location should remain metadata.

It should not define declaration identity.

## Compile-time values

VixC should distinguish between:

```text
ordinary runtime value
constant C++ value
reflection metadata
compiler semantic object
```

These categories may have different capabilities.

For example, a reflection entity may be available only during frontend processing.

A `constexpr int` can become an ordinary generated C++ constant.

The frontend should not force every compile-time semantic value into one runtime-representable type.

## Compile-time execution

One possible VixC direction is a compile-time execution environment capable of evaluating frontend-understood operations.

Before introducing such a mechanism, several questions must be answered.

What code may execute?

What state may it access?

Can it allocate?

Can it perform I/O?

Can it inspect the filesystem?

Can it access environment variables?

Can it execute arbitrary native code?

Can it mutate compiler state?

Can it generate declarations?

Can it produce diagnostics?

These capabilities have major consequences for determinism, security, builds, caching, and reproducibility.

## Pure compile-time computation

The simplest model is pure compile-time computation.

Conceptually:

```text
inputs
    -> deterministic computation
    -> compile-time value
```

with no external side effects.

This model is comparatively easy to reason about.

The same source and configuration can produce the same result.

Caching becomes straightforward.

Parallel compilation becomes safer.

VixC should prefer pure compile-time semantics unless real use cases require broader capabilities.

## Compile-time I/O

Allowing arbitrary filesystem or network access during semantic compilation changes the build model significantly.

For example:

```text
compile-time code reads file X
```

means file X becomes an implicit compilation input.

If that dependency is not represented explicitly, incremental builds can become incorrect.

Likewise:

```text
compile-time network request
```

would make builds depend on external availability and mutable remote state.

This conflicts with deterministic and offline-capable builds.

VixC should therefore treat compile-time I/O as a separate capability question rather than a default property.

## Environment access

Reading:

```text
environment variables
current directory
current time
machine configuration
```

during compile-time execution also introduces hidden inputs.

A deterministic frontend needs those inputs represented explicitly if they affect generated output.

The language should not silently inherit the entire host process environment into compile-time semantics.

## Determinism

Compile-time behavior should be deterministic wherever possible.

Given equivalent:

```text
source
frontend version
configuration
declared inputs
```

the frontend should produce equivalent semantic results.

This is important for:

- reproducible builds
- caching
- incremental compilation
- testing
- distributed compilation
- debugging

Nondeterministic compile-time execution can make failures extremely difficult to reproduce.

## Build dependencies

If compile-time computation reads external data, that data becomes part of the build graph.

For example:

```text
generate enum from schema.json
```

is reasonable if:

```text
schema.json
```

is an explicit dependency.

It is dangerous if the compiler discovers the dependency invisibly at execution time.

VixC should coordinate compile-time inputs with the build system rather than hiding them.

This is particularly important for integration with Vix.cpp.

## Vix.cpp boundary

VixC owns language semantics.

Vix.cpp owns application and build lifecycle.

If a VixC compile-time operation requires an external file, package, generated input, or platform configuration, Vix.cpp may be responsible for making that dependency available.

VixC should not absorb arbitrary project discovery or dependency resolution into the language frontend.

The integration boundary should remain explicit.

## Reflection without code generation

Reflection is useful even if it cannot generate new declarations.

For example, a frontend could validate that every field satisfies a property.

Conceptually:

```text
for each field in T:
    require field is serializable
```

The result may simply be:

```text
program valid
```

or a diagnostic.

This is an important starting point because it allows reflection to prove useful before VixC commits to metaprogrammatic declaration generation.

## Reflection for diagnostics

One of the most promising uses of semantic reflection is compile-time validation.

For example, an application framework may need to verify:

```text
every route handler has an accepted signature
```

or:

```text
every persisted field has supported metadata
```

Today this may be implemented through templates, macros, code generation, or runtime registration.

A semantic frontend could potentially explain violations more directly.

The value would come from diagnostics and coherent program understanding rather than syntax alone.

## Reflection for metadata

Reflection may allow program metadata to be generated from declarations.

Potential examples include:

- serialization descriptions
- command-line interfaces
- RPC schemas
- database mapping
- test discovery
- dependency injection metadata
- API documentation
- editor metadata

These use cases often involve inspecting declarations rather than changing their fundamental semantics.

They may provide useful early experiments.

## Serialization

Serialization is a common reflection example.

Given:

```cpp
struct User
{
    int id;
    std::string name;
};
```

a reflection system could potentially enumerate:

```text
id
name
```

and their semantic types.

A serializer could then be generated or selected.

However, serialization policy includes questions not answered by structural reflection alone:

- field names
- ignored fields
- versioning
- optional fields
- custom codecs
- private members
- constructors
- invariants

Reflection provides information.

It does not automatically define the domain policy.

## Reflection should not force structural programming everywhere

Not every abstraction should be automatically derived from class structure.

For example, exposing every private field for serialization could violate encapsulation.

A reflection model needs access rules.

Questions include:

- Can reflection inspect private declarations?
- Does reflection obey normal access control?
- Can a type explicitly expose metadata?
- Can privileged compiler operations bypass access?
- Is reflection capability-dependent?

These decisions affect both safety and library design.

## Access control

A conservative initial rule would preserve C++ access control.

Compile-time reflection should not automatically make:

```cpp
private
```

members publicly observable.

However, some metaprogramming use cases require implementation-level introspection.

Possible models include explicit opt-in or privileged contexts.

No final rule is selected.

## Metadata and attributes

C++ attributes provide a possible mechanism for attaching metadata to declarations.

For example:

```cpp
[[something]]
struct User
{
};
```

A VixC reflection system could potentially expose structured attributes.

This may avoid introducing a second annotation system.

However, VixC must consider how unknown attributes interact with ordinary C++ compilers and generated source.

## VixC-specific attributes

A VixC frontend could recognize attributes with VixC semantics.

Conceptually:

```cpp
[[vixc::...]]
```

This is only an implementation possibility.

Attributes may be appropriate for optional metadata because they integrate naturally with declarations.

They may be less suitable for core control-flow semantics such as Failure.

Compile-time reflection research should determine where attributes are the correct layer.

## User-defined metadata

A useful system may allow libraries to attach metadata without requiring every concept to become a language keyword.

For example, a library could mark a field as:

```text
persistent
remote
deprecated
indexed
```

The frontend might expose that metadata through reflection.

This creates an extensibility mechanism.

It also raises questions about namespacing, type safety, and whether metadata values are merely strings or structured compile-time values.

## Typed metadata

String metadata is easy to create but difficult to reason about semantically.

For example:

```text
"timeout=5000"
```

requires parsing and convention.

Typed metadata could expose:

```text
timeout: Duration
```

or another structured value.

This improves tooling and validation but requires a compile-time value model.

The research should determine whether typed metadata is needed before designing an annotation language.

## Reflection and templates

Templates already perform compile-time structural programming.

A reflection model should compose naturally with them.

For example, generic code may reflect:

```cpp
T
```

only after instantiation determines the actual type.

Questions include:

- When is reflection evaluated?
- Can reflection produce template arguments?
- Can reflected entities be stored in parameter packs?
- Can reflection participate in concepts?
- Can reflection results be used for specialization?

These questions strongly connect reflection to the C++ type system.

## Dependent reflection

Reflecting a dependent type before template instantiation may not reveal its final declarations.

For example:

```cpp
template <typename T>
void inspect();
```

reflection over `T` may need to remain dependent until instantiation.

A mature design needs a representation for dependent reflection values.

The first experiment should probably avoid this complexity.

## Reflection and concepts

Concepts may provide a natural consumer of reflection.

For example, a concept could require that a type contains a particular semantic property.

Conceptually:

```text
reflect T
verify members satisfy condition
```

However, many such constraints can already be expressed through `requires`.

VixC should test where reflection adds information unavailable through existing expression-based constraints.

## Reflection and overloads

A reflected function name may refer to an overload set.

For example:

```cpp
void process(int);
void process(std::string);
```

Reflecting:

```text
process
```

is ambiguous unless the model can represent an overload set or requires a specific declaration.

This demonstrates why reflection must operate on semantic entities rather than names alone.

## Reflection and templates themselves

Should reflection expose:

```text
template declaration
template parameters
constraints
specializations
```

This would enable powerful tooling and generation.

It would also greatly expand the semantic surface.

The first reflection model should probably focus on ordinary resolved declarations before template metaprogram reflection.

## Reflection and aliases

Consider:

```cpp
using UserId = int;
```

Should reflection of `UserId` preserve alias identity or immediately expose `int`?

Both pieces of information can matter.

For diagnostics and API tooling, the alias name is important.

For type compatibility, the underlying type matters.

A reflection model may therefore need both:

```text
declared identity
semantic canonical type
```

rather than choosing one globally.

## Reflection and typedefs

The same issue applies to:

```cpp
typedef int UserId;
```

Source-level names and semantic type identity serve different purposes.

VixC should retain both when useful.

## Reflection and namespaces

Program structure includes namespaces.

A future reflection system may want to enumerate declarations in a namespace.

That creates questions around:

- multiple namespace declarations
- inline namespaces
- using directives
- modules
- visibility
- separate translation units

Namespace reflection is likely beyond the first experiment.

## Reflection and translation units

A frontend typically analyzes one translation unit at a time.

Questions such as:

```text
enumerate every type in the application
```

are not naturally translation-unit-local.

Answering them may require:

- build-system indexes
- modules
- link-time metadata
- external databases
- whole-program analysis

VixC should distinguish local reflection from whole-program reflection.

## Local reflection

Local reflection operates on semantic entities visible in the current compilation context.

This is the simpler model.

For example:

```text
reflect this type
reflect this function
reflect members of this class
```

Such operations can potentially remain deterministic and compatible with separate compilation.

The first reflection experiment should probably remain local.

## Whole-program reflection

Whole-program reflection could enable:

```text
find every route
find every test
find every implementation of an interface
```

This is attractive for application frameworks.

It is also much more difficult.

Separate compilation means no individual translation unit necessarily knows the complete program.

A build-system or link-stage registry may be required.

This may belong to Vix.cpp or another tooling layer rather than core source semantics.

## Reflection and modules

C++ modules may expose semantic declarations through module interfaces.

A future VixC reflection model needs to determine whether imported declarations can be reflected and what information remains available.

This may provide a cleaner semantic boundary than textual headers.

However, VixC should not require modules as a prerequisite for reflection unless evidence demands it.

## Reflection and private implementation

Whole-program reflection could accidentally expose implementation details from dependencies.

Libraries may not want consumers to inspect every internal declaration.

The reflection model should respect visibility and encapsulation boundaries.

This is another argument for capability-based or local reflection rather than unrestricted compiler database access.

## Generative metaprogramming

Reflection becomes significantly more powerful if compile-time code can generate new declarations.

Conceptually:

```text
inspect fields
generate function
```

or:

```text
inspect service
generate client
```

This can eliminate external generators and macros.

It also introduces major complexity.

Generated declarations must participate in:

- name lookup
- overload resolution
- templates
- diagnostics
- source locations
- ordering
- modules
- incremental compilation

VixC should not commit to declaration generation merely because reflection exists.

## Reflection without injection

A useful initial model could intentionally separate:

```text
reflection
```

from:

```text
declaration injection
```

Reflection can first provide semantic information to:

- validate
- compute constants
- select existing code
- create runtime metadata

Only later should VixC investigate whether generating new declarations is necessary.

This keeps the initial semantic surface much smaller.

## Code generation

VixC already has a backend that generates ordinary C++.

That does not mean compile-time user code should be allowed to emit arbitrary C++ strings.

String-based code generation would bypass:

- semantic identity
- type checking before generation
- precise diagnostics
- structured source mapping
- frontend invariants

If VixC eventually supports user-driven generation, structured semantic generation is preferable to arbitrary textual emission.

## String-generated source

There may still be legitimate cases for textual source generation.

Existing build systems already use it extensively.

The question is whether that belongs inside VixC language semantics.

A compiler feature such as:

```text
emit this arbitrary string as C++
```

would make semantic guarantees difficult.

It should not become the default metaprogramming model.

## Declaration injection

A structured injection model would create semantic declarations directly.

Conceptually:

```text
create function declaration
create field declaration
create implementation
```

This is much stronger than generating strings.

It also means the frontend must define:

- when injected declarations become visible
- whether they can affect earlier lookup
- whether generation can recurse
- how conflicts are diagnosed
- whether generated declarations can themselves be reflected
- when the process reaches a fixed point

These questions must be resolved before introducing injection.

## Compile-time phases

Generative reflection creates phase-ordering problems.

For example:

```text
reflect declarations
generate declaration
reflect again
generate more declarations
```

When does compilation stop?

Can generation observe declarations generated later?

Can generated code alter overload resolution that affected the generator itself?

A language must define clear phase boundaries to avoid unstable semantics.

## Fixed-point generation

One theoretical model repeatedly processes generated declarations until no new declarations appear.

This can become difficult to reason about and may fail to terminate.

Another model allows generation only in explicit phases.

The first VixC reflection experiment should avoid this problem by not supporting arbitrary declaration injection.

## Compile-time termination

Any compile-time execution system needs a policy for non-termination.

Templates and constant evaluation already encounter resource limits.

A VixC compile-time evaluator may need limits on:

- recursion
- instruction count
- memory
- generated declarations
- diagnostics
- expansion depth

These limits should produce understandable frontend diagnostics rather than hanging the build indefinitely.

## Resource limits

Compile-time code consumes developer resources.

A program that requires enormous compile-time memory or CPU may be valid semantically but impractical.

VixC should distinguish:

```text
invalid compile-time program
```

from:

```text
compile-time resource limit exceeded
```

This distinction matters for diagnostics and tooling.

## Compile-time Failure

A compile-time operation can itself fail.

This raises an important connection with the Failure research.

Suppose reflection expects a declaration with a certain property and does not find it.

Is that:

```text
recoverable compile-time failure
```

or:

```text
program diagnostic
```

or:

```text
none
```

The answer depends on context.

A compile-time query may legitimately return no result.

A required invariant may need to produce a diagnostic.

VixC should not automatically reuse runtime Failure semantics for compiler errors.

## Compile-time Outcome

The broader Outcome model could theoretically apply to compile-time computations.

For example:

```text
success(value)
none
failure(E)
```

But compile-time Failure has different consequences from runtime Failure.

If an unhandled compile-time failure prevents code generation, compilation cannot continue normally.

The language may therefore need a distinct boundary between:

```text
compile-time values representing failure
```

and:

```text
frontend diagnostics that invalidate the program
```

This remains open.

## Compile-time diagnostics

One powerful reflection capability is producing diagnostics based on semantic information.

For example:

```text
this type cannot be persisted because field 'socket' has no persistence representation
```

Such diagnostics could be much clearer than template-instantiation failures.

A future API may need structured operations for:

```text
error
warning
note
```

during compile-time semantic evaluation.

These should integrate with the normal `DiagnosticEngine`.

## Diagnostic source locations

Compile-time diagnostics should be able to point to reflected declarations.

For example, a validation failure may need to point directly to:

```cpp
Socket socket;
```

inside a struct.

This requires reflected entities to preserve source provenance.

The existing `SourceRange` model provides a foundation.

## Diagnostic determinism

Compile-time metaprograms must not emit diagnostics in nondeterministic ordering.

If reflection enumerates declarations, the enumeration order should be defined or diagnostics should be sorted deterministically.

This matters for tests and reproducible builds.

## Reflection ordering

If a type has fields:

```cpp
int id;
std::string name;
bool enabled;
```

does reflection enumerate them in declaration order?

For many use cases, this is the natural expectation.

However, every reflected entity category needs a defined ordering policy.

The language should not expose unstable internal compiler container order.

## Names

Reflection may expose source-level names.

Questions include:

- unqualified name
- qualified name
- canonical name
- source spelling
- generated name
- mangled name

These serve different purposes.

One generic:

```text
name()
```

operation may be insufficient.

The first model should expose only the naming information required by real experiments.

## Renaming and refactoring

Semantic reflection should survive source refactoring better than string-based systems.

If code identifies a field semantically rather than by parsing its name from source text, many transformations become safer.

However, systems that serialize names externally may intentionally depend on spelling.

Reflection should distinguish semantic identity from externally visible string names.

## Stable external names

Serialization or RPC may require names that remain stable even if C++ declarations are renamed.

This belongs to domain metadata.

For example, a field might have an explicitly declared external name.

Reflection should expose such metadata without assuming source identifier spelling is always the external protocol identity.

## Compile-time strings

Strings remain useful compile-time values for:

- diagnostics
- serialization names
- generated documentation
- protocol identifiers

The challenge is not to eliminate strings.

The challenge is to avoid using strings where semantic entities are available.

## Reflection and overload resolution

A reflected callable may later be invoked or passed somewhere.

If reflection can produce callable entities, VixC must determine whether invocation occurs through:

- ordinary C++ expressions
- generated calls
- compile-time invocation
- runtime function pointers

Each has different type-system consequences.

The first reflection model should probably inspect declarations without introducing reflected invocation.

## Reflection and member access

A serializer may need to access a reflected member.

If reflection only provides metadata but cannot produce an expression accessing that member, its usefulness is limited.

On the other hand, arbitrary reflective member access can bypass encapsulation and complicate typing.

Possible models include:

- generate ordinary member access during lowering
- require public accessibility
- explicit opt-in
- library customization

This should be decided through experiments.

## Reflection and runtime metadata

Reflection information can sometimes be converted into runtime tables.

For example:

```text
field name
offset or accessor
type metadata
```

A backend could generate a static table.

This may support runtime systems without requiring full runtime reflection.

VixC should distinguish:

```text
compile-time reflection
```

from:

```text
runtime reflection metadata generated at compile time
```

The latter can be built from the former.

## Runtime reflection

A dynamic runtime reflection system may allow querying arbitrary types by name during execution.

That requires:

- metadata retention
- runtime identity
- storage
- lookup
- potentially ABI support

VixC has not established a need for this.

The current research is primarily compile-time.

Runtime metadata should be generated only where applications request it.

## Cost model

Compile-time reflection is not free.

Costs may include:

- frontend memory
- semantic graph retention
- compile-time execution
- generated C++ size
- template instantiation
- metadata generation

Experiments should measure these costs.

A simpler source model that dramatically increases build time may not improve the overall C++ development experience.

## Compile-time performance

One motivation for VixC research is that C++ compile-time mechanisms can become expensive when large template systems are instantiated repeatedly.

A VixC frontend may have an opportunity to perform some semantic work directly before native compilation.

However, this is only valuable if the total pipeline becomes simpler or faster.

Moving work from C++ templates into VixC does not automatically reduce compilation cost.

Both stages must be measured together.

## Generated C++ complexity

If reflection is used to produce ordinary C++, the generated code should avoid unnecessary template machinery when straightforward declarations are sufficient.

For example, VixC may be able to generate a simple static table rather than instantiate many layers of generic introspection code.

This is a possible practical advantage of frontend-assisted reflection.

It needs measurement.

## Incremental compilation

Reflection can create dependencies between declarations.

If generated output for type `A` depends on the fields of type `B`, changing `B` should invalidate the relevant generated result.

The frontend needs explicit dependency tracking.

Without it, incremental builds may produce stale generated code.

VixC should therefore treat semantic reflection dependencies as build inputs.

## Dependency graph

A reflected entity may depend on:

- declaration structure
- type properties
- annotations
- template arguments
- imported declarations
- external declared inputs

The frontend may eventually need to record these dependencies for Vix.cpp.

This should be done structurally rather than by assuming every compile-time operation depends on the entire source tree.

## Caching

Pure compile-time reflection is a strong candidate for caching.

If an operation depends only on stable semantic inputs, its result could potentially be reused.

However, caching semantic compiler objects is more complex than caching generated text.

Stable serialization and versioning may be required.

This is future work.

## Parallel compilation

Compile-time operations with no external mutable state can be evaluated safely in parallel.

Operations that mutate global compiler state or perform uncontrolled I/O make parallelism much harder.

This is another reason to prefer pure semantic reflection initially.

## Security

Arbitrary compile-time execution can become a security boundary.

Building an untrusted project should not automatically grant arbitrary code execution merely because the compiler processes its source.

C++ build systems already encounter this issue through build scripts and generators.

A VixC compile-time model should avoid increasing that surface unnecessarily.

Pure semantic evaluation has a significantly smaller risk profile than arbitrary host execution.

## Compile-time native execution

One possible implementation shortcut is to compile and execute helper programs during compilation.

This can provide enormous flexibility.

It also creates:

- host dependency
- cross-compilation problems
- security concerns
- nondeterminism
- additional compiler invocations
- cache complexity

VixC should not define compile-time semantics in terms of arbitrary host-native execution unless there is strong evidence that it is required.

## Cross-compilation

Compile-time code executes in a host environment while generated applications may target another platform.

This distinction matters.

A compile-time operation cannot assume target-native behavior if it is executed as host code.

For example:

```text
pointer size
endianness
platform API availability
```

may differ between host and target.

A semantic evaluator can explicitly model target properties without executing target code.

This is another argument for frontend-level compile-time semantics.

## Host and target distinction

Reflection over source declarations is generally target-independent until layout or ABI information is requested.

Questions such as:

```text
What are this type's fields?
```

can often be answered semantically.

Questions such as:

```text
What is the exact target offset of this field?
```

may require target layout information.

The reflection model should distinguish structural semantics from backend ABI facts.

## Layout reflection

Exposing:

```text
size
alignment
field offset
```

creates target-specific reflection.

These values may depend on:

- compiler ABI
- architecture
- packing
- attributes
- base classes

A C++ backend or native compiler may be the authority for some of this information.

VixC should avoid pretending it can compute complete C++ layout independently unless it has equivalent semantic machinery.

## ABI reflection

Similarly, reflecting mangled names or calling convention details ties the language to backend implementation.

Such information can be useful for tooling.

It should remain clearly target-specific rather than part of portable semantic reflection.

## Compile-time and backend independence

A VixC reflection model should operate before the C++ backend whenever the reflected property belongs to VixC semantics.

For example:

```text
Failure contract of this function
```

is VixC semantic information.

It should not be reconstructed from generated C++.

Likewise, future:

```text
async Outcome
ownership contract
Choice alternatives
```

could become reflectable VixC properties.

This is a major reason for developing a semantic frontend rather than only a source transformer.

## Reflecting VixC semantics

A future reflection system could potentially expose:

```text
Does this computation fail?
What is its Failure type?
Can this computation stop?
What alternatives belong to this Choice?
What lifetime contract belongs to this parameter?
```

These questions cannot be answered reliably from generated C++ if backend representation changes.

Reflection therefore strengthens the requirement that VixC semantics remain explicit in frontend data structures.

## Reflection and Failure

Suppose:

```cpp
User load() fails LoadError;
```

A reflection system might eventually expose:

```text
success type: User
failure type: LoadError
```

This could support generic adapters, documentation, or generated interfaces.

It also raises a larger design question:

> Are Outcome contracts part of reflected function type semantics?

The answer depends on unresolved Failure type-identity work.

## Reflection and Choice

If VixC introduces Choice semantics, reflection could expose alternatives.

Conceptually:

```text
alternatives(State)
```

could return semantic alternative entities.

This could support generic serialization or documentation.

Again, this does not imply a specific final reflection syntax.

## Reflection and async

A future async computation may expose metadata such as:

```text
eventual success type
failure type
stopped capability
```

Reflection could make these contracts usable by libraries without reconstructing task implementation types.

This is another reason to keep async semantics independent from one runtime representation.

## Reflection and ownership

If ownership contracts become semantic metadata, reflection may expose them.

For example:

```text
parameter consumed
return borrows from parameter 1
callback retained
```

This could improve documentation and generic adapter generation.

The ownership model must exist first.

Reflection should observe semantics rather than invent them.

## Reflection and composition

Composition research may eventually use reflection to discover capabilities and assemble application structures.

For example, a component could expose:

```text
requirements
provided interfaces
metadata
```

at compile time.

This may allow VixC to validate composition before runtime.

The reflection system should not be designed solely around this hypothetical use case, but the architecture should avoid preventing it.

## Metaprogramming and generated diagnostics

One potential advantage over complex templates is the ability to produce diagnostics intentionally.

Instead of exposing a long substitution trace, compile-time semantic code could report:

```text
field 'socket' cannot participate in automatic serialization
```

with a direct source location.

This would be a meaningful improvement if achieved without sacrificing generic power.

## Error recovery

Compile-time reflection may encounter invalid source while the editor is still being used.

For language tooling, the frontend should continue providing partial semantic information where possible.

A reflection engine should not assume the entire translation unit is always valid.

This creates complexity around incomplete declarations and error nodes.

The first compiler-oriented implementation may remain stricter than future editor integration.

## Reflection availability

Should reflection be available everywhere?

Possible restrictions include:

- only during compile-time functions
- only in explicit reflection contexts
- only through library APIs
- only on complete declarations
- only after semantic analysis

A restricted initial model may be easier to reason about.

Global unrestricted reflection can create hidden compile-time dependencies.

## Explicit reflection points

One useful principle may be that reflection is explicit in source.

A programmer reading code should be able to identify where program structure is being inspected.

This can improve:

- compile-time cost awareness
- dependency understanding
- debugging
- code review

The exact syntax is open.

## Possible reflection syntax

No syntax is currently selected.

Research notation may use concepts such as:

```text
reflect(T)
members(T)
type_of(entity)
name_of(entity)
```

These are explanatory forms only.

They are not committed VixC APIs or keywords.

The actual surface should be designed after the semantic objects are understood.

## Keyword compatibility

Introducing words such as:

```text
reflect
meta
comptime
```

as unconditional keywords could invalidate existing C++ identifiers.

VixC should learn from the Failure keyword compatibility problem.

Contextual syntax or library-style entry points may preserve compatibility better.

Syntax should not be chosen merely because another language uses a familiar keyword.

## Library API versus syntax

Reflection may not require much new syntax.

A possible design could use one language primitive to obtain semantic metadata and ordinary C++ functions to manipulate it.

This could keep the language surface small.

However, ordinary C++ types may not be capable of representing every compiler entity naturally.

The right boundary between language primitive and library API remains open.

## Reflection handles

A minimal language primitive might produce an opaque reflection handle.

Library-style compile-time functions could then query it.

Potential advantages include:

- small syntax surface
- extensibility
- fewer reserved words

Potential disadvantages include:

- weaker static typing between reflected entity categories
- library API complexity
- potential misuse of opaque handles

This should be explored experimentally.

## Typed reflection entities

An alternative is to expose different compile-time semantic types:

```text
TypeInfo
FunctionInfo
FieldInfo
ParameterInfo
```

This may provide better API clarity.

It also creates a larger standard reflection library.

Whether these types are language-defined or library-defined remains open.

## Reflection collections

Enumerating members requires a compile-time collection model.

For example:

```text
members(T)
```

may produce a sequence.

Questions include:

- Is its size known statically?
- Can it participate in range-for syntax?
- Is it a parameter pack?
- Is it a tuple-like object?
- Can it be filtered?
- Can it be stored?
- Is its order stable?

The answer influences the usability of the entire reflection system.

## Compile-time iteration

A reflection model needs a way to apply logic to multiple reflected entities.

C++ already supports:

- template pack expansion
- fold expressions
- constexpr loops over suitable values

If reflected collections can participate in ordinary `constexpr` iteration, VixC may avoid adding special metaprogramming loops.

This would preserve C++ familiarity.

The feasibility depends on how reflection entities are represented.

## Reflection entity lifetime

Compiler semantic entities exist during frontend processing.

If they appear as compile-time values, their validity must not escape into runtime unless converted into runtime metadata.

The language should prevent meaningless runtime storage of compiler-internal handles.

For example, a reflected field entity should not become a raw runtime pointer into compiler memory.

## Serialization of reflection data

For caching or cross-module metadata, reflection information may need a serialized representation.

This is an implementation concern.

The public semantic model should not expose compiler-memory addresses or unstable internal node IDs.

Stable semantic identity will matter if VixC eventually supports persisted reflection metadata.

## Reflection equality

Can two reflected entities be compared?

For declarations, semantic identity comparison is useful.

For types, canonical type equality may differ from alias identity.

The language may need more than one relation.

For example:

```text
same declaration
same canonical type
same source spelling
```

These should not be conflated.

## Reflection hashing

Generic compile-time algorithms may want stable keys for reflected entities.

Hashing compiler pointer addresses would be incorrect across invocations.

A stable hash may be possible only for certain semantic identities.

This is a later tooling and caching question.

## Generated names

If compile-time metaprogramming creates declarations, naming becomes difficult.

Generated declarations may need:

- hygienic internal names
- user-visible names
- deterministic mangling
- collision detection

VixC already has deterministic synthetic identifiers in lowering.

That mechanism is not automatically suitable for language-level generated declaration identity.

## Hygiene

Macro systems often struggle with accidental name capture.

Structured semantic generation could potentially provide hygiene by using declaration identity rather than textual names.

If VixC eventually supports generation, hygiene should be a first-class design concern.

It should not be solved through increasingly unusual generated identifiers alone.

## Reflection and macros

Macros will remain part of the C++ ecosystem.

A reflected declaration may originate from macro expansion.

Questions include:

- What source location does reflection expose?
- Is the declaration considered to belong to the expansion site or spelling site?
- Can reflection distinguish generated-by-macro declarations?
- Can compile-time diagnostics point through macro expansions correctly?

The current lightweight frontend does not yet model full preprocessing.

This limits near-term reflection ambitions.

## Preprocessing order

A fundamental question is whether VixC reflection operates:

```text
before C++ preprocessing
```

or:

```text
after preprocessing
```

Semantic reflection naturally requires something closer to the post-preprocessing program because declarations may be generated conditionally by macros.

However, VixC currently processes source directly.

A mature reflection model may therefore require deeper integration with C++ preprocessing or another semantic frontend.

## Need for richer C++ semantics

Reflection is one of the strongest cases where VixC's current lightweight syntax model may eventually become insufficient.

Reliable reflection needs information such as:

- resolved declarations
- semantic types
- templates
- overload sets
- access control
- namespaces
- attributes
- source provenance
- possibly preprocessing history

Building all of this independently would be a major undertaking.

VixC should evaluate integration with existing C++ frontend infrastructure before duplicating mature semantic machinery.

## Clang integration question

One possible long-term direction is using Clang semantic information for ordinary C++ while VixC owns its additional language semantics.

Potential advantages include:

- mature parsing
- semantic types
- templates
- macros
- declarations
- source locations
- tooling

Potential costs include:

- dependency size
- memory
- compilation cost
- version coupling
- architecture complexity
- portability considerations

This decision should be driven by concrete reflection and ownership requirements rather than made prematurely.

## Backend compiler independence

Even if one semantic analysis implementation uses Clang infrastructure, VixC's generated C++ should ideally remain compilable by:

- GCC
- Clang
- MSVC

Frontend implementation and native backend compiler are separate choices.

The architecture should preserve that distinction.

## Compile-time reflection and VixC IR

Reflection usually operates on semantic declarations before ordinary executable IR is finalized.

A reflected program entity should therefore not be reconstructed from backend IR.

However, compile-time operations may generate semantic values that influence IR construction.

The likely direction is:

```text
semantic declarations
reflection
compile-time semantic evaluation
validated semantic result
IR construction
lowering
backend
```

This is conceptual.

The exact frontend phase boundaries remain open.

## Reflection and IR introspection

Should user code be able to reflect VixC IR itself?

Probably not by default.

IR is a compiler implementation abstraction.

Making it part of the language would strongly constrain future compiler evolution.

Reflection should expose stable language semantics, not arbitrary internal lowering structures.

## Compiler plugins

Another alternative is compiler plugin APIs rather than language-level reflection.

Plugins can inspect rich semantic information without exposing it to every program.

This is useful for tooling and specialized generation.

However, plugins are usually compiler-specific and less portable.

VixC should distinguish:

```text
program reflection
```

from:

```text
compiler extension API
```

They solve different problems.

## External generators

External source generators remain useful.

For example:

```text
schema
    -> generator
    -> C++ source
```

This approach can be simple and language-independent.

VixC reflection should not attempt to eliminate every generator.

The language feature is justified only where semantic integration, diagnostics, incremental builds, or developer experience are meaningfully improved.

## Reflection and application frameworks

Frameworks often use macros or code generation for:

- routes
- RPC
- dependency injection
- serialization
- ORM
- tests
- commands

VixC reflection may offer a more semantic foundation for some of these.

However, VixC should not become a framework-specific language.

The reflection model must remain general enough to serve native software beyond one application architecture.

## Registration

Many C++ systems require explicit registration:

```cpp
register_type<User>();
```

Reflection might reduce some registration boilerplate.

But automatic whole-program discovery conflicts with separate compilation.

A more realistic model may allow reflecting declarations explicitly referenced by the program rather than discovering every declaration globally.

The distinction between:

```text
reflect known type
```

and:

```text
discover all types
```

is important.

## Discovery

Whole-program discovery is primarily a build and indexing problem.

For example:

```text
find all tests
```

may require knowledge from multiple translation units.

Vix.cpp could potentially collect metadata emitted by VixC from each source unit.

This may provide application-level discovery without making local language reflection global.

Such architecture should be explored before adding global compiler magic.

## Reflection metadata artifacts

One possibility is for VixC compilation to emit semantic metadata alongside generated C++.

For example:

```text
translation-unit metadata
```

could describe explicitly exported reflective entities.

Vix.cpp could combine those artifacts later.

This would preserve separate compilation.

No metadata format is currently defined.

## Stable metadata formats

If metadata becomes part of cross-translation-unit compilation, its versioning matters.

The format should not expose arbitrary internal compiler structures.

A stable semantic schema would be required.

This is future work and should not be introduced for the first local reflection experiment.

## Reflection and incremental Vix builds

Vix.cpp already manages build state and generated artifacts.

If VixC reflection emits deterministic metadata, Vix.cpp could integrate it into incremental builds.

A change to one reflected declaration should invalidate only consumers that depend on its relevant semantic properties where possible.

This requires dependency information from VixC.

## Reflection and offline development

Compile-time reflection based only on local semantic source is naturally compatible with offline builds.

Compile-time network access would not be.

Given the broader Vix.cpp goal of native application development that should not require unnecessary external infrastructure, pure local reflection is a better default direction.

The language should not make Internet access part of compilation semantics.

## First useful experiment

The first compile-time reflection experiment should remain narrow.

A strong candidate is semantic inspection of an ordinary C++ struct.

For example:

```cpp
struct User
{
    int id;
    std::string name;
};
```

The frontend could expose enough semantic information to answer:

```text
type name
number of fields
field names
field semantic types
source ranges
```

without yet supporting declaration generation.

The experiment could then perform a compile-time validation.

For example:

```text
every field must satisfy a selected property
```

and produce direct diagnostics when one does not.

This would test:

- semantic reflection
- source provenance
- compile-time iteration
- diagnostics
- C++ type integration

without requiring a complete metaprogramming language.

## Second useful experiment

A second experiment could generate static metadata from reflected fields.

For example, VixC could produce ordinary C++ metadata equivalent to:

```text
field descriptor table
```

The generated C++ could then be consumed by an ordinary runtime library.

This would test:

- deterministic generation
- source mapping
- runtime metadata
- native compiler interoperability
- compile-time cost

without allowing arbitrary user declaration injection.

## Failure reflection experiment

Once Failure declaration scope is complete, another useful experiment is reflecting:

```cpp
User load() fails LoadError;
```

The frontend could answer:

```text
has recoverable Failure: yes
success type: User
failure type: LoadError
```

This would verify that reflection observes VixC semantics rather than only C++ syntax.

It should happen after the Failure model itself is stable enough to reflect meaningfully.

## What should not be the first experiment

The research should not begin with:

- arbitrary compile-time filesystem access
- arbitrary network access
- execution of native host binaries
- unrestricted declaration injection
- whole-program reflection
- self-modifying compiler phases
- reflection over every C++ grammar entity
- a complete replacement for templates
- runtime dynamic reflection for all types

These would introduce too many independent problems at once.

## Questions about the compile-time model

The research must eventually answer:

- What code can execute at compile time?
- What values exist only during compilation?
- Is compile-time evaluation pure by default?
- Can compile-time code allocate?
- Can it perform I/O?
- How are external inputs declared?
- How are failures represented?
- How are diagnostics emitted?
- How are resource limits handled?
- How are compile-time dependencies tracked?
- Can compile-time operations be cached?
- How does cross-compilation affect evaluation?

These questions should be answered before introducing a general-purpose compile-time execution syntax.

## Questions about reflection

The reflection model must answer:

- What program entities can be reflected?
- What information is exposed?
- Are entities strongly typed?
- How is semantic identity represented?
- How are aliases handled?
- How are overloads handled?
- How are templates handled?
- Does reflection obey access control?
- Can private declarations opt in?
- Are attributes visible?
- Are VixC semantic contracts reflectable?
- Is reflection local or whole-program?
- How are reflected collections represented?
- What ordering is guaranteed?

The first experiment only needs a subset.

## Questions about generation

If VixC later supports generative metaprogramming, it must answer:

- Can compile-time code create declarations?
- When do generated declarations become visible?
- Can generated declarations themselves be reflected?
- Can generation recurse?
- How are name conflicts handled?
- Is generation hygienic?
- How are generated source locations represented?
- How does generation affect incremental compilation?
- Can generated code change overload resolution retroactively?
- How is termination guaranteed?

These questions are intentionally deferred.

## Questions about interoperability

A useful reflection system must coexist with ordinary C++.

Questions include:

- Can ordinary C++ libraries expose reflectable metadata?
- Can third-party libraries opt in without depending deeply on VixC?
- Can reflection consume C++ attributes?
- Can generated runtime metadata be used from ordinary C++?
- Can VixC reflection work across headers and modules?
- Can native compilers ignore VixC metadata safely?

The design should not require rewriting the ecosystem.

## Questions about templates

Templates create additional questions:

- Can reflected entities become template arguments?
- Can reflection execute during template instantiation?
- Can a template reflect an incomplete dependent type?
- Are reflection results dependent values?
- Can concepts query reflection?
- How does reflection interact with specialization?
- Does reflection increase instantiation cost substantially?

This area likely requires mature C++ semantic information.

## Questions about diagnostics

Reflection should improve diagnostics rather than create another layer of opaque metaprogramming errors.

The research should determine:

- how compile-time user diagnostics are represented
- how source ranges are attached
- whether notes can refer to multiple reflected declarations
- whether compile-time call traces are displayed
- how internal compiler errors are separated from program diagnostics
- how resource-limit failures are explained

Diagnostic quality should be part of reflection design from the beginning.

## Questions about reproducibility

Any capability with external effects must answer:

- What are its inputs?
- Are they declared?
- Can they change without source changes?
- Can the build system detect those changes?
- Is the output deterministic?
- Can it run during cross-compilation?
- Can it run in sandboxed or remote builds?

If these questions cannot be answered, the capability probably does not belong in baseline compile-time semantics.

## Potential semantic vocabulary

Research may use terms such as:

```text
reflection entity
compile-time value
semantic metadata
reflection query
compile-time evaluation
declaration generation
injection
```

These are conceptual terms.

They are not proposed source keywords.

The language surface should remain smaller than the semantic model wherever possible.

## What would justify VixC reflection

Reflection would be justified if experiments show meaningful improvements such as:

- direct semantic access to declarations
- substantially clearer compile-time diagnostics
- reduced dependence on repetitive macros
- simpler generation of metadata
- less template machinery for structural introspection
- better tooling
- reflection of VixC-specific semantics
- deterministic integration with native C++ builds
- acceptable compile-time and memory cost

The value must extend beyond syntactic novelty.

## What would argue against language-level reflection

The direction should be reconsidered if:

- existing C++ reflection facilities already provide the required model cleanly
- VixC must reproduce a complete C++ semantic frontend independently
- compile-time cost becomes excessive
- syntax adds little over library facilities
- deterministic builds become difficult
- generation requires unrestricted host execution
- interoperability with ordinary C++ becomes poor
- reflection routinely breaks encapsulation
- metaprogram errors remain as difficult as existing template diagnostics

In that case, VixC may be better served by tooling or integration rather than new language semantics.

## What must not become accidental specification

The following remain provisional:

- whether VixC introduces a `reflect` keyword
- whether VixC introduces a `comptime` concept
- whether reflection entities are ordinary values
- whether reflection uses a standard library API
- whether reflection obeys access control exactly like ordinary expressions
- whether reflected members are returned in declaration order
- whether declaration generation is supported
- whether arbitrary compile-time I/O is allowed
- whether compile-time Failure uses the runtime Failure model
- whether reflection metadata survives into runtime
- whether VixC uses Clang semantic infrastructure
- whether reflection requires a runtime
- whether metadata uses attributes
- whether whole-program reflection exists

Experiments must not silently freeze these choices.

## Relationship with the current frontend

Compile-time reflection should use the same VixC frontend architecture.

The likely semantic path includes:

- source management
- parsing
- declaration analysis
- type information
- semantic reflection
- compile-time evaluation
- diagnostics
- IR construction
- lowering
- backend generation

Reflection requires semantic information richer than the current first-generation Failure parser provides.

This research therefore depends on the frontend becoming capable of representing declarations and semantic types more precisely.

## Relationship with the C++ backend

The C++ backend should receive the result of compile-time semantic work.

It should not need to reconstruct reflection operations from source text.

For example, if reflection creates runtime metadata for a type, the backend may receive a structured representation of that metadata and emit ordinary C++.

The C++ backend remains responsible for native representation.

The reflection layer remains responsible for semantic introspection.

## Runtime independence

Compile-time reflection does not inherently require a VixC runtime.

Pure reflection and compile-time validation can disappear entirely after compilation.

Generated metadata may become ordinary static C++ data.

A runtime should only be introduced when the application requests runtime behavior that requires one.

Reflection itself should not justify a mandatory runtime dependency.

## Research priority

Compile-time and reflection work should not interrupt completion of the first Failure vertical slice.

The Failure experiment is already exposing foundational requirements that reflection will also need:

- declaration scope
- semantic type identity
- structured diagnostics
- source provenance
- backend-independent semantics

Completing those foundations first will make reflection research more concrete.

## Immediate research questions

Before designing syntax, this research should answer a smaller set of questions.

First, what concrete compile-time problem in real Vix.cpp applications is currently difficult enough to justify frontend support?

Second, can that problem be solved by existing C++ `constexpr`, concepts, templates, or ordinary code generation?

Third, what semantic declaration information does VixC need to expose that C++ library mechanisms cannot currently access coherently?

Fourth, can the first reflection experiment remain purely observational without declaration injection?

Fifth, can reflected information be represented independently from source strings?

Sixth, what minimum C++ semantic frontend capability is required to identify fields, types, and declarations correctly?

Seventh, can the result improve diagnostics or compile-time cost enough to justify the additional frontend complexity?

These questions should be answered before a permanent language surface is introduced.

## Research discipline

Compile-time programming is powerful enough to consume an entire language design if its boundaries are not controlled.

VixC should distinguish:

```text
constant evaluation
semantic reflection
template metaprogramming
preprocessing
code generation
build-system generation
compiler extension
runtime metadata
```

These are different mechanisms.

A useful VixC feature should solve a specific semantic problem at the correct layer.

For example:

```text
inspect the semantic fields of this type
```

may justify reflection.

```text
copy this text into another file
```

probably belongs to a build or generation tool.

```text
evaluate this pure function before runtime
```

may already be solved by `constexpr`.

```text
discover every test across the application
```

may require build-level aggregation rather than local reflection.

Keeping these distinctions clear is necessary if VixC is to add coherence rather than another metaprogramming subsystem.

## Current position

The current direction is to treat compile-time computation and reflection as related but distinct research areas.

Ordinary C++ compile-time mechanisms remain the foundation.

VixC should only introduce additional semantics where the frontend needs structured program knowledge that existing mechanisms cannot expose coherently enough.

The strongest initial hypothesis is semantic reflection for explicit declarations, combined with compile-time validation and deterministic metadata generation.

The research does not currently commit to:

```text
new reflection syntax
declaration injection
arbitrary host execution
whole-program reflection
runtime reflection
a new template system
```

The next useful evidence should come from a small semantic reflection experiment over ordinary C++ declarations, with direct diagnostics and deterministic generated output, before VixC attempts to design a general metaprogramming language.
