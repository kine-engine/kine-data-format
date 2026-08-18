#include "kdf_internal.h"

// array init/destroy

void kdf__array_init(kdf_array *arr, kdf_allocator *alloc, kdf_string_pool *strings)
{
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
    arr->alloc = alloc;
    arr->strings = strings;
}

void kdf__array_destroy(kdf_array *arr)
{
    if (!arr)
        return;
    for (size_t i = 0; i < arr->count; i++)
    {
        kdf__value_destroy(&arr->items[i], arr->alloc);
    }
    if (arr->items)
    {
        kdf__free(arr->alloc, arr->items, sizeof(kdf_value) * arr->capacity);
    }
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void kdf__array_grow(kdf_array *arr)
{
    size_t new_cap = arr->capacity == 0 ? KDF_ARRAY_INIT_CAP : arr->capacity * 2;
    kdf_value *new_items = (kdf_value *)kdf__realloc(arr->alloc, arr->items, sizeof(kdf_value) * arr->capacity,
                                                     sizeof(kdf_value) * new_cap);
    if (!new_items)
        return;
    arr->items = new_items;
    arr->capacity = new_cap;
}

// push operations

static kdf_value *kdf__arr_push(kdf_array *arr, kdf_type type)
{
    if (!arr)
        return NULL;
    if (arr->count >= arr->capacity)
    {
        kdf__array_grow(arr);
    }
    kdf_value *v = &arr->items[arr->count++];
    v->type = type;
    memset(&v->as, 0, sizeof(v->as));
    return v;
}

void kdf_arr_push_null(kdf_array *arr)
{
    kdf__arr_push(arr, KDF_TYPE_NULL);
}

void kdf_arr_push_bool(kdf_array *arr, bool value)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_BOOL);
    if (v)
        v->as.boolean = value;
}

void kdf_arr_push_int(kdf_array *arr, int64_t value)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_INT);
    if (v)
        v->as.i64 = value;
}

void kdf_arr_push_uint(kdf_array *arr, uint64_t value)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_UINT);
    if (v)
        v->as.u64 = value;
}

void kdf_arr_push_float(kdf_array *arr, double value)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_FLOAT);
    if (v)
        v->as.f64 = value;
}

void kdf_arr_push_string(kdf_array *arr, const char *value)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_STRING);
    if (v && value)
    {
        if (arr->strings)
        {
            v->as.str = kdf__intern(arr->strings, value);
        }
        else
        {
            // fallback: allocate a minimal kdf_string
            kdf_string *s = (kdf_string *)kdf__alloc(arr->alloc, sizeof(kdf_string));
            if (s)
            {
                size_t len = strlen(value);
                s->data = (char *)kdf__alloc(arr->alloc, len + 1);
                if (s->data)
                {
                    memcpy(s->data, value, len + 1);
                }
                s->length = len;
                s->hash = kdf__fnv1a(value, len);
                s->next = NULL;
                v->as.str = s;
            }
        }
    }
}

void kdf_arr_push_vec2(kdf_array *arr, float x, float y)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_VEC2);
    if (v)
    {
        v->as.vec2[0] = x;
        v->as.vec2[1] = y;
    }
}

void kdf_arr_push_vec3(kdf_array *arr, float x, float y, float z)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_VEC3);
    if (v)
    {
        v->as.vec3[0] = x;
        v->as.vec3[1] = y;
        v->as.vec3[2] = z;
    }
}

void kdf_arr_push_vec4(kdf_array *arr, float x, float y, float z, float w)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_VEC4);
    if (v)
    {
        v->as.vec4[0] = x;
        v->as.vec4[1] = y;
        v->as.vec4[2] = z;
        v->as.vec4[3] = w;
    }
}

void kdf_arr_push_quat(kdf_array *arr, float x, float y, float z, float w)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_QUAT);
    if (v)
    {
        v->as.vec4[0] = x;
        v->as.vec4[1] = y;
        v->as.vec4[2] = z;
        v->as.vec4[3] = w;
    }
}

void kdf_arr_push_color(kdf_array *arr, float r, float g, float b, float a)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_COLOR);
    if (v)
    {
        v->as.vec4[0] = r;
        v->as.vec4[1] = g;
        v->as.vec4[2] = b;
        v->as.vec4[3] = a;
    }
}

void kdf_arr_push_asset_ref(kdf_array *arr, const char *path)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_ASSET_REF);
    if (v && path)
    {
        if (arr->strings)
        {
            v->as.str = kdf__intern(arr->strings, path);
        }
        else
        {
            kdf_string *s = (kdf_string *)kdf__alloc(arr->alloc, sizeof(kdf_string));
            if (s)
            {
                size_t len = strlen(path);
                s->data = (char *)kdf__alloc(arr->alloc, len + 1);
                if (s->data)
                {
                    memcpy(s->data, path, len + 1);
                }
                s->length = len;
                s->hash = kdf__fnv1a(path, len);
                s->next = NULL;
                v->as.str = s;
            }
        }
    }
}

kdf_object *kdf_arr_push_object(kdf_array *arr)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_OBJECT);
    if (!v)
        return NULL;
    v->as.object = (kdf_object *)kdf__alloc(arr->alloc, sizeof(kdf_object));
    if (!v->as.object)
        return NULL;
    kdf__object_init(v->as.object, arr->alloc, arr->strings);
    return v->as.object;
}

kdf_array *kdf_arr_push_array(kdf_array *arr)
{
    kdf_value *v = kdf__arr_push(arr, KDF_TYPE_ARRAY);
    if (!v)
        return NULL;
    v->as.array = (kdf_array *)kdf__alloc(arr->alloc, sizeof(kdf_array));
    if (!v->as.array)
        return NULL;
    kdf__array_init(v->as.array, arr->alloc, arr->strings);
    return v->as.array;
}

// access

size_t kdf_arr_count(const kdf_array *arr)
{
    return arr ? arr->count : 0;
}

kdf_value *kdf_arr_get(kdf_array *arr, size_t index)
{
    if (!arr || index >= arr->count)
        return NULL;
    return &arr->items[index];
}

const kdf_value *kdf_arr_get_const(const kdf_array *arr, size_t index)
{
    if (!arr || index >= arr->count)
        return NULL;
    return &arr->items[index];
}

kdf_type kdf_arr_get_type(const kdf_array *arr, size_t index)
{
    const kdf_value *v = kdf_arr_get_const(arr, index);
    return v ? v->type : KDF_TYPE_NULL;
}

// modification

void kdf_arr_clear(kdf_array *arr)
{
    if (!arr)
        return;
    for (size_t i = 0; i < arr->count; i++)
    {
        kdf__value_destroy(&arr->items[i], arr->alloc);
    }
    arr->count = 0;
}

void kdf_arr_remove(kdf_array *arr, size_t index)
{
    if (!arr || index >= arr->count)
        return;
    kdf__value_destroy(&arr->items[index], arr->alloc);
    for (size_t i = index; i < arr->count - 1; i++)
    {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(kdf_value));
}

// internal: push a value by ownership transfer

void kdf__arr_push_value(kdf_array *arr, const kdf_value *val)
{
    if (!arr || !val)
        return;
    if (arr->count >= arr->capacity)
    {
        kdf__array_grow(arr);
    }
    arr->items[arr->count++] = *val;
}
