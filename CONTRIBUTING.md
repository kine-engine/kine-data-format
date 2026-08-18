# Contributing to KDF

## Code Standard

- C11. No C++ features.
- No external dependencies.
- All public headers must have `extern "C"` guards.
- All public functions use the `kdf_` prefix with a category infix: `kdf_doc_`, `kdf_obj_`, `kdf_arr_`, `kdf_val_`, `kdf_text_`, `kdf_binary_`.
- Internal functions use the `kdf__` prefix (double underscore).
- Use `size_t` for sizes and counts. Use `int64_t`/`uint64_t` for integer values. Use `double` for floating-point values.

## File Layout

- Public headers go in `include/kdf/`.
- Internal headers go in `src/`.
- Implementation files go in `src/`.
- Tests go in `tests/`.

## Building

Use xmake or CMake. See [docs/BUILDING.md](docs/BUILDING.md).

## Running Tests

```sh
# xmake
xmake run kdf_tests

# CMake
cmake --build build && ctest --test-dir build
```

All tests must pass before you submit a change.

## Adding a New Type

1. Add the type tag to `kdf_type` in `include/kdf/kdf_types.h`.
2. Add the binary tag to the `enum` in `src/kdf_internal.h`.
3. Add setter functions to `include/kdf/kdf_object.h` and `include/kdf/kdf_array.h`.
4. Add getter functions to `include/kdf/kdf_value.h`.
5. Implement the setter/getter in `src/kdf_object.c` and `src/kdf_array.c`.
6. Add text writing support in `src/kdf_text_writer.c`.
7. Add text parsing support in `src/kdf_text_parser.c`.
8. Add binary writing support in `src/kdf_binary_writer.c`.
9. Add binary reading support in `src/kdf_binary_reader.c`.
10. Add tests in `tests/test_kdf.c`.

## Commit Messages

Use clear, short commit messages. Describe what changed and why. Do not use vague messages like "fix stuff" or "update".

## Pull Requests

- Keep each pull request focused on one change.
- Make sure all tests pass.
- Do not add dependencies.
