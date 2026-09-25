# VixC

VixC is a programming-language frontend for native software.

It is designed to explore and implement programming semantics that are difficult to express coherently through existing C++ mechanisms alone, while preserving access to native performance, existing toolchains, and the C++ ecosystem.

The first generation of VixC accepts source code, performs syntax and semantic analysis, builds an intermediate representation, lowers VixC semantics, and emits ordinary C++ that can be compiled by GCC, Clang, or MSVC.

C++ is therefore the first compilation target of VixC, but the frontend is designed around its own semantic model rather than around source rewriting. The language model, diagnostics, intermediate representation, and lowering rules belong to VixC. C++ emission is one backend for that model.

## Why VixC exists

C++ has accumulated several decades of language evolution, libraries, compiler extensions, conventions, and programming techniques. This gives developers an unusually large amount of control, but it also means that important programming problems can have several competing representations with different semantics, error models, composition rules, and tooling support.

VixC studies those problems at the frontend level.

The goal is not to introduce new syntax whenever C++ feels verbose. A new mechanism must represent a semantic distinction that is difficult to preserve through libraries, conventions, static analysis, diagnostics, or existing language facilities.

When C++ already provides a strong solution, VixC can use it. When several approaches exist, VixC can investigate whether one model provides a clearer direction. When the required semantics cannot be expressed reliably through those mechanisms, the frontend can introduce language support and lower it into the native compilation pipeline.

This keeps the project focused on semantics rather than syntax for its own sake.

## Language model

VixC treats syntax, semantics, lowering, and code generation as separate responsibilities.

Parsing determines the structure of the source program. Semantic analysis determines what that structure means and whether it is valid. The intermediate representation preserves those semantics independently from their source spelling. Lowering progressively converts higher-level constructs into forms that a backend can implement.

This separation is important because the generated C++ must not become the definition of VixC semantics. A VixC construct should have one meaning even if its implementation changes or another backend is introduced later.

Diagnostics follow the same principle. Errors are produced from the frontend's understanding of the program and remain connected to the original source locations instead of exposing implementation details from generated code whenever VixC has enough information to explain the problem itself.

## Failure and outcome

Failure and outcome are the first semantic area being implemented through the complete frontend.

The research begins from a recurring problem in native software: successful computation, legitimate absence, operational failure, cancellation, and programmer errors are often represented through mechanisms that look similar while carrying different meanings.

VixC is investigating a model in which those states remain semantically distinct and can be propagated without losing domain-specific failure information.

This work is not limited to defining a result container. It includes function contracts, propagation, control flow, diagnostics, interaction with existing C++ types, lowering rules, and the boundary between recoverable outcomes and contract violations.

The feature will only be considered established when the same semantics survive parsing, semantic analysis, intermediate representation, lowering, generated C++, diagnostics, and real programs.

## Relationship with C++

VixC begins from C++ rather than treating C++ as an external system.

Ordinary C++ remains available for types, templates, RAII, value semantics, move semantics, references, libraries, platform APIs, and existing native code. VixC does not require the ecosystem to be recreated behind a foreign-function boundary.

The frontend owns the semantics it introduces. It does not need to rename existing C++ concepts merely to place them under a VixC identity.

This distinction allows VixC to evolve without making compatibility the only design constraint. Early generations can remain very close to C++, while later research can determine whether some areas require deeper language changes.

## Relationship with Vix.cpp

Vix.cpp and VixC have different responsibilities.

Vix.cpp is a C++ development platform concerned with the practical lifecycle of native applications, including project workflows, dependencies, builds, execution, testing, diagnostics, and production-oriented tooling.

VixC owns frontend and language semantics.

The Vix.cpp CLI can integrate VixC as a reusable component before handing accepted output to its existing compilation pipeline. The CLI does not need to implement VixC syntax, semantic rules, lowering, or diagnostics itself.

The same frontend must remain usable independently by other software. An editor, IDE integration, compiler driver, build system, language server, or research tool should be able to embed VixC without depending on the Vix.cpp CLI.

This keeps language development independent from the application platform while still allowing both projects to work together.

## Research direction

VixC is being developed around a small number of difficult programming-language problems rather than around a predetermined feature list.

The current work covers failure and outcomes, choice and pattern matching, asynchronous work and cancellation, ownership and lifetime expression, compile-time programming and reflection, and program composition.

These areas do not automatically become language features. Each one must first establish its semantics, demonstrate where existing C++ mechanisms are sufficient or insufficient, and survive implementation in real programs.

A research result can therefore produce a language construct, a library mechanism, a static analysis rule, a diagnostic improvement, a tooling feature, or a decision that no VixC mechanism is necessary.

The architecture is intended to let those decisions evolve without requiring a new frontend for every experiment.

## Future language

VixC is the frontend and research implementation. It does not require the eventual language, if one emerges, to be named VixC.

The project can continue to serve as the compiler frontend even if the language later receives an independent identity.

That distinction matters because the name and public identity of a language should follow from what the language becomes, not constrain the research before its semantics are mature.

The current work is therefore focused on building the frontend and proving its programming model rather than declaring a finished language prematurely.

## Current status

VixC is at the beginning of its new frontend generation.

The previous experimental implementation explored a different direction and remains available through the Git history. The current implementation starts from a new architecture centered on source management, diagnostics, syntax, semantic analysis, intermediate representation, lowering, and backend generation.

Failure and outcome are the first semantics being taken through that complete pipeline.

The first major milestone is a real native program whose VixC semantics are parsed and validated by the frontend, represented independently from generated C++, lowered deterministically, compiled through an existing C++ toolchain, and integrated into the normal Vix.cpp workflow.

## Maintained by Softadastra

VixC is maintained by [Softadastra](https://softadastra.com/).

## License

VixC is available under the MIT License. See [`LICENSE`](LICENSE) for details.
