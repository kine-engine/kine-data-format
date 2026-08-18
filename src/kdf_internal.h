#ifndef KDF_INTERNAL_H
#define KDF_INTERNAL_H

#include "kdf/kdf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// default allocator

static void *kdf__default_alloc(void *ud, size_t size)
{
    (void)ud;
    return malloc(size);
}

static void *kdf__default_realloc(void *ud, void *ptr, size_t old_size, size_t new_size)
{
    (void)ud;
    (void)old_size;
    return realloc(ptr, new_size);
}

static void kdf__default_free(void *ud, void *ptr, size_t size)
{
    (void)ud;
    (void)size;
    free(ptr);
}

static inline kdf_allocator kdf__alloc_or_default(const kdf_allocator *alloc)
{
    if (alloc && alloc->alloc)
    {
        return *alloc;
    }
    kdf_allocator a;
    a.alloc = kdf__default_alloc;
    a.realloc = kdf__default_realloc;
    a.free = kdf__default_free;
    a.userdata = NULL;
    return a;
}

static inline void *kdf__alloc(kdf_allocator *a, size_t size)
{
    return a->alloc(a->userdata, size);
}

static inline void *kdf__realloc(kdf_allocator *a, void *ptr, size_t old_size, size_t new_size)
{
    return a->realloc(a->userdata, ptr, old_size, new_size);
}

static inline void kdf__free(kdf_allocator *a, void *ptr, size_t size)
{
    a->free(a->userdata, ptr, size);
}

// string interning

typedef struct kdf_string
{
    char *data;
    size_t length;
    size_t hash;
    struct kdf_string *next; // hash chain
} kdf_string;

#define KDF_STRING_POOL_BUCKETS 64

typedef struct kdf_string_pool
{
    kdf_string *buckets[KDF_STRING_POOL_BUCKETS];
    kdf_allocator *alloc;
} kdf_string_pool;

// intern a string. returns a stable pointer that lives as long as the pool.
kdf_string *kdf__intern(kdf_string_pool *pool, const char *str);
kdf_string *kdf__intern_len(kdf_string_pool *pool, const char *str, size_t len);
void kdf__string_pool_init(kdf_string_pool *pool, kdf_allocator *alloc);
void kdf__string_pool_destroy(kdf_string_pool *pool);

// fnv-1a hash

static inline size_t kdf__fnv1a(const char *data, size_t len)
{
    size_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (unsigned char)data[i];
        h *= 0x100000001b3ULL;
    }
    return h ? h : 1; // avoid 0 as a valid hash
}

// value

struct kdf_value
{
    kdf_type type;
    union
    {
        bool boolean;
        int64_t i64;
        uint64_t u64;
        double f64;
        kdf_string *str;
        float vec2[2];
        float vec3[3];
        float vec4[4];
        kdf_object *object;
        kdf_array *array;
    } as;
};

void kdf__value_destroy(kdf_value *val, kdf_allocator *alloc);

// object entry (hash table node)

struct kdf_entry
{
    kdf_string *key;
    kdf_value value;
    struct kdf_entry *hash_next;  // hash chain
    struct kdf_entry *order_next; // insertion-order linked list
    struct kdf_entry *order_prev;
};

#define KDF_OBJECT_INIT_BUCKETS 8

struct kdf_object
{
    kdf_string *type_name; // null if untyped
    int version;           // 0 if unversioned
    kdf_entry **buckets;
    size_t bucket_count;
    size_t count;
    kdf_entry *first; // insertion-order list
    kdf_entry *last;
    kdf_allocator *alloc;
    kdf_string_pool *strings;
};

void kdf__object_init(kdf_object *obj, kdf_allocator *alloc, kdf_string_pool *strings);
void kdf__object_destroy(kdf_object *obj);
kdf_entry *kdf__object_find(const kdf_object *obj, const char *key, size_t key_len, size_t hash);
kdf_entry *kdf__object_insert(kdf_object *obj, const char *key, kdf_type type);
void kdf__object_grow(kdf_object *obj);

// array

#define KDF_ARRAY_INIT_CAP 8

struct kdf_array
{
    kdf_value *items;
    size_t count;
    size_t capacity;
    kdf_allocator *alloc;
    kdf_string_pool *strings; // for interning strings, may be null
};

void kdf__array_init(kdf_array *arr, kdf_allocator *alloc, kdf_string_pool *strings);
void kdf__array_destroy(kdf_array *arr);
void kdf__array_grow(kdf_array *arr);
void kdf__arr_push_value(kdf_array *arr, const kdf_value *val);

// document

struct kdf_document
{
    kdf_allocator alloc;
    kdf_string_pool strings;
    kdf_object root;
};

// binary format constants

enum
{
    KDF_BIN_NULL = 0x00,
    KDF_BIN_BOOL = 0x01,
    KDF_BIN_INT = 0x02,
    KDF_BIN_UINT = 0x03,
    KDF_BIN_FLOAT = 0x04,
    KDF_BIN_STRING = 0x05,
    KDF_BIN_VEC2 = 0x06,
    KDF_BIN_VEC3 = 0x07,
    KDF_BIN_VEC4 = 0x08,
    KDF_BIN_QUAT = 0x09,
    KDF_BIN_COLOR = 0x0A,
    KDF_BIN_ARRAY = 0x0B,
    KDF_BIN_OBJECT = 0x0C,
    KDF_BIN_ASSET_REF = 0x0D,
    KDF_BIN_RESOURCE_REF = 0x0E,
    KDF_BIN_SUBRESOURCE = 0x0F
};

#endif /* KDF_INTERNAL_H */
