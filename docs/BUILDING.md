# Building KDF

KDF uses C11. It has no external dependencies. You can build it with xmake or CMake.

## Prerequisites

- A C11 compiler (GCC, Clang, or MSVC)
- xmake or CMake 3.10+

## Installing xmake

xmake is a cross-platform build system. Install it from https://xmake.io.

### Linux / macOS

```sh
curl -fsSL https://xmake.io/shget.text | bash
```

Or with a package manager:

```sh
# Arch Linux
pacman -S xmake

# macOS (Homebrew)
brew install xmake
```

### Windows

Download the installer from https://xmake.io/#/getting-started?id=installation-on-windows.

Or with Scoop:

```sh
scoop install xmake
```

### Verify

```sh
xmake --version
```

## Build with xmake

```sh
# Configure (debug mode is the default)
xmake f -m debug

# Build the static library
xmake build kdf

# Build and run tests
xmake build kdf_tests
xmake run kdf_tests
```

To build in release mode:

```sh
xmake f -m release
xmake build kdf
```

## Build with CMake

```sh
# Configure
cmake -B build

# Build
cmake --build build

# Run tests
ctest --test-dir build
```

### CMake Options

| Option            | Default | Description           |
| ----------------- | ------- | --------------------- |
| `KDF_BUILD_TESTS` | `ON`    | Build the test binary |

To disable tests:

```sh
cmake -B build -DKDF_BUILD_TESTS=OFF
```

## Build with gcc Directly

If you do not have xmake or CMake, compile with gcc:

```sh
# Compile source files
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_document.c -o build/kdf_document.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_value.c -o build/kdf_value.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_object.c -o build/kdf_object.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_array.c -o build/kdf_array.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_io.c -o build/kdf_io.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_text_writer.c -o build/kdf_text_writer.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_text_parser.c -o build/kdf_text_parser.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_binary_writer.c -o build/kdf_binary_writer.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src/kdf_binary_reader.c -o build/kdf_binary_reader.o

# Create static library
ar rcs build/libkdf.a build/*.o

# Compile and link tests
gcc -std=c11 -Wall -Wextra -Iinclude tests/test_kdf.c -Lbuild -lkdf -lm -o build/kdf_tests

# Run tests
./build/kdf_tests
```

## Using KDF in Your Project

### Option 1: Add as a Subdirectory

Copy the KDF source into your project (for example, as `thirdparty/kdf/`) and add it to your build system.

**CMake:**

```cmake
add_subdirectory(thirdparty/kdf)
target_link_libraries(your_target PRIVATE kdf)
```

**xmake:**

```lua
add_requires("kdf")
-- or add the local path
```

### Option 2: Install System-Wide

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

Then in your project:

```cmake
find_package(kdf REQUIRED)
target_link_libraries(your_target PRIVATE kdf)
```

### Option 3: Copy Source Files

Copy `include/kdf/` to your include path and `src/*.c` to your source tree. Add the `.c` files to your build. No other setup is needed.
