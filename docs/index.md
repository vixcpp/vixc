# Vix++

Vix++ is a thin language layer for Vix-powered C++ applications.

It keeps the power of standard C++, but removes part of the friction around includes, build flow, and runtime usage.

Vix++ does not replace C++. It transpiles `.vix` files into standard `.cpp` files, then delegates the build and execution to the existing `vix` CLI.

## Goal

The goal of Vix++ is simple:

```txt
write cleaner C++
generate standard C++
build with Vix
```

A Vix++ file looks like this:

```cpp
use vix.core;
use std.iostream;

int main()
{
  std::cout << "Hello from Vix++\n";
  return 0;
}
```

The generated C++ looks like this:

```cpp
#include <vix/core.hpp>
#include <iostream>

int main()
{
  std::cout << "Hello from Vix++\n";
  return 0;
}
```

## Why Vix++ exists

C++ builds can become complex because headers, include paths, libraries, compiler flags, dependency setup, and build systems often have to be managed manually.

Vix already improves this with:

```sh
vix run main.cpp
vix build main.cpp
vix install
vix cache
```

Vix++ adds a small language layer on top:

```sh
vix++ run main.vix
```

The `.vix` file is converted to `.cpp`, then Vix handles the build.

## Relationship with Vix

Vix++ is separate from Vix.

```txt
Vix     = runtime and build engine for standard C++
Vix++   = language frontend for .vix files
```

The pipeline is:

```txt
main.vix
  ↓
vix++ transpiler
  ↓
.vix/build/vixpp/main.generated.cpp
  ↓
vix run/build/check
  ↓
binary
```

This keeps the Vix CLI stable and keeps Vix++ free to evolve.

## File extension

Vix++ source files use:

```txt
.vix
```

Example: `main.vix`

## Commands

Run a Vix++ file:

```sh
vix++ run main.vix
```

Build a Vix++ file:

```sh
vix++ build main.vix
```

Check a Vix++ file:

```sh
vix++ check main.vix
```

Shortcut:

```sh
vix++ main.vix
```

This is equivalent to `vix++ run main.vix`.

## Imports

Vix++ supports `use` declarations:

```cpp
use vix.core;
use vix.json;
use std.vector;
use local.config;
```

They are converted to C++ includes:

```cpp
#include <vix/core.hpp>
#include <vix/json.hpp>
#include <vector>
#include "config.hpp"
```

## MVP rules

The first version of Vix++ is intentionally small. It supports:

- `use vix.<module>;`
- `use std.<header>;`
- `use local.<header>;`

The rest of the file remains normal C++. That means this is valid Vix++:

```cpp
use vix.core;
use std.string;

int main()
{
  vix::App app;
  return app.run();
}
```

## Design principle

Vix++ should stay close to C++. It should improve the developer experience without hiding the language too much.

```txt
Vix++ makes C++ easier to write.
Vix keeps C++ fast and buildable.
```

## Current status

Vix++ is currently experimental. The first version focuses on:

- import syntax
- portable transpilation
- diagnostics
- generated C++
- delegation to Vix
- basic tests

Future versions can add better diagnostics, import maps, dependency-aware imports, formatting, linting, and safer C++ patterns.
