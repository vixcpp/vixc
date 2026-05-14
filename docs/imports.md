# Vix++ Imports

Vix++ imports are written with `use`.

They are converted into standard C++ `#include` directives before the generated file is passed to `vix`.

## Basic idea

Instead of writing:

```cpp
#include <vix/core.hpp>
#include <vix/json.hpp>
#include <iostream>
```

You can write:

```cpp
use vix.core;
use vix.json;
use std.iostream;
```

Vix++ generates:

```cpp
#include <vix/core.hpp>
#include <vix/json.hpp>
#include <iostream>
```

## Import syntax

The syntax is:

```cpp
use module.name;
```

Examples:

```cpp
use vix.core;
use vix.http;
use std.vector;
use local.config;
```

Rules:

- the line must start with `use`
- the module name must not be empty
- the declaration must end with `;`
- the module name must use dots
- imports must appear before normal C++ code

## Import families

Vix++ supports three import families in the MVP: `vix.*`, `std.*`, and `local.*`.

### Vix imports

`vix.*` imports map to Vix headers:

```cpp
use vix.core;   // → #include <vix/core.hpp>
use vix.json;   // → #include <vix/json.hpp>
use vix.http;   // → #include <vix/http.hpp>
```

Nested imports are supported by convention:

```cpp
use vix.http.server;
```

Generated C++:

```cpp
#include <vix/http/server.hpp>
```

Rule: `use vix.a.b;` → `#include <vix/a/b.hpp>`

### Standard library imports

`std.*` imports map to standard C++ headers:

```cpp
use std.iostream;      // → #include <iostream>
use std.vector;        // → #include <vector>
use std.unordered_map; // → #include <unordered_map>
```

Rule: `use std.header;` → `#include <header>`

### Local imports

`local.*` imports map to local quoted headers:

```cpp
use local.config;         // → #include "config.hpp"
use local.services.user;  // → #include "services/user.hpp"
```

Rule: `use local.a.b;` → `#include "a/b.hpp"`

## Exact mappings

Some imports are mapped explicitly by the resolver.

Vix mappings:

```txt
vix.core       -> <vix/core.hpp>
vix.http       -> <vix/http.hpp>
vix.json       -> <vix/json.hpp>
vix.websocket  -> <vix/websocket.hpp>
vix.async      -> <vix/async.hpp>
vix.db         -> <vix/db.hpp>
vix.orm        -> <vix/orm.hpp>
vix.cache      -> <vix/cache.hpp>
vix.crypto     -> <vix/crypto.hpp>
vix.sync       -> <vix/sync.hpp>
vix.p2p        -> <vix/p2p.hpp>
```

Standard mappings:

```txt
std.vector         -> <vector>
std.string         -> <string>
std.string_view    -> <string_view>
std.iostream       -> <iostream>
std.filesystem     -> <filesystem>
std.optional       -> <optional>
std.variant        -> <variant>
std.memory         -> <memory>
std.unordered_map  -> <unordered_map>
std.map            -> <map>
std.set            -> <set>
std.array          -> <array>
std.algorithm      -> <algorithm>
std.chrono         -> <chrono>
std.thread         -> <thread>
std.mutex          -> <mutex>
std.atomic         -> <atomic>
std.functional     -> <functional>
```

## Deduplication

Duplicate imports are emitted only once.

Input:

```cpp
use vix.json;
use vix.json;
use std.iostream;
use std.iostream;

int main()
{
  return 0;
}
```

Generated C++:

```cpp
#include <vix/json.hpp>
#include <iostream>

int main()
{
  return 0;
}
```

## Import position

Imports must appear before regular C++ code.

Valid:

```cpp
// Imports
use std.iostream;
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

use std.iostream;
```

The invalid version produces an error:

```txt
import declarations must appear before regular C++ code
```

## Comments and blank lines

Blank lines and `//` comments are allowed before and between imports:

```cpp
// Standard library
use std.iostream;

// Vix runtime
use vix.core;

int main()
{
  return 0;
}
```

Block comments are preserved, but the MVP import scanner only treats `//` as comment lines for import-position detection.

## Invalid import names

Invalid:

```cpp
use ;
use .vix;
use vix.;
use vix..http;
use vix/http;
use vix-http;
```

Valid:

```cpp
use vix.http;
use std.string_view;
use local.services.user;
```

Import names may contain: letters, digits, underscores, and dots between segments.

## Generated file

For:

```sh
vixc run main.vix
```

Vix++ writes:

```txt
.vix/build/vixc/main.generated.cpp
```

Then delegates to:

```sh
vix run .vix/build/vixc/main.generated.cpp
```

## Why not C++20 `import`?

C++ already has a standard `import` keyword for modules. Vix++ uses `use` instead to avoid confusion with C++20 modules.

This keeps the MVP simple:

```txt
use    = Vix++ source-level import resolver
import = standard C++ modules keyword
```

Later, Vix++ can decide whether a `use` declaration should generate `#include <...>` or `import ...;` depending on compiler support and package metadata.

## Future import map

Later, Vix++ can support a project import map:

```json
{
  "imports": {
    "app.config": {
      "path": "src/config/AppConfig.hpp",
      "kind": "local"
    },
    "vendor.json": {
      "path": "nlohmann/json.hpp",
      "kind": "system"
    }
  }
}
```

Then:

```cpp
use app.config;
use vendor.json;
```

Could generate:

```cpp
#include "src/config/AppConfig.hpp"
#include <nlohmann/json.hpp>
```

## Design rule

Imports should stay predictable. The developer should be able to understand exactly what Vix++ will generate.

```txt
use vix.x    -> #include <vix/x.hpp>
use std.x    -> #include <x>
use local.x  -> #include "x.hpp"
```
