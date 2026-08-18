# Using KDF

## Include

```c
#include <kdf/kdf.h>
```

This includes all KDF headers. You can also include individual headers if you only need part of the API.

## Documents

A `kdf_document` owns all memory for a data tree. Create one, populate it, serialize it, then destroy it.

```c
kdf_document *doc = kdf_doc_create();
kdf_object *root = kdf_doc_root(doc);

/* ... populate root ... */

kdf_text_save(doc, "output.kdf");
kdf_doc_destroy(doc);
```

The root object is always valid for a non-NULL document. You never need to null-check `kdf_doc_root()` if `kdf_doc_create()` returned non-NULL.

## Custom Allocator

Pass a `kdf_allocator` to control memory allocation:

```c
kdf_allocator alloc;
alloc.alloc = my_alloc;
alloc.realloc = my_realloc;
alloc.free = my_free;
alloc.userdata = my_arena;

kdf_document *doc = kdf_doc_create_alloc(&alloc);
```

If you pass `NULL`, the library uses `malloc`, `realloc`, and `free`.

The allocator callback signatures are:

```c
void *(*alloc)(void *userdata, size_t size);
void *(*realloc)(void *userdata, void *ptr, size_t old_size, size_t new_size);
void  (*free)(void *userdata, void *ptr, size_t size);
```

The `old_size` parameter is provided so arena allocators can avoid tracking individual allocations.

## Objects

An object is a set of named properties. Property order is preserved.

### Setting Properties

```c
kdf_object *obj = kdf_doc_root(doc);

kdf_obj_set_null(obj, "empty");
kdf_obj_set_bool(obj, "alive", true);
kdf_obj_set_int(obj, "health", 100);
kdf_obj_set_uint(obj, "score", 9999u);
kdf_obj_set_float(obj, "speed", 5.5);
kdf_obj_set_string(obj, "name", "Player");
kdf_obj_set_vec2(obj, "pos2d", 10.0f, 20.0f);
kdf_obj_set_vec3(obj, "position", 0.0f, 2.0f, 0.0f);
kdf_obj_set_vec4(obj, "rect", 0.0f, 0.0f, 1920.0f, 1080.0f);
kdf_obj_set_quat(obj, "rotation", 0.0f, 0.0f, 0.0f, 1.0f);
kdf_obj_set_color(obj, "tint", 1.0f, 0.5f, 0.2f, 1.0f);
kdf_obj_set_asset_ref(obj, "texture", "res://textures/player.png");
kdf_obj_set_resource_ref(obj, "mesh", "res://models/player.mesh");
```

Setting a property that already exists replaces the old value.

### Nested Objects

```c
kdf_object *physics = kdf_obj_add_object(obj, "physics");
kdf_obj_set_float(physics, "mass", 70.0);
kdf_obj_set_bool(physics, "gravity", true);
```

### Subresources

A subresource is an object with a type name. Use it for engine resources that carry metadata:

```c
kdf_object *mat = kdf_obj_add_subresource(obj, "material", "Material");
kdf_obj_set_float(mat, "roughness", 0.5);
kdf_obj_set_asset_ref(mat, "shader", "res://shaders/pbr.shader");
```

### Object Metadata

```c
kdf_obj_set_type(obj, "Character");
kdf_obj_set_version(obj, 3);

const char *type = kdf_obj_type(obj);  /* "Character" */
int version = kdf_obj_version(obj);    /* 3 */
```

### Getting Properties

```c
const char *name = kdf_obj_get_string(obj, "name", "unknown");
int health = kdf_obj_get_int(obj, "health", 0);
double speed = kdf_obj_get_float(obj, "speed", 1.0);
bool alive = kdf_obj_get_bool(obj, "alive", false);
```

Each getter takes a fallback value. If the property does not exist or has a different type, the fallback is returned.

For direct value access:

```c
const kdf_value *val = kdf_obj_get_const(obj, "position");
if (kdf_val_type(val) == KDF_TYPE_VEC3) {
    const float *v = kdf_val_as_vec3(val);
    /* use v[0], v[1], v[2] */
}
```

### Iteration

```c
for (const kdf_entry *e = kdf_obj_first(obj); e; e = kdf_obj_next(e)) {
    const char *key = kdf_entry_key(e);
    const kdf_value *val = kdf_entry_value(e);
    /* ... */
}
```

Iteration follows insertion order.

### Removal

```c
kdf_obj_remove(obj, "speed");   /* remove one property */
kdf_obj_clear(obj);             /* remove all properties */
```

## Arrays

An array is an ordered list of values. Values in an array can have different types.

### Pushing Values

```c
kdf_array *arr = kdf_obj_add_array(obj, "items");

kdf_arr_push_string(arr, "sword");
kdf_arr_push_int(arr, 42);
kdf_arr_push_float(arr, 3.14);
kdf_arr_push_bool(arr, true);
kdf_arr_push_null(arr);
kdf_arr_push_vec3(arr, 1.0f, 2.0f, 3.0f);
kdf_arr_push_color(arr, 1.0f, 0.0f, 0.0f, 1.0f);
kdf_arr_push_asset_ref(arr, "res://textures/icon.png");
```

### Accessing Elements

```c
size_t count = kdf_arr_count(arr);
const kdf_value *first = kdf_arr_get_const(arr, 0);
const char *s = kdf_val_as_string(first);
```

### Modification

```c
kdf_arr_remove(arr, 1);  /* remove element at index 1 */
kdf_arr_clear(arr);       /* remove all elements */
```

## Serialization

### Text Format

```c
/* Write to file */
kdf_text_save(doc, "output.kdf");

/* Read from file */
kdf_document *doc = kdf_text_load("output.kdf", NULL);

/* Write to memory */
kdf_mem_writer mw = {0};
kdf_writer w = kdf_mem_writer_create(&mw);
kdf_text_write(doc, w);
/* mw.data contains the text, mw.size is the length */
kdf_mem_writer_finish(&mw);

/* Read from memory */
kdf_mem_reader mr;
mr.data = my_bytes;
mr.size = my_len;
mr.pos = 0;
kdf_reader r = kdf_mem_reader_create(&mr);
kdf_document *doc = kdf_text_read(r, NULL);
```

### Binary Format

```c
/* Write to file */
kdf_binary_save(doc, "output.kdfb");

/* Read from file */
kdf_document *doc = kdf_binary_load("output.kdfb", NULL);
```

Binary serialization uses the same API as text. The only difference is the function names (`kdf_binary_*` instead of `kdf_text_*`).

### Custom I/O

Implement the `kdf_reader` or `kdf_writer` callbacks to read from or write to any source:

```c
size_t my_write(void *userdata, const void *data, size_t size) {
    my_stream *s = (my_stream *)userdata;
    return stream_write(s, data, size);
}

kdf_writer w;
w.userdata = &my_stream;
w.write = my_write;
kdf_text_write(doc, w);
```

## Value Types

| Type                    | C getter                  | Notes                 |
| ----------------------- | ------------------------- | --------------------- |
| `KDF_TYPE_NULL`         | -                         | No data               |
| `KDF_TYPE_BOOL`         | `kdf_val_as_bool`         | true/false            |
| `KDF_TYPE_INT`          | `kdf_val_as_int`          | 64-bit signed         |
| `KDF_TYPE_UINT`         | `kdf_val_as_uint`         | 64-bit unsigned       |
| `KDF_TYPE_FLOAT`        | `kdf_val_as_float`        | 64-bit IEEE 754       |
| `KDF_TYPE_STRING`       | `kdf_val_as_string`       | UTF-8, interned       |
| `KDF_TYPE_VEC2`         | `kdf_val_as_vec2`         | float[2]              |
| `KDF_TYPE_VEC3`         | `kdf_val_as_vec3`         | float[3]              |
| `KDF_TYPE_VEC4`         | `kdf_val_as_vec4`         | float[4]              |
| `KDF_TYPE_QUAT`         | `kdf_val_as_quat`         | float[4]              |
| `KDF_TYPE_COLOR`        | `kdf_val_as_color`        | RGBA, float[4]        |
| `KDF_TYPE_ARRAY`        | `kdf_val_as_array`        | Ordered list          |
| `KDF_TYPE_OBJECT`       | `kdf_val_as_object`       | Named properties      |
| `KDF_TYPE_ASSET_REF`    | `kdf_val_as_asset_ref`    | String reference      |
| `KDF_TYPE_RESOURCE_REF` | `kdf_val_as_resource_ref` | String reference      |
| `KDF_TYPE_SUBRESOURCE`  | `kdf_val_as_object`       | Object with type name |

## Text Format Syntax

```
kdf 1

key = value
key = "string"
key = 42
key = 42u
key = 3.14
key = true
key = null
key = vec2(10, 20)
key = vec3(1, 2, 3)
key = vec4(1, 1, 1, 1)
key = quat(0, 0, 0, 1)
key = color(1, 0.5, 0.2, 1)
key = asset("res://path")
key = external("res://path")
key = [1, 2, 3]
key {
    nested = "object"
}
key = subresource Type {
    typed = "object"
}
```

Comments start with `#` and go to the end of the line.

## Error Handling

All functions that can fail return `kdf_error` codes or `NULL`:

- `kdf_doc_create` / `kdf_doc_create_alloc` return `NULL` on allocation failure.
- `kdf_text_read` / `kdf_binary_read` return `NULL` on parse or I/O error.
- `kdf_text_write` / `kdf_binary_write` return `KDF_OK` or a negative `kdf_error`.
- `kdf_text_save` / `kdf_binary_save` return `KDF_OK` or a negative `kdf_error`.

Error codes are defined in `kdf/kdf_types.h`:

| Code                            | Value | Meaning                       |
| ------------------------------- | ----- | ----------------------------- |
| `KDF_OK`                        | 0     | Success                       |
| `KDF_ERROR_INVALID_ARGUMENT`    | -1    | NULL pointer or invalid input |
| `KDF_ERROR_OUT_OF_MEMORY`       | -2    | Allocation failed             |
| `KDF_ERROR_PARSE`               | -3    | Text format syntax error      |
| `KDF_ERROR_IO`                  | -4    | Read or write failed          |
| `KDF_ERROR_INVALID_TYPE`        | -5    | Type mismatch                 |
| `KDF_ERROR_NOT_FOUND`           | -6    | Key not found                 |
| `KDF_ERROR_INVALID_FORMAT`      | -7    | Binary format error           |
| `KDF_ERROR_UNSUPPORTED_VERSION` | -8    | Unknown format version        |
| `KDF_ERROR_BUFFER_TOO_SMALL`    | -9    | Buffer too small              |
