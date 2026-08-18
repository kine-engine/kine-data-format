# KDF - Kine Data Format

KDF is a standalone C library for structured data serialization. It stores data in a typed data model and encodes that model in two formats:

- `.kdf` - human-readable text
- `.kdfb` - compact binary

The library has no external dependencies. It does not depend on any engine, framework, or runtime. You can use it in games, tools, editors, servers, or any C or C++ application.

## Features

- Pure C11. No C++ required.
- No dependencies beyond the C standard library.
- Allocator-driven API. You control memory allocation.
- Stream-based I/O. Read from files, memory buffers, archives, or any custom source.
- 16 value types: null, bool, int, uint, float, string, vec2, vec3, vec4, quat, color, array, object, asset ref, resource ref, subresource.
- String interning. Duplicate strings are stored once per document.
- Object properties preserve insertion order.
- Subresources and versioning for engine resource systems.
- Text format is human-editable. Binary format is compact and platform-independent.

## Quick Start

```c
#include <kdf/kdf.h>

int main(void) {
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");
    kdf_obj_set_int(root, "health", 100);
    kdf_obj_set_float(root, "speed", 5.5);
    kdf_obj_set_vec3(root, "position", 0.0f, 2.0f, 0.0f);

    kdf_object *physics = kdf_obj_add_object(root, "physics");
    kdf_obj_set_float(physics, "mass", 70.0);
    kdf_obj_set_bool(physics, "gravity", true);

    kdf_text_save(doc, "player.kdf");
    kdf_binary_save(doc, "player.kdfb");
    kdf_doc_destroy(doc);
    return 0;
}
```

This produces `player.kdf`:

```
kdf 1

name = "Player"
health = 100
speed = 5.5
position = vec3(0, 2, 0)
physics {
    mass = 70
    gravity = true
}
```

## Build

KDF supports both xmake and CMake.

### xmake

```sh
xmake build
xmake run kdf_tests
```

### CMake

```sh
cmake -B build
cmake --build build
ctest --test-dir build
```

See [docs/BUILDING.md](docs/BUILDING.md) for detailed instructions, including how to install xmake.

See [docs/USAGE.md](docs/USAGE.md) for the full API reference with examples.

## Text Format

The `.kdf` text format is designed for human editing. It supports typed constructors that JSON does not have:

```
kdf 1

resource type="Material" version=2 {
    shader = asset("res://shaders/pbr.shader")
    albedo = color(1, 0.2, 0.1, 1)
    metallic = 0.0
    roughness = 0.65

    textures {
        albedo = asset("res://textures/player_albedo.png")
        normal = asset("res://textures/player_normal.png")
    }
}
```

## Binary Format

The `.kdfb` binary format stores the same logical data in a compact, platform-independent encoding. It uses:

- Little-endian byte order
- A string table with interning
- Typed value tags
- No padding or alignment dependencies

The binary format is not a dump of C structs. It is a stable, versioned wire format.

## License

MIT. See [LICENSE](LICENSE).
