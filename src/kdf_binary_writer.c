#include "kdf_internal.h"

// binary writer state

typedef struct
{
    kdf_writer writer;
    int error;
} bw;

static void bw_write(bw *w, const void *data, size_t size)
{
    if (w->error)
        return;
    if (w->writer.write(w->writer.userdata, data, size) != size)
    {
        w->error = KDF_ERROR_IO;
    }
}

static void bw_u8(bw *w, uint8_t v)
{
    bw_write(w, &v, 1);
}

static void bw_u16(bw *w, uint16_t v)
{
    uint8_t b[2] = {(uint8_t)(v & 0xFF), (uint8_t)(v >> 8)};
    bw_write(w, b, 2);
}

static void bw_u32(bw *w, uint32_t v)
{
    uint8_t b[4] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF), (uint8_t)((v >> 16) & 0xFF),
                    (uint8_t)((v >> 24) & 0xFF)};
    bw_write(w, b, 4);
}

static void bw_i32(bw *w, int32_t v)
{
    uint8_t b[4] = {(uint8_t)((uint32_t)v & 0xFF), (uint8_t)(((uint32_t)v >> 8) & 0xFF),
                    (uint8_t)(((uint32_t)v >> 16) & 0xFF), (uint8_t)(((uint32_t)v >> 24) & 0xFF)};
    bw_write(w, b, 4);
}

static void bw_i64(bw *w, int64_t v)
{
    uint8_t b[8];
    for (int i = 0; i < 8; i++)
        b[i] = (uint8_t)((uint64_t)v >> (i * 8));
    bw_write(w, b, 8);
}

static void bw_u64(bw *w, uint64_t v)
{
    uint8_t b[8];
    for (int i = 0; i < 8; i++)
        b[i] = (uint8_t)(v >> (i * 8));
    bw_write(w, b, 8);
}

static void bw_f64(bw *w, double v)
{
    uint64_t bits;
    memcpy(&bits, &v, 8);
    bw_u64(w, bits);
}

static void bw_f32(bw *w, float v)
{
    uint32_t bits;
    memcpy(&bits, &v, 4);
    bw_u32(w, bits);
}

// string table collection

typedef struct
{
    kdf_string **strings;
    size_t count;
    size_t capacity;
} str_table;

static void str_table_init(str_table *t)
{
    t->strings = NULL;
    t->count = 0;
    t->capacity = 0;
}

static void str_table_free(str_table *t, kdf_allocator *alloc)
{
    if (t->strings)
    {
        kdf__free(alloc, t->strings, sizeof(kdf_string *) * t->capacity);
    }
}

static uint32_t str_table_add(str_table *t, kdf_string *s, kdf_allocator *alloc)
{
    if (!s)
        return UINT32_MAX;

    for (size_t i = 0; i < t->count; i++)
    {
        if (t->strings[i] == s)
            return (uint32_t)i;
    }

    if (t->count >= t->capacity)
    {
        size_t new_cap = t->capacity == 0 ? 64 : t->capacity * 2;
        kdf_string **new_data = (kdf_string **)kdf__realloc(alloc, t->strings, sizeof(kdf_string *) * t->capacity,
                                                            sizeof(kdf_string *) * new_cap);
        if (!new_data)
            return UINT32_MAX;
        t->strings = new_data;
        t->capacity = new_cap;
    }
    t->strings[t->count] = s;
    return (uint32_t)(t->count++);
}

static uint32_t str_index(const str_table *st, kdf_string *s)
{
    if (!s)
        return UINT32_MAX;
    for (size_t i = 0; i < st->count; i++)
    {
        if (st->strings[i] == s)
            return (uint32_t)i;
    }
    return UINT32_MAX;
}

static void collect_strings_value(str_table *t, const kdf_value *val, kdf_allocator *alloc);
static void collect_strings_object(str_table *t, const kdf_object *obj, kdf_allocator *alloc);

static void collect_strings_value(str_table *t, const kdf_value *val, kdf_allocator *alloc)
{
    if (!val)
        return;
    switch (val->type)
    {
    case KDF_TYPE_STRING:
    case KDF_TYPE_ASSET_REF:
    case KDF_TYPE_RESOURCE_REF:
        str_table_add(t, val->as.str, alloc);
        break;
    case KDF_TYPE_ARRAY:
        if (val->as.array)
        {
            for (size_t i = 0; i < val->as.array->count; i++)
            {
                collect_strings_value(t, &val->as.array->items[i], alloc);
            }
        }
        break;
    case KDF_TYPE_OBJECT:
    case KDF_TYPE_SUBRESOURCE:
        if (val->as.object)
        {
            collect_strings_object(t, val->as.object, alloc);
        }
        break;
    default:
        break;
    }
}

static void collect_strings_object(str_table *t, const kdf_object *obj, kdf_allocator *alloc)
{
    if (!obj)
        return;
    str_table_add(t, obj->type_name, alloc);
    for (const kdf_entry *e = kdf_obj_first(obj); e; e = kdf_obj_next(e))
    {
        str_table_add(t, e->key, alloc);
        collect_strings_value(t, &e->value, alloc);
    }
}

// value/object writing

static void bw_write_value(bw *w, const kdf_value *val, const str_table *st);
static void bw_write_object(bw *w, const kdf_object *obj, const str_table *st);

static void bw_write_value(bw *w, const kdf_value *val, const str_table *st)
{
    if (!val)
    {
        bw_u8(w, KDF_BIN_NULL);
        return;
    }

    bw_u8(w, (uint8_t)val->type);

    switch (val->type)
    {
    case KDF_TYPE_NULL:
        break;
    case KDF_TYPE_BOOL:
        bw_u8(w, val->as.boolean ? 1 : 0);
        break;
    case KDF_TYPE_INT:
        bw_i64(w, val->as.i64);
        break;
    case KDF_TYPE_UINT:
        bw_u64(w, val->as.u64);
        break;
    case KDF_TYPE_FLOAT:
        bw_f64(w, val->as.f64);
        break;
    case KDF_TYPE_STRING:
    case KDF_TYPE_ASSET_REF:
    case KDF_TYPE_RESOURCE_REF:
        bw_u32(w, str_index(st, val->as.str));
        break;
    case KDF_TYPE_VEC2:
        bw_f32(w, val->as.vec2[0]);
        bw_f32(w, val->as.vec2[1]);
        break;
    case KDF_TYPE_VEC3:
        bw_f32(w, val->as.vec3[0]);
        bw_f32(w, val->as.vec3[1]);
        bw_f32(w, val->as.vec3[2]);
        break;
    case KDF_TYPE_VEC4:
    case KDF_TYPE_QUAT:
    case KDF_TYPE_COLOR:
        bw_f32(w, val->as.vec4[0]);
        bw_f32(w, val->as.vec4[1]);
        bw_f32(w, val->as.vec4[2]);
        bw_f32(w, val->as.vec4[3]);
        break;
    case KDF_TYPE_ARRAY:
    {
        kdf_array *arr = val->as.array;
        uint32_t count = arr ? (uint32_t)arr->count : 0;
        bw_u32(w, count);
        for (uint32_t i = 0; i < count; i++)
        {
            bw_write_value(w, &arr->items[i], st);
        }
        break;
    }
    case KDF_TYPE_OBJECT:
    case KDF_TYPE_SUBRESOURCE:
        bw_write_object(w, val->as.object, st);
        break;
    }
}

static void bw_write_object(bw *w, const kdf_object *obj, const str_table *st)
{
    if (!obj)
    {
        bw_i32(w, -1);
        bw_i32(w, 0);
        bw_u32(w, 0);
        return;
    }

    int32_t type_idx = obj->type_name ? (int32_t)str_index(st, obj->type_name) : -1;
    bw_i32(w, type_idx);
    bw_i32(w, (int32_t)obj->version);
    bw_u32(w, (uint32_t)obj->count);

    for (const kdf_entry *e = kdf_obj_first(obj); e; e = kdf_obj_next(e))
    {
        bw_u32(w, str_index(st, e->key));
        bw_write_value(w, &e->value, st);
    }
}

// public api

int kdf_binary_write(const kdf_document *doc, kdf_writer writer)
{
    if (!doc)
        return KDF_ERROR_INVALID_ARGUMENT;

    bw w;
    w.writer = writer;
    w.error = 0;

    str_table st;
    str_table_init(&st);
    collect_strings_object(&st, &doc->root, (kdf_allocator *)&doc->alloc);

    // header: magic "kdfb"
    bw_u8(&w, 'K');
    bw_u8(&w, 'D');
    bw_u8(&w, 'F');
    bw_u8(&w, 'B');
    bw_u16(&w, 1); // version
    bw_u16(&w, 0); // flags

    // string table
    bw_u32(&w, (uint32_t)st.count);
    for (size_t i = 0; i < st.count; i++)
    {
        kdf_string *s = st.strings[i];
        bw_u32(&w, (uint32_t)s->length);
        bw_write(&w, s->data, s->length);
    }

    // root object
    bw_write_object(&w, &doc->root, &st);

    str_table_free(&st, (kdf_allocator *)&doc->alloc);
    return w.error;
}

int kdf_binary_save(const kdf_document *doc, const char *path)
{
    if (!doc || !path)
        return KDF_ERROR_INVALID_ARGUMENT;

    FILE *f = fopen(path, "wb");
    if (!f)
        return KDF_ERROR_IO;

    kdf_mem_writer mw = {0};
    kdf_writer w = kdf_mem_writer_create(&mw);

    int result = kdf_binary_write(doc, w);

    if (result == KDF_OK && mw.size > 0)
    {
        if (fwrite(mw.data, 1, mw.size, f) != mw.size)
        {
            result = KDF_ERROR_IO;
        }
    }

    kdf_mem_writer_finish(&mw);
    fclose(f);
    return result;
}
