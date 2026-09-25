# Composition Research

This directory contains research for possible VixC language semantics around composing software units, capabilities, requirements, and application structure.

The goal is not to invent another dependency injection framework, module system, service container, package manager, or object model.

C++ already provides many composition mechanisms:

- functions
- classes
- inheritance
- templates
- concepts
- namespaces
- translation units
- libraries
- modules
- constructors
- factories
- dependency injection libraries
- build-system dependency graphs
- runtime registries

The research question is narrower:

> Can VixC provide a more coherent semantic model for describing what a software unit provides, what it requires, and how those relationships are validated before runtime?

This work is exploratory.

No composition syntax, component model, capability system, or application declaration described here should be considered stable.

## Motivation

Large applications are rarely built from isolated functions.

They are composed from units that:

- provide behavior
- require other behavior
- own resources
- depend on configuration
- expose interfaces
- participate in startup
- participate in shutdown
- create asynchronous work
- fail during initialization
- depend on platform capabilities

C++ can represent all of these relationships.

The difficulty is that application composition is often distributed across several layers:

```text
source code
constructors
factories
templates
build configuration
registration
configuration files
runtime wiring
dependency injection containers
startup code
```

The result may work correctly while the architectural relationships remain difficult for the compiler to understand.

VixC research asks whether selected composition relationships can become explicit semantic information.

## Composition is not packaging

A package dependency answers questions such as:

```text
Which library does this project depend on?
Where does it come from?
How is it built?
```

Composition answers different questions:

```text
Which capability does this software unit require?
Which implementation provides it?
What lifetime does that provider have?
Can the application satisfy every requirement?
```

Vix.cpp already has responsibilities around dependencies and builds.

VixC composition should not become another package manager.

The language frontend should focus on semantic relationships inside software.

## Composition is not a build graph

A build graph represents dependencies required to produce binaries and other artifacts.

For example:

```text
source file
    -> object file
    -> library
    -> executable
```

Application composition represents runtime or semantic relationships between program units.

For example:

```text
HTTP service
    requires Logger
    requires Database
```

Those two graphs may overlap, but they are not equivalent.

A service may depend semantically on an interface even when the implementation is linked indirectly through another library.

VixC should keep build composition and program composition separate.

## Composition is not automatically dependency injection

Dependency injection is one possible technique for providing required objects.

For example:

```cpp
class UserService
{
public:
    UserService(
        Database &database,
        Logger &logger);
};
```

Constructor injection already expresses dependencies clearly.

A framework may provide those objects automatically.

VixC should not add a language feature merely to remove constructor calls.

The deeper research question is whether the frontend should understand dependency relationships as semantic contracts and validate application assembly.

## Existing C++ composition

C++ already offers several strong mechanisms.

### Constructor composition

Objects can require dependencies explicitly through constructors:

```cpp
class Service
{
public:
    Service(
        Database &database,
        Logger &logger);
};
```

This is simple and type-safe.

Ownership remains visible through ordinary C++ types.

Any VixC composition model should preserve the usefulness of this pattern.

### Templates

Compile-time composition can be expressed through templates:

```cpp
template <
    typename Database,
    typename Logger>
class Service;
```

This can provide zero-runtime-overhead dependency selection.

It can also create large template surfaces and couple implementation structure to compile-time types.

VixC should not replace templates merely because application wiring becomes verbose.

### Concepts

Concepts can define requirements on implementations.

For example:

```cpp
template <typename T>
concept Logger =
    requires(T logger)
    {
        logger.write("message");
    };
```

This provides structural compile-time validation.

A composition model should understand what additional information it would provide beyond concepts.

### Virtual interfaces

Runtime polymorphism allows implementation selection behind stable interfaces.

For example:

```cpp
struct Logger
{
    virtual ~Logger() = default;

    virtual void write(
        std::string_view message) = 0;
};
```

This supports runtime substitution.

It also introduces virtual dispatch and explicit lifetime management.

VixC should remain compatible with this model.

### Factories

Factories centralize construction:

```cpp
std::unique_ptr<Service>
make_service();
```

They can hide implementation details and compose dependencies.

They are ordinary C++ and often sufficient.

### Registries

Applications may register implementations in a central registry.

Conceptually:

```cpp
registry.add<Logger>(
    make_logger());
```

and later resolve:

```cpp
registry.get<Logger>();
```

This allows dynamic composition.

It also moves some errors from compilation to runtime.

A VixC semantic model may provide value if it can validate some registry relationships earlier without requiring a universal runtime container.

## What is being composed?

Before designing syntax, the research needs a precise unit of composition.

Possible units include:

- objects
- types
- functions
- services
- modules
- capabilities
- application components
- tasks
- resources

These categories are not interchangeable.

A C++ type is not necessarily one runtime service.

A namespace is not necessarily one component.

A translation unit is not necessarily one ownership boundary.

VixC should avoid choosing an existing structural unit merely because it is convenient for the parser.

## Capability-oriented composition

One promising direction is capability-oriented composition.

Instead of saying:

```text
Component A depends on class ConcreteDatabase
```

the semantic relationship could be:

```text
Component A requires database access
```

Another unit may provide that capability.

This separates:

```text
what is required
```

from:

```text
which concrete implementation satisfies it
```

This is conceptually similar to interfaces or concepts, but the composition relationship may also include lifetime, multiplicity, startup, and resource semantics.

## Capability is provisional terminology

The word:

```text
capability
```

is working research vocabulary.

It is not committed source syntax.

It may eventually mean:

```text
a semantic service or operation that one software unit can require or provide
```

However, the term is also used in security and operating-system research with stronger meanings.

VixC should not adopt the word permanently until its exact semantics are clear.

## Requirements and providers

The simplest composition hypothesis is that one unit can declare:

```text
requires X
```

and another can declare:

```text
provides X
```

Then application composition verifies that every required capability has an appropriate provider.

This raises immediate questions.

Can there be multiple providers?

Can a requirement be optional?

Can one provider depend on another?

Can requirements be parameterized?

What determines provider lifetime?

Can providers be selected at runtime?

Can a provider fail during initialization?

These questions define the actual composition model.

## Structural versus nominal capability

A capability could be nominal:

```text
Logger
```

means one explicitly declared semantic capability.

Or it could be structural:

```text
anything satisfying these operations
```

C++ concepts already provide structural constraints.

Nominal capability identity may make application wiring and diagnostics clearer.

Structural capabilities may integrate better with existing types.

The correct model remains open.

## Composition and ordinary types

A VixC capability should probably remain related to ordinary C++ types.

For example, if a service requires:

```cpp
Logger &
```

the language should not invent an unrelated hidden object representation.

One possible direction is that composition metadata describes how ordinary C++ values are obtained and supplied.

This would preserve normal C++ calling and ownership semantics.

## Application assembly

A composition system needs a point where requirements are resolved.

Conceptually:

```text
application
    provide Database with PostgresDatabase
    provide Logger with FileLogger
    use UserService
```

This is only semantic notation.

The important question is:

> At what point does VixC know the complete set of providers and requirements?

Possible answers include:

- per translation unit
- explicit application composition block
- Vix.cpp project configuration
- generated metadata aggregated by the build
- runtime registration

The answer affects separate compilation significantly.

## Local composition

Local composition is straightforward.

For example:

```cpp
Database database;
Logger logger;

Service service(
    database,
    logger);
```

The compiler already understands this.

VixC does not need another mechanism for simple local construction.

The stronger opportunity is application-level composition where dependencies are intentionally declared apart from their concrete assembly.

## Whole-application composition

Application-level validation may need knowledge from multiple translation units.

For example:

```text
Service requires Database
AnotherService requires Logger
Postgres provides Database
ConsoleLogger provides Logger
```

No single translation unit may contain the entire graph.

This makes composition partly a build-level problem.

VixC may need to emit semantic metadata that Vix.cpp can aggregate.

The language frontend and build system should cooperate without collapsing into one layer.

## Separate compilation

Separate compilation is a central constraint.

A source file should not require parsing the complete application merely to compile one function.

A scalable composition model may therefore separate:

```text
local declaration of requirements and providers
```

from:

```text
application-level validation and selection
```

This is similar to how linkers resolve symbols after separate compilation.

The exact mechanism remains open.

## Composition metadata

One possible architecture is for each VixC compilation unit to emit metadata describing:

```text
provided capabilities
required capabilities
lifetime constraints
configuration requirements
initialization properties
```

Vix.cpp could aggregate that metadata during application assembly.

This would preserve VixC as the semantic authority while allowing Vix.cpp to own project-level orchestration.

No metadata format is currently defined.

## Composition is not global reflection

A composition system may need to discover providers across an application.

This resembles whole-program reflection.

However, it may be better to require explicit export of composition metadata rather than scanning every type and function in the program.

Explicit metadata improves:

- determinism
- encapsulation
- incremental builds
- separate compilation
- diagnostics

The system should not assume every declaration participates in application composition.

## Provider selection

If exactly one provider satisfies a requirement, selection is simple.

If several providers exist, the language needs a rule.

Possible strategies include:

- explicit application selection
- priority
- configuration
- type parameter
- runtime selection
- error on ambiguity

Implicit priority systems can become difficult to reason about.

A strong default may be that ambiguity requires explicit resolution.

This should be tested.

## Missing provider

A semantic composition system could diagnose:

```text
no provider satisfies required capability 'Database'
```

before application execution.

This is one of the clearest potential benefits.

The diagnostic could point to:

- the unit requiring the capability
- the application assembly boundary
- available related providers if relevant

This is stronger than a later runtime lookup failure.

## Ambiguous provider

If two providers satisfy the same single-provider requirement:

```text
PostgresDatabase
SQLiteDatabase
```

the frontend or application assembler could report:

```text
multiple providers satisfy capability 'Database'
```

and require explicit selection.

Again, this is potentially clearer than runtime container behavior.

## Multiple providers

Some capabilities legitimately have multiple providers.

Examples include:

- event handlers
- middleware
- plugins
- observers
- command implementations
- serializers

The model therefore needs multiplicity.

A requirement might conceptually mean:

```text
exactly one
zero or one
one or more
zero or more
```

No source syntax is selected.

This semantic dimension should be understood before designing declaration grammar.

## Optional requirements

A component may use a capability when available but function without it.

For example:

```text
optional Metrics
```

This should remain distinct from:

```text
required Metrics
```

The model must define whether optionality becomes:

- `std::optional`
- nullable reference
- separate capability query
- another representation

The language semantics should not hide important runtime absence.

## Provider lifetime

Providing an object is not enough.

The consumer may require that it remain valid for a certain lifetime.

For example:

```text
application-wide Logger
request-scoped DatabaseSession
operation-scoped Transaction
```

A composition model that ignores lifetime may create dangling references even when type matching succeeds.

This connects composition directly to ownership and lifetime research.

## Lifetime categories

Possible provider lifetime categories include:

```text
application
scope
request
task
transient
shared
```

These are common framework concepts.

VixC should not adopt such categories blindly.

The semantic question is more general:

> What lifetime relationship exists between the provided value and its consumers?

Concrete categories may be library-level policy rather than language semantics.

## Ownership of providers

A provider may be:

- owned by the application
- owned by a parent component
- constructed for each consumer
- shared
- borrowed from external state
- supplied by the platform

Composition must not hide ownership.

For example, injecting:

```cpp
Database &
```

requires the database to outlive the consumer.

If the composition frontend knows both lifetimes, it may be able to validate this.

## Construction

A provider may require construction.

For example:

```cpp
Database database(config);
```

Construction itself may require dependencies.

This naturally forms a dependency graph.

A composition system can potentially determine a valid initialization order from the graph.

However, construction is ordinary C++ code and may contain arbitrary behavior.

VixC should not reduce all constructors to declarative dependency nodes.

## Initialization order

Suppose:

```text
Service requires Repository
Repository requires Database
```

then initialization logically requires:

```text
Database
Repository
Service
```

A semantic composition graph could establish that ordering.

This may reduce manual startup code.

But cycles and dynamic conditions complicate the model.

## Dependency cycles

Consider:

```text
A requires B
B requires A
```

This may indicate an invalid construction cycle.

It may also be valid if one relationship is indirect, lazy, or uses an interface available before full initialization.

A composition system needs a clear model before diagnosing every cycle as invalid.

At minimum, immediate construction cycles should be detectable.

## Lazy providers

A provider may be created only when first needed.

This changes initialization semantics.

Possible representation includes:

```text
factory
lazy handle
provider function
```

VixC should probably not make laziness an implicit property.

If construction timing matters, it should remain visible semantically.

## Factories as providers

An existing C++ function can naturally provide a capability:

```cpp
std::unique_ptr<Database>
make_database();
```

A composition model could potentially treat this as a provider factory.

This preserves ordinary C++.

The frontend would need metadata explaining what capability the function provides and how the returned value is owned.

## Provider Failure

Provider initialization may fail.

For example:

```text
construct Database
    -> failure(ConnectionError)
```

This connects composition to Failure semantics.

A composition system must decide whether application startup:

- propagates the Failure
- handles it
- chooses another provider
- reports configuration invalidity

This should not be hidden behind generic container exceptions.

## Startup Failure

Some composition problems are static:

```text
missing provider
ambiguous provider
dependency cycle
```

These should ideally be diagnosed before runtime.

Other problems are dynamic:

```text
database credentials rejected
port already in use
file missing
```

These are recoverable or fatal runtime Failures depending on application policy.

The composition model should distinguish static graph invalidity from runtime provider Failure.

## Fallback providers

An application might want fallback behavior.

For example:

```text
try primary storage
if unavailable use local storage
```

This is runtime policy, not simply provider selection.

A static composition system should not silently choose fallback based on initialization Failure unless the application explicitly defines that behavior.

Failure handling should remain visible.

## Configuration

Providers frequently require configuration.

For example:

```text
Database requires DatabaseConfig
```

Configuration may come from:

- source constants
- command-line arguments
- files
- environment variables
- remote systems
- user input

Composition should not automatically absorb configuration management.

A configuration value may simply be another dependency represented through ordinary types.

The source of that value can remain outside the core composition semantics.

## Environment dependency

A provider may depend on external environment capabilities.

For example:

```text
filesystem
network
GPU
database server
specific operating-system feature
```

Some of these can be known at build time.

Others can only be checked at runtime.

VixC should distinguish semantic application requirements from assumptions about the current build machine.

## Platform capabilities

One interesting direction is composition with platform capability information.

For example, a component may require:

```text
network access
persistent storage
GPU compute
```

The application could select implementations depending on the target environment.

However, this begins to overlap with deployment and infrastructure concerns.

VixC should remain focused on program semantics unless a strong language-level need appears.

## Composition and Vix.cpp

Vix.cpp is a natural place for application-level integration.

VixC can describe semantic requirements and providers.

Vix.cpp can know:

- project structure
- target platform
- build configuration
- dependencies
- generated artifacts

A possible long-term relationship is:

```text
VixC describes semantic composition facts
Vix.cpp assembles the application and validates project-level resolution
```

This is a research direction, not a committed architecture.

## VixC must remain independently embeddable

Even if Vix.cpp performs application-wide aggregation, VixC should not depend on the Vix CLI.

The frontend should be able to:

- parse composition declarations
- validate local semantics
- emit composition metadata

without knowing how the application is built.

Another build system could theoretically consume the same metadata.

## Runtime container

A composition model does not necessarily require a runtime service container.

If the dependency graph is known statically, VixC can generate ordinary C++ construction.

For example, the backend could emit equivalent code to:

```cpp
Database database(config);
Repository repository(database);
Service service(repository);
```

This has no need for runtime type lookup.

A runtime container should only be used when dynamic composition genuinely requires it.

## Static composition

Static composition has several attractive properties.

The frontend can know:

- whether requirements are satisfied
- which provider is selected
- initialization order
- concrete types
- lifetime relationships

The backend can generate ordinary C++.

This may produce good optimization and diagnostics.

The limitation is reduced runtime flexibility.

## Dynamic composition

Some applications genuinely need runtime selection.

Examples include:

- plugins
- optional modules
- user-selected backends
- feature loading
- dynamically discovered devices

A useful VixC model should not assume all composition is static.

Dynamic composition may require runtime registries or erased interfaces.

The language should distinguish static and dynamic relationships rather than forcing one representation.

## Plugins

Plugins are an especially important dynamic case.

A plugin may not exist when the main application is compiled.

Therefore, compile-time exhaustive provider validation is impossible.

The application can still validate:

```text
plugin must provide capability X
```

when loading it.

This becomes a runtime contract.

VixC may eventually help define such contracts without pretending every provider is statically known.

## Open and closed composition

A composition set can be:

```text
closed
```

where every provider is known during application assembly,

or:

```text
open
```

where external providers may appear later.

Closed composition enables stronger static validation.

Open composition enables plugin systems.

This distinction resembles closed versus open alternatives in Choice research.

The language should make the difference explicit if both models are supported.

## Components

A future model may introduce the concept of a component.

A component might group:

```text
provided capabilities
required capabilities
owned state
startup
shutdown
```

This can be useful conceptually.

It can also become a framework abstraction that duplicates classes and modules.

VixC should not introduce a `component` keyword unless real programs demonstrate that existing C++ types are not sufficient as the structural unit.

## Classes as components

One possible approach is to treat ordinary classes as composition units through metadata.

For example:

```cpp
class UserService
{
    // ordinary C++
};
```

could be annotated as requiring specific capabilities.

This preserves the object model.

The composition system becomes additional semantic information rather than a parallel class system.

This is likely preferable to inventing an entirely new component object model.

## Functions as components

Not every provider needs to be a class.

A function may provide stateless behavior.

For example:

```cpp
Response handle_request(
    const Request &request);
```

A composition model should not force all software into service objects.

Capabilities may map to functions or callable values as naturally as to classes.

## Values as providers

Configuration or immutable data can also be provided.

For example:

```text
ApplicationConfig
Clock
Logger
```

The model should therefore think in terms of semantic values and capabilities rather than only service classes.

## Namespaces are not components

Namespaces organize names.

They do not have runtime identity, ownership, construction, or destruction.

VixC should not confuse namespace structure with application composition.

The same applies to source directories.

## Modules are not necessarily components

C++ modules organize compilation and visibility.

A module may contain many runtime services or none.

Application composition and C++ modules may interact, but one should not be defined as the other.

## Capability granularity

Capabilities can be too broad or too narrow.

For example:

```text
Database
```

may be a useful abstraction.

But:

```text
ReadUser
WriteUser
BeginTransaction
RunQuery
```

could provide finer-grained requirements.

Smaller capabilities improve dependency precision.

Too many tiny capabilities increase complexity.

This is API design rather than something the language can solve automatically.

## Capability composition

A provider may provide several capabilities.

For example, one database object might provide:

```text
UserStore
TransactionManager
HealthCheck
```

The composition model should allow one value to satisfy multiple requirements where semantically appropriate.

It should not require constructing duplicate objects merely because several capabilities are exposed.

## Capability aliases

Different parts of an application may use different abstraction names for equivalent behavior.

Automatic aliasing could become confusing.

The language should probably prefer explicit relationships rather than guessing that similarly shaped capabilities are interchangeable.

## Structural compatibility

If capabilities are structural, two unrelated types may satisfy the same operation requirements.

This integrates naturally with C++ concepts.

However, a structural capability may accidentally match a type that has the right operations but different semantic meaning.

Nominal identity avoids that.

The research should test both models.

## Semantic capability identity

A capability may need identity stronger than a C++ method signature.

For example:

```text
Clock
```

and:

```text
RandomSource
```

might both expose:

```cpp
std::uint64_t next();
```

but they are not semantically interchangeable.

This argues for some nominal semantic identity at composition boundaries.

## Interfaces

Traditional abstract interfaces already provide nominal capability identity.

For example:

```cpp
struct Clock
{
    virtual Time now() = 0;
};
```

If this already solves the problem adequately, VixC should reuse it.

The composition feature should not require a new capability declaration when an ordinary C++ interface is sufficient.

## Concepts as capabilities

Concepts provide structural capability identity at compile time.

For example:

```cpp
template <typename T>
concept Clock =
    requires(T value)
    {
        value.now();
    };
```

A static composition system could potentially use concepts as provider constraints.

This would integrate strongly with existing C++.

The challenge is representing application-level provider identity and lifetime, which concepts alone do not solve.

## Named bindings

Sometimes two values of the same type serve different roles.

For example:

```text
primary Database
analytics Database
```

Type identity alone is insufficient.

A composition model may need named or qualified bindings.

This is common in dependency injection frameworks.

The syntax and semantic identity of qualifiers remain open.

## Qualifiers

A requirement might conceptually ask for:

```text
Database qualified as "analytics"
```

String qualifiers are simple but weakly typed.

Nominal qualifier declarations are safer but add language surface.

The first experiment should avoid this unless a real application requires multiple same-type providers.

## Scopes

Composition often needs provider scopes.

For example:

```text
application-scoped logger
request-scoped request context
operation-scoped transaction
```

Rather than hard-coding framework vocabulary, VixC could potentially derive scope from ownership structure.

For example, a provider owned by a request computation naturally has request lifetime.

This would integrate composition with ownership and async semantics more coherently.

## Structured provider lifetime

A strong direction is to tie provider lifetime to structured scopes already present in the program.

Conceptually:

```text
application scope owns Database
request scope owns Transaction
child services borrow from those owners
```

Then composition does not need a separate magic lifetime system.

It uses ordinary ownership relationships enriched with semantic composition metadata.

## Shutdown order

Provider destruction order matters.

If:

```text
Service depends on Repository
Repository depends on Database
```

then shutdown often needs the reverse order:

```text
Service
Repository
Database
```

RAII already provides reverse destruction for nested objects.

A statically generated composition can exploit this naturally.

This is another reason ordinary C++ construction may be preferable to a runtime service container.

## Async startup

A provider may require asynchronous initialization.

For example:

```text
connect to database
load remote configuration
initialize network service
```

This connects composition to async semantics.

Application assembly may need to wait for provider readiness before starting dependents.

No async composition semantics are currently defined.

## Async shutdown

Shutdown may also require asynchronous work.

Examples include:

- flush queue
- close network session
- commit telemetry
- stop child tasks

C++ destructors cannot suspend.

A future composition model may therefore need explicit async shutdown phases rather than relying only on object destruction.

This should be designed together with async research.

## Failure during startup

Provider startup may fail.

A static composition graph can tell VixC:

```text
which provider is being initialized
which dependents cannot start without it
```

Failure semantics can describe the runtime problem.

This may support diagnostics such as:

```text
application startup failed while initializing Database
```

with underlying:

```text
failure(ConnectionError)
```

The composition layer should add context without replacing Failure semantics.

## Failure during shutdown

Shutdown Failure is difficult.

The application may already be terminating or cancelling.

Several providers may fail during cleanup.

The language should avoid promising a simple single Failure channel until real shutdown semantics are studied.

This may require aggregation or diagnostics rather than ordinary propagation.

## Composition and `none`

An optional provider may naturally correspond to absence.

This could interact with the wider Outcome state:

```text
none
```

However, configuration absence and computation absence are not necessarily the same semantic concept.

The composition system should not reuse Outcome terminology automatically unless the models genuinely align.

## Composition and stopped

When an application scope is stopped, owned components may need cancellation.

For example:

```text
server scope stops
child network tasks stop
provider cleanup begins
```

This connects composition with structured concurrency.

A component model that ignores active child computations would have incomplete lifetime semantics.

## Composition and ownership

Composition declares relationships between consumers and providers.

Ownership determines who keeps those providers alive.

These models should work together.

A provider may be injected as:

```cpp
T &
```

while another may be transferred as:

```cpp
std::unique_ptr<T>
```

The composition system should preserve those ordinary C++ semantics rather than hide them behind universal opaque handles.

## Composition and reflection

Compile-time reflection may help composition inspect declarations and metadata.

For example, a frontend could discover explicitly annotated requirements on a class.

However, composition should not require unrestricted whole-program reflection.

Local declaration metadata plus application aggregation may be sufficient.

## Composition and Match

Dynamic provider selection may return several states:

```text
provider available
provider unavailable
provider rejected
```

Future Match semantics could help consume such results.

This is not central to the first composition experiment.

## Composition and compile-time validation

Many composition errors are good candidates for compile-time or build-time diagnostics:

```text
missing provider
ambiguous provider
invalid dependency cycle
lifetime incompatibility
unsatisfied configuration type
duplicate exclusive provider
```

This is one of the strongest arguments for making composition semantic rather than purely runtime.

## Runtime validation

Some relationships can only be checked at runtime.

For example:

```text
plugin loaded successfully
network capability available
dynamic configuration selects valid implementation
```

The composition system should not pretend such conditions are statically known.

A useful model distinguishes:

```text
statically invalid graph
```

from:

```text
valid graph whose runtime initialization may fail
```

## Diagnostic quality

Composition errors can become confusing in template-heavy frameworks.

For example, a missing dependency may surface as a long template-instantiation trace.

A VixC semantic composition model could instead report:

```text
'UserService' requires capability 'UserStore', but no provider is available
```

with source ranges pointing to the relevant declarations.

This diagnostic improvement may be more valuable than reducing construction syntax.

## Diagnostic provenance

Application-level composition may combine metadata from several translation units.

Diagnostics therefore need cross-file provenance.

A missing provider diagnostic might point to:

```text
requirement declaration in one file
application assembly in another file
candidate provider declarations elsewhere
```

VixC's source identity model must eventually support this broader context.

## Determinism

Static composition should be deterministic.

Provider selection must not depend on:

- filesystem enumeration order
- hash-map order
- linker accident
- source compilation order

If multiple candidates are equally valid, the system should require explicit resolution rather than choose arbitrarily.

This is important for reproducible builds.

## Registration order

Runtime registration systems often depend on static initialization order or explicit startup sequencing.

VixC should avoid making provider discovery depend on unspecified initialization order.

If registration is generated, the ordering should be deterministic and semantically defined.

## Static initialization

C++ global static initialization can create ordering problems across translation units.

A composition system that generates global registries may reproduce those problems.

Where possible, explicit application assembly and local RAII construction are safer.

VixC should avoid hiding composition behind static initialization.

## Service locator

A service locator provides globally accessible dependency lookup.

For example:

```cpp
services.get<Database>();
```

This is flexible but hides dependencies from function and constructor signatures.

A VixC composition model should be cautious about encouraging ambient dependency access.

If the frontend's goal is clearer composition, hidden global lookup may undermine it.

## Ambient context

Some dependencies are naturally contextual.

Examples include:

- current request
- tracing context
- cancellation token
- scheduler
- transaction

Passing every contextual value manually may become noisy.

Ambient context mechanisms can help but make dependencies less explicit.

This tradeoff belongs to composition and async research.

No ambient context model is currently selected.

## Dependency direction

Composition graphs can help enforce architectural direction.

For example:

```text
domain
    must not depend on transport
```

This resembles module architecture constraints.

Such policy may be valuable but is broader than provider resolution.

VixC should not turn the first composition experiment into a full architecture-rule language.

## Layering

Applications often define conceptual layers.

For example:

```text
UI
application
domain
infrastructure
```

Build systems and lint tools can enforce some dependency directions.

Composition semantics may expose enough information for tooling to validate layering later.

This is likely a tooling concern rather than core language semantics initially.

## Component visibility

A provider may be private to one composition scope.

Another may be exported to child scopes.

Visibility affects encapsulation.

The composition system should not automatically make every provider globally resolvable.

Scope-local composition is likely safer and easier to reason about.

## Nested composition scopes

An application may create nested scopes.

For example:

```text
application
    request
        transaction
```

Requirements in the transaction scope may use providers inherited from parent scopes.

Providers created in the transaction scope must not escape beyond it without appropriate ownership.

This naturally connects composition to lifetime regions.

## Scope shadowing

A nested scope may provide a different implementation for the same capability.

For example:

```text
application Logger
test scope TestLogger
```

This can be useful.

The language would need clear resolution rules.

Lexical-style nearest-scope selection is one possibility.

Explicit selection may be safer.

## Testing

Composition semantics could improve testing by allowing controlled provider substitution.

For example:

```text
production:
    Database -> PostgresDatabase

test:
    Database -> FakeDatabase
```

If substitution is explicit and type-checked, tests can replace infrastructure without changing service code.

This is a useful application, but it should not make testing policy part of the language itself.

## Mocks

A mock is simply another implementation satisfying a required capability.

VixC does not need a special mock concept.

Ordinary C++ test types should remain usable.

Composition metadata may make substitution easier.

## Configuration-specific providers

Different targets may use different implementations.

For example:

```text
Linux -> EpollNetwork
Windows -> IocpNetwork
```

Some selection may happen at build time.

Vix.cpp already knows target configuration.

A clean integration could allow project-level provider selection without introducing preprocessor conditionals into every consumer.

This is a promising area, but it touches build-system semantics.

## Preprocessor versus composition

C++ often selects implementations with:

```cpp
#if defined(...)
```

Composition metadata could potentially move some implementation selection out of conditional source.

However, the preprocessor remains necessary for many platform differences.

VixC should not claim composition eliminates conditional compilation.

## Plugin architecture

A mature composition model may help define plugin contracts.

A plugin could declare that it:

```text
provides capability X
requires capability Y
```

The host could validate those contracts when loading.

This would extend static composition semantics into runtime boundaries.

Such a model requires stable metadata and ABI.

It is far beyond the first experiment.

## ABI

If composition only supplies ordinary C++ values to constructors or functions, it may not require a new ABI.

This is desirable.

For example, generated composition can call:

```cpp
Service service(
    database,
    logger);
```

using normal C++ ABI.

A runtime capability system using erased handles may impose additional ABI requirements.

VixC should prefer ordinary C++ representation where possible.

## Binary components

Composition across shared-library boundaries introduces questions about:

- ABI stability
- ownership transfer
- exception boundaries
- Failure representation
- RTTI
- symbol visibility

The first composition experiment should remain within one native application build.

Binary plugin composition can be studied later.

## Templates and composition

Compile-time provider selection may involve templates.

For example, a service may be generic over its dependencies.

A composition engine could instantiate it with selected provider types.

This may create significant compile-time cost.

An alternative is runtime polymorphism.

VixC should not prescribe one dispatch model universally.

## Type erasure

Type erasure can provide runtime flexibility without requiring inheritance.

A capability could theoretically be represented through an erased callable or wrapper.

This remains ordinary C++ implementation strategy.

The semantic composition model should not depend on one dispatch representation.

## Performance

Composition itself should not impose unnecessary runtime overhead.

Static composition can often disappear into ordinary construction and direct calls.

Dynamic composition may require indirection.

Experiments should measure:

- startup cost
- allocation
- lookup overhead
- binary size
- generated code size
- native compile time

The cost should follow the dynamic behavior requested by the application.

## Compile time

A sophisticated compile-time dependency graph implemented through templates can become expensive.

VixC may be able to validate composition directly and emit simpler C++.

This is a possible advantage.

It must be measured against the added VixC frontend work.

## Generated code

A static composition backend should ideally generate understandable ordinary C++.

For example, the final output might resemble direct manual assembly:

```cpp
Database database(config);
Repository repository(database);
Service service(repository);
```

This makes semantics easy to inspect and gives native compilers straightforward code to optimize.

The exact generated form depends on ownership and initialization requirements.

## Generated names

Application assembly may require synthesized local variables.

VixC already has deterministic synthetic identity infrastructure in lowering.

Composition lowering could eventually use similar deterministic naming.

Generated names must not become user-visible semantic identity.

## Source mapping

Generated construction should map back to composition declarations.

If native C++ compilation fails because a provider constructor is incompatible, diagnostics should ideally point to the provider binding or requirement that caused the generated call.

This is another use of source provenance.

## Semantic IR

If composition becomes a VixC feature, semantic IR may include concepts such as:

```text
Requirement
Provider
Binding
CompositionScope
ConstructProvider
ResolveCapability
```

These names are provisional.

The IR should represent semantic relationships rather than a particular service-container API.

## Lowering

Static composition lowering may:

1. resolve requirements to providers
2. validate graph constraints
3. determine construction ordering
4. determine ownership relationships
5. synthesize provider construction
6. supply values to consumers
7. preserve reverse cleanup order

The backend can then emit ordinary C++.

Dynamic composition would require a different lowered representation.

## Graph validation

Composition naturally forms a graph.

Nodes may represent providers or consumers.

Edges represent requirements.

The frontend or application assembler can analyze:

- missing edges
- ambiguous resolution
- cycles
- lifetime conflicts
- unreachable providers

Graph algorithms are implementation mechanisms.

The language semantics should remain expressed in terms of requirements and providers.

## Unused providers

A provider may be declared but never required.

Should this produce a warning?

Sometimes unused providers indicate dead configuration.

Sometimes they exist for future dynamic lookup or optional features.

The correct behavior may depend on whether the composition scope is closed.

This should not become an unconditional warning.

## Unreachable components

A closed static application graph may contain a component not reachable from any application root.

The frontend could identify it.

Whether that is an error, warning, or acceptable depends on intended use.

This is likely tooling rather than core semantic validity.

## Application roots

Static composition needs some root from which required software is assembled.

For example:

```text
HTTPServer
```

may be an application root.

The graph resolves everything required to construct and run it.

How roots are declared is open.

Vix.cpp may already know executable entry points, but composition roots are not necessarily identical to `main`.

## `main`

One possible integration is to generate or wrap application startup around `main`.

However, VixC should be cautious about owning the program entry point.

Ordinary C++ applications should remain able to define:

```cpp
int main()
```

The composition system could instead produce an object or function that `main` invokes.

This preserves compatibility.

## Lifecycle

Some components need explicit lifecycle operations:

```text
construct
start
stop
destroy
```

Others only need construction and destruction.

A universal component lifecycle interface would be too heavy for simple values.

VixC should only introduce lifecycle semantics when components genuinely require them.

## Start ordering

If components have explicit `start` operations, the order may differ from construction.

For example:

```text
construct all components
then start network listeners
```

The composition model would need to represent lifecycle phases.

This should not be part of the first experiment unless necessary.

## Shutdown

Shutdown is even more complex when:

- child tasks remain active
- providers depend on one another
- cleanup can fail
- cancellation is in progress

This strongly connects composition with async and Failure.

The first composition work should probably focus on static construction rather than complete lifecycle management.

## Hot replacement

Some systems support replacing providers while the application is running.

For example:

```text
reload configuration
replace implementation
restart component
```

This is dynamic runtime architecture.

It requires strong ownership, synchronization, and state migration semantics.

It is outside the first composition scope.

## Distributed composition

A service may depend on something running on another process or machine.

For example:

```text
UserService requires PaymentService
```

Treating remote services as ordinary local providers could hide network Failure and deployment realities.

VixC composition should remain careful about boundaries.

A remote capability is not semantically identical to a local object reference.

## Local versus remote capability

A remote dependency may have:

- latency
- Failure
- cancellation
- serialization
- authentication
- availability

A composition system should not erase those properties merely because both local and remote dependencies satisfy a similar interface.

This is another reason composition should not become a universal abstraction over infrastructure.

## Security capabilities

The term capability also has security meaning.

A true security capability grants authority to perform an operation.

For example:

```text
filesystem write capability
network capability
```

VixC composition may eventually intersect this idea.

However, declaring a dependency is not automatically a security enforcement mechanism.

The research should not claim authority safety unless runtime and platform behavior actually enforce it.

## Composition and permissions

A component may require permission to access a resource.

This could potentially become part of semantic metadata.

For now, such permissions are better treated as external policy or platform integration.

The first composition model should focus on dependency satisfaction.

## Debugging composition

When application behavior depends on resolved providers, developers need to know what was selected.

Tooling could provide:

```text
UserService
    -> UserStore: PostgresUserStore
    -> Logger: ConsoleLogger
```

This may be useful even when the composition is fully static.

Structured semantic metadata enables this without runtime service lookup.

## Explainability

One advantage of semantic composition is the ability to explain why a provider was selected.

For example:

```text
'PostgresDatabase' provides 'Database'
selected by application binding at X
required by 'Repository' at Y
```

This kind of explanation is difficult when wiring is distributed across macros or template machinery.

## Tooling

Potential tooling includes:

- dependency graph inspection
- missing provider diagnostics
- provider navigation
- requirement navigation
- lifetime visualization
- startup ordering
- test substitutions
- configuration-specific graph views

This could provide value even if the source syntax remains minimal.

## Reflection integration

Compile-time reflection may allow tools to inspect provider metadata.

However, the composition system should own the actual semantic relationships.

Reflection should expose them after they exist.

It should not infer composition from arbitrary class structure automatically.

## Metadata annotations

The first composition experiment may use attributes or another lightweight metadata mechanism.

For example, one could imagine annotating:

```text
provides capability
requires capability
```

without introducing a new component grammar.

This would allow testing the semantic model before committing to syntax.

No annotation format is selected.

## External metadata

For third-party libraries, source annotations may be unavailable.

An external metadata mechanism could describe composition properties without modifying upstream code.

This resembles adapters in Failure and ownership research.

Such a system may eventually be needed for ecosystem interoperability.

It should not be designed before the basic local model works.

## Contextual keywords

If VixC introduces words such as:

```text
requires
provides
component
```

compatibility must be considered.

`requires` is already a C++ keyword used by concepts.

This makes it particularly unsuitable for casual repurposing.

VixC should avoid syntax that collides with existing C++ grammar unless the context is unambiguous.

## No syntax-first design

Composition is especially vulnerable to framework-looking syntax.

It is easy to invent source such as:

```text
component Service
requires Database
provides Handler
```

without understanding:

- ownership
- initialization
- separate compilation
- provider ambiguity
- runtime selection
- Failure
- async shutdown

The research should define the semantics first.

Syntax should come later.

## First useful experiment

The first composition experiment should be narrow and use ordinary C++ types.

For example:

```cpp
struct Logger
{
};

struct Database
{
};

struct UserService
{
    UserService(
        Database &database,
        Logger &logger);
};
```

VixC could attach semantic metadata stating that:

```text
Database provides Database capability
Logger provides Logger capability
UserService requires Database
UserService requires Logger
```

Then the application assembler could:

1. validate that each requirement has one provider
2. determine construction ordering
3. generate ordinary C++ assembly
4. preserve provider lifetimes
5. emit direct diagnostics for missing or ambiguous providers

This tests the core hypothesis without requiring a new object model.

## Failure experiment

A second experiment should allow provider construction to produce VixC Failure.

Conceptually:

```text
Database creation
    -> success(Database)
    -> failure(DatabaseError)
```

The application startup path must preserve that Failure explicitly.

This tests composition with the first real VixC semantic feature.

## Lifetime experiment

Another experiment should test scope incompatibility.

For example:

```text
Service has application lifetime
Database provider has shorter child-scope lifetime
```

The frontend should reject the binding if `Service` keeps a borrow beyond the provider's lifetime.

This would test composition with ownership semantics.

It should wait until the ownership model has enough precision.

## Async experiment

A later experiment could involve asynchronous startup.

For example:

```text
Database provider requires async connection
Service cannot start until Database is ready
application cancellation stops initialization
```

This would test interaction between:

```text
composition
Failure
async
stopped
```

It should not be the first implementation.

## What should not be the first experiment

The initial composition work should not attempt to solve:

- dynamic plugin loading
- distributed services
- hot reload
- whole-program reflection
- arbitrary runtime service lookup
- universal lifecycle hooks
- automatic remote proxies
- deployment
- security capability enforcement
- asynchronous destructors
- global configuration management

These would obscure the core semantic question.

## Questions about semantic identity

The research needs to answer:

- What exactly is a capability?
- Is it represented by a C++ type?
- Can concepts act as capabilities?
- Can one type provide several capabilities?
- Can two instances of the same type represent distinct providers?
- How are qualifiers represented?
- Can capabilities be parameterized?
- Is capability identity nominal or structural?

These questions should be answered before source syntax becomes stable.

## Questions about requirements

The research must define:

- whether requirements are mandatory by default
- how optional requirements work
- whether multiple providers can be requested
- whether requirements can be dynamic
- whether a requirement can specify lifetime constraints
- whether requirements can depend on configuration
- whether requirements appear in function signatures or separate metadata

The model should avoid hiding dependencies that ordinary C++ signatures already express well.

## Questions about providers

Provider semantics need answers to:

- What can provide a capability?
- Is the provider a value, type, function, or component?
- Who owns the provider?
- When is it constructed?
- Can it fail to initialize?
- Can it be asynchronous?
- Can it provide multiple capabilities?
- Can it be replaced dynamically?
- Can it exist in multiple instances?

These questions directly affect lowering.

## Questions about scopes

Composition scopes need answers to:

- How are scopes created?
- Which providers are visible in child scopes?
- Can providers be shadowed?
- How are lifetimes derived?
- Can values escape a scope?
- How does a scope shut down?
- Are child tasks part of scope lifetime?

This connects strongly with ownership and async research.

## Questions about application assembly

Application-level integration needs answers to:

- Where is the complete composition graph assembled?
- Does Vix.cpp perform graph resolution?
- What metadata does VixC emit?
- Can another build system consume it?
- How are cross-translation-unit declarations identified?
- How are diagnostics mapped back to source?
- How is provider selection configured per target?

This is the key boundary between frontend and build system.

## Questions about static versus dynamic resolution

The model needs to decide:

- which bindings must be known statically
- how dynamic providers are represented
- whether open composition scopes exist
- how plugin contracts are validated
- whether runtime lookup is built into VixC
- whether dynamic composition requires a runtime

The first implementation should prefer static resolution.

## Questions about lifecycle

If lifecycle becomes part of composition, the research must answer:

- Is construction enough?
- What is startup?
- Can startup fail?
- Can startup suspend?
- What is shutdown?
- Can shutdown fail?
- Can shutdown suspend?
- How are dependent components ordered?
- What happens during cancellation?

This is a large area and should remain separate from first provider resolution.

## Questions about diagnostics

Composition diagnostics should eventually cover:

```text
missing provider
ambiguous provider
invalid provider
dependency cycle
lifetime mismatch
invalid scope escape
duplicate exclusive binding
invalid application root
provider construction incompatibility
```

The diagnostic should explain application relationships directly rather than exposing generated wiring code.

## Questions about performance

Experiments should measure:

- compile-time graph resolution cost
- generated C++ size
- native compiler time
- runtime construction cost
- allocation
- indirection
- binary size

A static composition model should aim to add little or no runtime overhead beyond the ordinary C++ construction it represents.

## Questions about incremental builds

Changing one provider should not necessarily force every translation unit to rebuild.

If application assembly is separated from local compilation, Vix.cpp may be able to regenerate only composition output and affected dependents.

This requires stable metadata and dependency tracking.

The architecture should consider incremental builds early enough to avoid a whole-program recompilation model.

## What would justify VixC composition

Composition semantics would be justified if experiments demonstrate meaningful improvements such as:

- compile-time or build-time detection of missing dependencies
- deterministic provider selection
- substantially clearer diagnostics
- explicit lifetime relationships
- straightforward static C++ generation
- easy test substitution
- separation between semantic requirements and concrete implementations
- integration with Failure and async without a mandatory service container
- useful tooling over application structure
- acceptable compile-time cost

The benefit must be larger than simply writing fewer constructors.

## What would argue against language-level composition

The direction should be reconsidered if:

- ordinary constructors and factories already provide sufficient clarity
- useful composition requires a heavyweight runtime
- metadata becomes more verbose than manual wiring
- application-wide analysis damages incremental compilation
- templates or concepts already solve the target problem more cleanly
- third-party integration requires extensive adapters
- provider lifetime becomes too difficult to infer
- syntax turns VixC into a framework-specific language
- composition hides important C++ ownership or Failure behavior

In that case, composition may belong in libraries or Vix.cpp tooling rather than the language frontend.

## What must not become accidental specification

The following remain provisional:

- whether VixC has a `component` concept
- whether the term capability is used publicly
- whether requirements use dedicated syntax
- whether providers use dedicated syntax
- whether composition is static by default
- whether Vix.cpp performs application graph aggregation
- whether composition metadata is emitted separately
- whether provider selection uses types
- whether concepts can represent capabilities
- whether application scopes exist as language constructs
- whether provider lifetimes use named categories
- whether a runtime service container exists
- whether lifecycle operations are standardized
- whether plugins participate in the same model

Prototype implementation choices must not freeze these decisions.

## Relationship with frontend architecture

Composition should use the same semantic architecture as other VixC features.

A likely path includes:

- declaration recognition
- semantic requirement/provider information
- local validation
- semantic metadata or IR
- application-level graph resolution where required
- lowering
- C++ backend generation
- source provenance
- diagnostics

The backend should not discover dependencies by parsing strings or generated names.

The semantic relationships should already be explicit before code generation.

## Relationship with IR

Not every composition fact needs executable IR.

For example:

```text
Service requires Logger
```

may primarily be semantic metadata.

Once a provider is selected, executable IR may need operations representing:

```text
construct Logger
construct Service using Logger
```

The frontend should separate declarative composition information from actual generated initialization behavior.

## Relationship with Vix.cpp

Vix.cpp may become the natural application-level consumer of composition metadata.

A possible long-term model is:

```text
VixC
    understands local composition semantics
    emits deterministic semantic metadata

Vix.cpp
    gathers application units
    resolves target-specific composition
    asks VixC or a composition library to validate the graph
    compiles generated ordinary C++
```

This architecture is only a research possibility.

The important constraint is that VixC language semantics remain independently defined.

## Runtime independence

Static composition should not require a VixC runtime.

If all providers are known, VixC can generate ordinary C++ construction and rely on RAII.

Dynamic composition may require runtime support.

That runtime should be introduced only when dynamic behavior is explicitly requested.

The simple static case should remain simple.

## Research priority

Composition should not interrupt completion of the first Failure vertical slice.

It also depends conceptually on future progress in:

- ownership and lifetimes
- async and cancellation
- reflection
- semantic declaration modeling

The purpose of this research directory is to ensure those features evolve in compatible directions.

For example, Failure-aware provider initialization should remain possible.

Structured provider lifetime should remain possible.

Reflection should be able to observe composition contracts if they become stable semantics.

## Immediate research questions

Before designing syntax, composition research should answer a smaller set of questions.

First, what real Vix.cpp application problem cannot be expressed clearly enough with constructors, factories, concepts, and ordinary C++ types?

Second, can VixC validate requirements and providers while keeping generated code as direct ordinary C++ construction?

Third, what is the minimum semantic identity required for a capability?

Fourth, can provider lifetime be derived from ordinary C++ ownership rather than a separate scope system?

Fifth, how should composition information cross translation-unit boundaries?

Sixth, what belongs in VixC and what belongs in Vix.cpp application assembly?

Seventh, can the first implementation operate without any runtime container?

Eighth, do the resulting diagnostics and tooling provide enough value to justify language-level semantics?

These questions should be answered before a permanent composition surface is introduced.

## Research discipline

Composition can easily become a framework architecture disguised as a language feature.

VixC should distinguish:

```text
semantic dependency
C++ type relationship
ownership
build dependency
configuration
runtime service lookup
application lifecycle
deployment
```

These are different concerns.

A useful language feature should own only the relationships that need compiler-level meaning.

For example:

```text
Service requires capability Logger
```

may be semantic composition.

```text
Logger implementation comes from package X
```

is package/build information.

```text
Logger writes to /var/log/app.log
```

is runtime configuration.

```text
Logger runs on machine Y
```

is deployment.

Collapsing these into one composition system would make the language depend on infrastructure policy.

VixC should remain focused on software semantics.

## Current position

The current composition hypothesis is that VixC may eventually benefit from explicit semantic relationships between software requirements and providers.

The strongest initial direction is static composition over ordinary C++ values, with deterministic validation and generated direct construction.

The research does not currently commit to:

```text
a component keyword
a dependency injection container
a service locator
a mandatory runtime
a universal lifecycle interface
whole-program reflection
dynamic plugin composition
deployment semantics
```

The first useful evidence should come from a small ordinary C++ application where VixC can validate a dependency graph, generate straightforward native construction, preserve ownership and Failure semantics, and produce clearer diagnostics than equivalent framework machinery.

Only after that should composition become a candidate for stable VixC language semantics.
