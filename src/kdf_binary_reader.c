#include "kdf_internal.h"

// binary reader state

typedef struct
{
    kdf_reader reader;
    int error;
    kdf_document *doc;
    kdf_allocator *alloc;
    kdf_string_pool *strings;

    // string table
    char **str_table;
    uint32_t str_count;
} br;

static int br_read(br *r, void *dst, size_t size)
{
    if (r->error)
        return r->error;
    size_t got = r->reader.read(r->reader.userdata, dst, size);
    if (got != size)
    {
        r->error = KDF_ERROR_IO;
        return KDF_ERROR_IO;
    }
    return KDF_OK;
}

static uint8_t br_u8(br *r)
{
    uint8_t v = 0;
    br_read(r, &v, 1);
    return v;
}

static uint16_t br_u16(br *r)
{
    uint8_t b[2];
    if (br_read(r, b, 2) != KDF_OK)
        return 0;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

static uint32_t br_u32(br *r)
{
    uint8_t b[4];
    if (br_read(r, b, 4) != KDF_OK)
        return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int32_t br_i32(br *r)
{
    return (int32_t)br_u32(r);
}

static int64_t br_i64(br *r)
{
    uint8_t b[8];
    if (br_read(r, b, 8) != KDF_OK)
        return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)b[i] << (i * 8);
    return (int64_t)v;
}

static uint64_t br_u64(br *r)
{
    uint8_t b[8];
    if (br_read(r, b, 8) != KDF_OK)
        return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)b[i] << (i * 8);
    return v;
}

static double br_f64(br *r)
{
    uint64_t bits = br_u64(r);
    double v;
    memcpy(&v, &bits, 8);
    return v;
}

static float br_f32(br *r)
{
    uint32_t bits = br_u32(r);
    float v;
    memcpy(&v, &bits, 4);
    return v;
}

// string table reading

static int br_read_string_table(br *r)
{
    r->str_count = br_u32(r);
    if (r->error)
        return r->error;

    if (r->str_count > 0)
    {
        r->str_table = (char **)kdf__alloc(r->alloc, sizeof(char *) * r->str_count);
        if (!r->str_table)
        {
            r->error = KDF_ERROR_OUT_OF_MEMORY;
            return r->error;
        }

        for (uint32_t i = 0; i < r->str_count; i++)
        {
            uint32_t len = br_u32(r);
            if (r->error)
                return r->error;

            char *s = (char *)kdf__alloc(r->alloc, len + 1);
            if (!s)
            {
                r->error = KDF_ERROR_OUT_OF_MEMORY;
                return r->error;
            }

            if (len > 0)
            {
                br_read(r, s, len);
                if (r->error)
                {
                    kdf__free(r->alloc, s, len + 1);
                    return r->error;
                }
            }
            s[len] = '\0';
            r->str_table[i] = s;
        }
    }

    return KDF_OK;
}

static void br_free_string_table(br *r)
{
    if (r->str_table)
    {
        for (uint32_t i = 0; i < r->str_count; i++)
        {
            if (r->str_table[i])
            {
                kdf__free(r->alloc, r->str_table[i], strlen(r->str_table[i]) + 1);
            }
        }
        kdf__free(r->alloc, r->str_table, sizeof(char *) * r->str_count);
        r->str_table = NULL;
    }
}

static const char *br_get_string(br *r, uint32_t index)
{
    if (index == UINT32_MAX || index >= r->str_count)
        return "";
    return r->str_table[index] ? r->str_table[index] : "";
}

// value/object reading

static int br_read_value(br *r, kdf_value *out);
static int br_read_object(br *r, kdf_object *obj);

static int br_read_value(br *r, kdf_value *out)
{
    uint8_t type = br_u8(r);
    if (r->error)
        return r->error;

    out->type = (kdf_type)type;

    switch (out->type)
    {
    case KDF_TYPE_NULL:
        break;
    case KDF_TYPE_BOOL:
        out->as.boolean = br_u8(r) != 0;
        break;
    case KDF_TYPE_INT:
        out->as.i64 = br_i64(r);
        break;
    case KDF_TYPE_UINT:
        out->as.u64 = br_u64(r);
        break;
    case KDF_TYPE_FLOAT:
        out->as.f64 = br_f64(r);
        break;
    case KDF_TYPE_STRING:
    case KDF_TYPE_ASSET_REF:
    case KDF_TYPE_RESOURCE_REF:
    {
        uint32_t idx = br_u32(r);
        const char *s = br_get_string(r, idx);
        out->as.str = kdf__intern(r->strings, s);
        break;
    }
    case KDF_TYPE_VEC2:
        out->as.vec2[0] = br_f32(r);
        out->as.vec2[1] = br_f32(r);
        break;
    case KDF_TYPE_VEC3:
        out->as.vec3[0] = br_f32(r);
        out->as.vec3[1] = br_f32(r);
        out->as.vec3[2] = br_f32(r);
        break;
    case KDF_TYPE_VEC4:
    case KDF_TYPE_QUAT:
    case KDF_TYPE_COLOR:
        out->as.vec4[0] = br_f32(r);
        out->as.vec4[1] = br_f32(r);
        out->as.vec4[2] = br_f32(r);
        out->as.vec4[3] = br_f32(r);
        break;
    case KDF_TYPE_ARRAY:
    {
        uint32_t count = br_u32(r);
        if (r->error)
            return r->error;

        out->as.array = (kdf_array *)kdf__alloc(r->alloc, sizeof(kdf_array));
        if (!out->as.array)
        {
            r->error = KDF_ERROR_OUT_OF_MEMORY;
            return r->error;
        }
        kdf__array_init(out->as.array, r->alloc, r->strings);

        for (uint32_t i = 0; i < count; i++)
        {
            kdf_value item;
            memset(&item, 0, sizeof(item));
            if (br_read_value(r, &item) != KDF_OK)
                return r->error;

            switch (item.type)
            {
            case KDF_TYPE_NULL:
                kdf_arr_push_null(out->as.array);
                break;
            case KDF_TYPE_BOOL:
                kdf_arr_push_bool(out->as.array, item.as.boolean);
                break;
            case KDF_TYPE_INT:
                kdf_arr_push_int(out->as.array, item.as.i64);
                break;
            case KDF_TYPE_UINT:
                kdf_arr_push_uint(out->as.array, item.as.u64);
                break;
            case KDF_TYPE_FLOAT:
                kdf_arr_push_float(out->as.array, item.as.f64);
                break;
            case KDF_TYPE_STRING:
                kdf_arr_push_string(out->as.array, item.as.str ? item.as.str->data : "");
                break;
            case KDF_TYPE_VEC2:
                kdf_arr_push_vec2(out->as.array, item.as.vec2[0], item.as.vec2[1]);
                break;
            case KDF_TYPE_VEC3:
                kdf_arr_push_vec3(out->as.array, item.as.vec3[0], item.as.vec3[1], item.as.vec3[2]);
                break;
            case KDF_TYPE_VEC4:
                kdf_arr_push_vec4(out->as.array, item.as.vec4[0], item.as.vec4[1], item.as.vec4[2], item.as.vec4[3]);
                break;
            case KDF_TYPE_QUAT:
                kdf_arr_push_quat(out->as.array, item.as.vec4[0], item.as.vec4[1], item.as.vec4[2], item.as.vec4[3]);
                break;
            case KDF_TYPE_COLOR:
                kdf_arr_push_color(out->as.array, item.as.vec4[0], item.as.vec4[1], item.as.vec4[2], item.as.vec4[3]);
                break;
            case KDF_TYPE_ASSET_REF:
                kdf_arr_push_asset_ref(out->as.array, item.as.str ? item.as.str->data : "");
                break;
            default:
                // objects, arrays, subresources: transfer ownership
                kdf__arr_push_value(out->as.array, &item);
                break;
            }
        }
        break;
    }
    case KDF_TYPE_OBJECT:
    case KDF_TYPE_SUBRESOURCE:
    {
        out->as.object = (kdf_object *)kdf__alloc(r->alloc, sizeof(kdf_object));
        if (!out->as.object)
        {
            r->error = KDF_ERROR_OUT_OF_MEMORY;
            return r->error;
        }
        kdf__object_init(out->as.object, r->alloc, r->strings);
        if (br_read_object(r, out->as.object) != KDF_OK)
            return r->error;
        break;
    }
    default:
        r->error = KDF_ERROR_INVALID_FORMAT;
        return r->error;
    }

    return r->error;
}

static int br_read_object(br *r, kdf_object *obj)
{
    int32_t type_idx = br_i32(r);
    int32_t version = br_i32(r);
    uint32_t prop_count = br_u32(r);
    if (r->error)
        return r->error;

    if (type_idx >= 0)
    {
        obj->type_name = kdf__intern(r->strings, br_get_string(r, (uint32_t)type_idx));
    }
    obj->version = (int)version;

    for (uint32_t i = 0; i < prop_count; i++)
    {
        uint32_t key_idx = br_u32(r);
        if (r->error)
            return r->error;

        const char *key = br_get_string(r, key_idx);

        kdf_value val;
        memset(&val, 0, sizeof(val));
        if (br_read_value(r, &val) != KDF_OK)
            return r->error;

        // assign to object
        switch (val.type)
        {
        case KDF_TYPE_NULL:
            kdf_obj_set_null(obj, key);
            break;
        case KDF_TYPE_BOOL:
            kdf_obj_set_bool(obj, key, val.as.boolean);
            break;
        case KDF_TYPE_INT:
            kdf_obj_set_int(obj, key, val.as.i64);
            break;
        case KDF_TYPE_UINT:
            kdf_obj_set_uint(obj, key, val.as.u64);
            break;
        case KDF_TYPE_FLOAT:
            kdf_obj_set_float(obj, key, val.as.f64);
            break;
        case KDF_TYPE_STRING:
            kdf_obj_set_string(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_VEC2:
            kdf_obj_set_vec2(obj, key, val.as.vec2[0], val.as.vec2[1]);
            break;
        case KDF_TYPE_VEC3:
            kdf_obj_set_vec3(obj, key, val.as.vec3[0], val.as.vec3[1], val.as.vec3[2]);
            break;
        case KDF_TYPE_VEC4:
            kdf_obj_set_vec4(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_QUAT:
            kdf_obj_set_quat(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_COLOR:
            kdf_obj_set_color(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_ASSET_REF:
            kdf_obj_set_asset_ref(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_RESOURCE_REF:
            kdf_obj_set_resource_ref(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_ARRAY:
        {
            // transfer the temporary array directly to the object
            kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_ARRAY);
            if (e)
            {
                e->value.as.array = val.as.array;
                val.as.array = NULL; // prevent double-free
            }
            break;
        }
        case KDF_TYPE_OBJECT:
        case KDF_TYPE_SUBRESOURCE:
        {
            // transfer the temporary object directly to the entry
            kdf_entry *e = kdf__object_insert(obj, key, val.type);
            if (e && val.as.object)
            {
                e->value.as.object = val.as.object;
                val.as.object = NULL; // prevent double-free
            }
            break;
        }
        default:
            break;
        }
    }

    return KDF_OK;
}

// public api

kdf_document *kdf_binary_read(kdf_reader reader, const kdf_allocator *alloc)
{
    kdf_allocator a = kdf__alloc_or_default(alloc);
    kdf_document *doc = kdf_doc_create_alloc(&a);
    if (!doc)
        return NULL;

    br r;
    memset(&r, 0, sizeof(r));
    r.reader = reader;
    r.doc = doc;
    r.alloc = &doc->alloc;
    r.strings = &doc->strings;

    // read and verify magic
    uint8_t magic[4];
    if (br_read(&r, magic, 4) != KDF_OK || memcmp(magic, "KDFB", 4) != 0)
    {
        kdf_doc_destroy(doc);
        return NULL;
    }

    // read version and flags
    uint16_t version = br_u16(&r);
    uint16_t flags = br_u16(&r);
    (void)version;
    (void)flags;

    // read string table
    if (br_read_string_table(&r) != KDF_OK)
    {
        br_free_string_table(&r);
        kdf_doc_destroy(doc);
        return NULL;
    }

    // read root object
    if (br_read_object(&r, &doc->root) != KDF_OK)
    {
        br_free_string_table(&r);
        kdf_doc_destroy(doc);
        return NULL;
    }

    br_free_string_table(&r);
    return doc;
}

kdf_document *kdf_binary_load(const char *path, const kdf_allocator *alloc)
{
    if (!path)
        return NULL;

    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0)
    {
        fclose(f);
        return NULL;
    }

    kdf_allocator a = kdf__alloc_or_default(alloc);
    uint8_t *data = (uint8_t *)a.alloc(a.userdata, (size_t)file_size);
    if (!data)
    {
        fclose(f);
        return NULL;
    }

    size_t rd = fread(data, 1, (size_t)file_size, f);
    fclose(f);

    if (rd != (size_t)file_size)
    {
        a.free(a.userdata, data, (size_t)file_size);
        return NULL;
    }

    kdf_mem_reader mr;
    mr.data = data;
    mr.size = (size_t)file_size;
    mr.pos = 0;

    kdf_reader reader = kdf_mem_reader_create(&mr);
    kdf_document *doc = kdf_binary_read(reader, &a);

    a.free(a.userdata, data, (size_t)file_size);
    return doc;
}
