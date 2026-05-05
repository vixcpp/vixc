# Vix++ Syntax

Vix++ is intentionally close to standard C++.

A `.vix` file is mostly normal C++ with a small set of Vix++ rules added on top.

The first syntax feature is:

```cpp
use vix.core;
use std.iostream;
```

These declarations are converted to C++ `#include` directives before the file is passed to `vix`.

## Minimal example

```cpp
use std.iostream;

int main()
{
  std::cout << "Hello from Vix++\n";
  return 0;
}
```

Generated C++:

```cpp
#include <iostream>

int main()
{
  std::cout << "Hello from Vix++\n";
  return 0;
}
```

## Basic rule

A Vix++ source file has this shape:

```cpp
use vix.core;
use vix.json;
use std.string;

// normal C++ starts here
int main()
{
  return 0;
}
```

The `use` declarations must appear before regular C++ code. Blank lines and `//` comments are allowed before and between imports.

Valid:

```cpp
// Standard library import
use std.iostream;

// Vix runtime import
use vix.core;

int main()
{
  return 0;
}
```

Invalid:

```cpp
int main()
{
  return 0;
}

use vix.core;
```

The invalid version produces a diagnostic because the import appears after normal C++ code.

## Import declaration

The syntax is:

```cpp
use module.name;
```

Examples:

```cpp
use vix.core;
use vix.json;
use std.vector;
use local.config;
```

The declaration must end with a semicolon.

Valid:

```cpp
use vix.http;
```

Invalid:

```cpp
use vix.http
use ;
use vix..http;
```

## Supported import families

### `vix.*`

Vix imports map to Vix headers:

```cpp
use vix.core;
use vix.json;
use vix.http;
```

Generated C++:

```cpp
#include <vix/core.hpp>
#include <vix/json.hpp>
#include <vix/http.hpp>
```

Nested Vix imports are also supported by convention:

```cpp
use vix.http.server;
```

Generated C++:

```cpp
#include <vix/http/server.hpp>
```

### `std.*`

Standard library imports map to standard C++ headers:

```cpp
use std.iostream;
use std.vector;
use std.string;
```

Generated C++:

```cpp
#include <iostream>
#include <vector>
#include <string>
```

### `local.*`

Local imports map to quoted local headers:

```cpp
use local.config;
use local.services.user;
```

Generated C++:

```cpp
#include "config.hpp"
#include "services/user.hpp"
```

## Normal C++ remains valid

After the `use` section, the rest of the file is standard C++:

```cpp
use std.iostream;
use std.string;

static std::string message()
{
  return "Hello";
}

int main()
{
  std::cout << message() << "\n";
  return 0;
}
```

Vix++ does not change functions, classes, namespaces, templates, lambdas, memory management, or C++ types in the MVP.

## Comments

Line comments are preserved:

```cpp
// This comment stays in the generated C++
use std.iostream;

int main()
{
  // This comment also stays
  std::cout << "Hello\n";
}
```

Block comments are copied as normal text, but the MVP only treats `//` comments as comment lines when deciding whether imports are still allowed.

Recommended style:

```cpp
// Imports
use std.iostream;
use vix.core;
```

## Duplicate imports

Duplicate imports are deduplicated.

Input:

```cpp
use std.iostream;
use std.iostream;

int main()
{
  return 0;
}
```

Generated C++:

```cpp
#include <iostream>

int main()
{
  return 0;
}
```

## Generated file

When running:

```sh
vix++ run main.vix
```

Vix++ generates:

```txt
.vix/build/vixpp/main.generated.cpp
```

Then it delegates to Vix:

```sh
vix run .vix/build/vixpp/main.generated.cpp
```

## Current MVP syntax

Supported now:

- `use vix.<module>;`
- `use std.<header>;`
- `use local.<header>;`
- Everything else is normal C++

Not supported yet:

- `fn main()`
- `let value`
- class sugar
- automatic namespace imports
- package metadata imports
- module aliases

These can be added later without changing the first rule.

## Design rule

Vix++ should not hide C++. It should remove friction only where C++ is unnecessarily hard to use.

Current focus:

- clean imports
- portable transpilation
- clear diagnostics
- standard C++ output
- Vix-powered build
