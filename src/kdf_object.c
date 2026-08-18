#include "kdf_internal.h"

// object init/destroy

void kdf__object_init(kdf_object *obj, kdf_allocator *alloc, kdf_string_pool *strings)
{
    obj->type_name = NULL;
    obj->version = 0;
    obj->buckets = NULL;
    obj->bucket_count = 0;
    obj->count = 0;
    obj->first = NULL;
    obj->last = NULL;
    obj->alloc = alloc;
    obj->strings = strings;
}

void kdf__object_destroy(kdf_object *obj)
{
    kdf_entry *e = obj->first;
    while (e)
    {
        kdf_entry *next = e->order_next;
        kdf__value_destroy(&e->value, obj->alloc);
        kdf__free(obj->alloc, e, sizeof(kdf_entry));
        e = next;
    }
    if (obj->buckets)
    {
        kdf__free(obj->alloc, obj->buckets, sizeof(kdf_entry *) * obj->bucket_count);
    }
    obj->buckets = NULL;
    obj->bucket_count = 0;
    obj->count = 0;
    obj->first = NULL;
    obj->last = NULL;
}

// hash table operations

void kdf__object_grow(kdf_object *obj)
{
    size_t new_count = obj->bucket_count == 0 ? KDF_OBJECT_INIT_BUCKETS : obj->bucket_count * 2;
    kdf_entry **new_buckets = (kdf_entry **)kdf__alloc(obj->alloc, sizeof(kdf_entry *) * new_count);
    if (!new_buckets)
        return;
    memset(new_buckets, 0, sizeof(kdf_entry *) * new_count);

    for (kdf_entry *e = obj->first; e; e = e->order_next)
    {
        size_t idx = e->key->hash % new_count;
        e->hash_next = new_buckets[idx];
        new_buckets[idx] = e;
    }

    if (obj->buckets)
    {
        kdf__free(obj->alloc, obj->buckets, sizeof(kdf_entry *) * obj->bucket_count);
    }
    obj->buckets = new_buckets;
    obj->bucket_count = new_count;
}

kdf_entry *kdf__object_find(const kdf_object *obj, const char *key, size_t key_len, size_t hash)
{
    if (!obj->buckets)
        return NULL;
    size_t idx = hash % obj->bucket_count;
    for (kdf_entry *e = obj->buckets[idx]; e; e = e->hash_next)
    {
        if (e->key->hash == hash && e->key->length == key_len && memcmp(e->key->data, key, key_len) == 0)
        {
            return e;
        }
    }
    return NULL;
}

kdf_entry *kdf__object_insert(kdf_object *obj, const char *key, kdf_type type)
{
    size_t key_len = strlen(key);
    size_t hash = kdf__fnv1a(key, key_len);

    // check if key already exists
    kdf_entry *existing = kdf__object_find(obj, key, key_len, hash);
    if (existing)
    {
        kdf__value_destroy(&existing->value, obj->alloc);
        existing->value.type = type;
        memset(&existing->value.as, 0, sizeof(existing->value.as));
        return existing;
    }

    // grow if needed
    if (obj->count >= obj->bucket_count * 3 / 4 || obj->buckets == NULL)
    {
        kdf__object_grow(obj);
    }

    // create new entry
    kdf_entry *e = (kdf_entry *)kdf__alloc(obj->alloc, sizeof(kdf_entry));
    if (!e)
        return NULL;
    memset(e, 0, sizeof(kdf_entry));

    e->key = kdf__intern(obj->strings, key);
    e->value.type = type;

    // insert into hash chain
    size_t idx = hash % obj->bucket_count;
    e->hash_next = obj->buckets[idx];
    obj->buckets[idx] = e;

    // insert into order list
    e->order_prev = obj->last;
    e->order_next = NULL;
    if (obj->last)
    {
        obj->last->order_next = e;
    }
    else
    {
        obj->first = e;
    }
    obj->last = e;
    obj->count++;

    return e;
}

// object metadata

void kdf_obj_set_type(kdf_object *obj, const char *type)
{
    if (!obj)
        return;
    obj->type_name = type ? kdf__intern(obj->strings, type) : NULL;
}

void kdf_obj_set_version(kdf_object *obj, int version)
{
    if (obj)
        obj->version = version;
}

const char *kdf_obj_type(const kdf_object *obj)
{
    if (!obj || !obj->type_name)
        return NULL;
    return obj->type_name->data;
}

int kdf_obj_version(const kdf_object *obj)
{
    return obj ? obj->version : 0;
}

// setters

void kdf_obj_set_null(kdf_object *obj, const char *key)
{
    if (!obj || !key)
        return;
    kdf__object_insert(obj, key, KDF_TYPE_NULL);
}

void kdf_obj_set_bool(kdf_object *obj, const char *key, bool value)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_BOOL);
    if (e)
        e->value.as.boolean = value;
}

void kdf_obj_set_int(kdf_object *obj, const char *key, int64_t value)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_INT);
    if (e)
        e->value.as.i64 = value;
}

void kdf_obj_set_uint(kdf_object *obj, const char *key, uint64_t value)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_UINT);
    if (e)
        e->value.as.u64 = value;
}

void kdf_obj_set_float(kdf_object *obj, const char *key, double value)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_FLOAT);
    if (e)
        e->value.as.f64 = value;
}

void kdf_obj_set_string(kdf_object *obj, const char *key, const char *value)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_STRING);
    if (e && value)
        e->value.as.str = kdf__intern(obj->strings, value);
}

void kdf_obj_set_vec2(kdf_object *obj, const char *key, float x, float y)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_VEC2);
    if (e)
    {
        e->value.as.vec2[0] = x;
        e->value.as.vec2[1] = y;
    }
}

void kdf_obj_set_vec3(kdf_object *obj, const char *key, float x, float y, float z)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_VEC3);
    if (e)
    {
        e->value.as.vec3[0] = x;
        e->value.as.vec3[1] = y;
        e->value.as.vec3[2] = z;
    }
}

void kdf_obj_set_vec4(kdf_object *obj, const char *key, float x, float y, float z, float w)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_VEC4);
    if (e)
    {
        e->value.as.vec4[0] = x;
        e->value.as.vec4[1] = y;
        e->value.as.vec4[2] = z;
        e->value.as.vec4[3] = w;
    }
}

void kdf_obj_set_quat(kdf_object *obj, const char *key, float x, float y, float z, float w)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_QUAT);
    if (e)
    {
        e->value.as.vec4[0] = x;
        e->value.as.vec4[1] = y;
        e->value.as.vec4[2] = z;
        e->value.as.vec4[3] = w;
    }
}

void kdf_obj_set_color(kdf_object *obj, const char *key, float r, float g, float b, float a)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_COLOR);
    if (e)
    {
        e->value.as.vec4[0] = r;
        e->value.as.vec4[1] = g;
        e->value.as.vec4[2] = b;
        e->value.as.vec4[3] = a;
    }
}

void kdf_obj_set_asset_ref(kdf_object *obj, const char *key, const char *path)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_ASSET_REF);
    if (e && path)
        e->value.as.str = kdf__intern(obj->strings, path);
}

void kdf_obj_set_resource_ref(kdf_object *obj, const char *key, const char *path)
{
    if (!obj || !key)
        return;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_RESOURCE_REF);
    if (e && path)
        e->value.as.str = kdf__intern(obj->strings, path);
}

kdf_object *kdf_obj_add_object(kdf_object *obj, const char *key)
{
    if (!obj || !key)
        return NULL;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_OBJECT);
    if (!e)
        return NULL;
    e->value.as.object = (kdf_object *)kdf__alloc(obj->alloc, sizeof(kdf_object));
    if (!e->value.as.object)
        return NULL;
    kdf__object_init(e->value.as.object, obj->alloc, obj->strings);
    return e->value.as.object;
}

kdf_array *kdf_obj_add_array(kdf_object *obj, const char *key)
{
    if (!obj || !key)
        return NULL;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_ARRAY);
    if (!e)
        return NULL;
    e->value.as.array = (kdf_array *)kdf__alloc(obj->alloc, sizeof(kdf_array));
    if (!e->value.as.array)
        return NULL;
    kdf__array_init(e->value.as.array, obj->alloc, obj->strings);
    return e->value.as.array;
}

kdf_object *kdf_obj_add_subresource(kdf_object *obj, const char *key, const char *type)
{
    if (!obj || !key)
        return NULL;
    kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_SUBRESOURCE);
    if (!e)
        return NULL;
    e->value.as.object = (kdf_object *)kdf__alloc(obj->alloc, sizeof(kdf_object));
    if (!e->value.as.object)
        return NULL;
    kdf__object_init(e->value.as.object, obj->alloc, obj->strings);
    if (type)
    {
        e->value.as.object->type_name = kdf__intern(obj->strings, type);
    }
    return e->value.as.object;
}

// getters

kdf_value *kdf_obj_get(kdf_object *obj, const char *key)
{
    if (!obj || !key)
        return NULL;
    size_t len = strlen(key);
    size_t hash = kdf__fnv1a(key, len);
    kdf_entry *e = kdf__object_find(obj, key, len, hash);
    return e ? &e->value : NULL;
}

const kdf_value *kdf_obj_get_const(const kdf_object *obj, const char *key)
{
    if (!obj || !key)
        return NULL;
    size_t len = strlen(key);
    size_t hash = kdf__fnv1a(key, len);
    kdf_entry *e = kdf__object_find(obj, key, len, hash);
    return e ? &e->value : NULL;
}

bool kdf_obj_has(const kdf_object *obj, const char *key)
{
    return kdf_obj_get_const(obj, key) != NULL;
}

kdf_type kdf_obj_get_type_at(const kdf_object *obj, const char *key)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    return v ? v->type : KDF_TYPE_NULL;
}

bool kdf_obj_get_bool(const kdf_object *obj, const char *key, bool fallback)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    return v ? kdf_val_as_bool(v) : fallback;
}

int64_t kdf_obj_get_int(const kdf_object *obj, const char *key, int64_t fallback)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    return v ? kdf_val_as_int(v) : fallback;
}

uint64_t kdf_obj_get_uint(const kdf_object *obj, const char *key, uint64_t fallback)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    return v ? kdf_val_as_uint(v) : fallback;
}

double kdf_obj_get_float(const kdf_object *obj, const char *key, double fallback)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    return v ? kdf_val_as_float(v) : fallback;
}

const char *kdf_obj_get_string(const kdf_object *obj, const char *key, const char *fallback)
{
    const kdf_value *v = kdf_obj_get_const(obj, key);
    if (!v)
        return fallback;
    const char *s = kdf_val_as_string(v);
    return s ? s : fallback;
}

// iteration

const kdf_entry *kdf_obj_first(const kdf_object *obj)
{
    return obj ? obj->first : NULL;
}

const kdf_entry *kdf_obj_next(const kdf_entry *entry)
{
    return entry ? entry->order_next : NULL;
}

const char *kdf_entry_key(const kdf_entry *entry)
{
    return (entry && entry->key) ? entry->key->data : NULL;
}

const kdf_value *kdf_entry_value(const kdf_entry *entry)
{
    return entry ? &entry->value : NULL;
}

size_t kdf_obj_count(const kdf_object *obj)
{
    return obj ? obj->count : 0;
}

// removal

bool kdf_obj_remove(kdf_object *obj, const char *key)
{
    if (!obj || !key || !obj->buckets)
        return false;

    size_t key_len = strlen(key);
    size_t hash = kdf__fnv1a(key, key_len);
    size_t idx = hash % obj->bucket_count;

    kdf_entry *prev = NULL;
    for (kdf_entry *e = obj->buckets[idx]; e; e = e->hash_next)
    {
        if (e->key->hash == hash && e->key->length == key_len && memcmp(e->key->data, key, key_len) == 0)
        {
            // remove from hash chain
            if (prev)
            {
                prev->hash_next = e->hash_next;
            }
            else
            {
                obj->buckets[idx] = e->hash_next;
            }
            // remove from order list
            if (e->order_prev)
            {
                e->order_prev->order_next = e->order_next;
            }
            else
            {
                obj->first = e->order_next;
            }
            if (e->order_next)
            {
                e->order_next->order_prev = e->order_prev;
            }
            else
            {
                obj->last = e->order_prev;
            }
            kdf__value_destroy(&e->value, obj->alloc);
            kdf__free(obj->alloc, e, sizeof(kdf_entry));
            obj->count--;
            return true;
        }
        prev = e;
    }
    return false;
}

void kdf_obj_clear(kdf_object *obj)
{
    if (!obj)
        return;
    kdf_entry *e = obj->first;
    while (e)
    {
        kdf_entry *next = e->order_next;
        kdf__value_destroy(&e->value, obj->alloc);
        kdf__free(obj->alloc, e, sizeof(kdf_entry));
        e = next;
    }
    if (obj->buckets)
    {
        memset(obj->buckets, 0, sizeof(kdf_entry *) * obj->bucket_count);
    }
    obj->count = 0;
    obj->first = NULL;
    obj->last = NULL;
}
