#include "kdf_internal.h"

// version

const char *kdf_version(void)
{
    return "0.1.0";
}

// default allocator

kdf_allocator kdf_allocator_default(void)
{
    kdf_allocator a;
    a.alloc = kdf__default_alloc;
    a.realloc = kdf__default_realloc;
    a.free = kdf__default_free;
    a.userdata = NULL;
    return a;
}

// string pool

void kdf__string_pool_init(kdf_string_pool *pool, kdf_allocator *alloc)
{
    memset(pool->buckets, 0, sizeof(pool->buckets));
    pool->alloc = alloc;
}

void kdf__string_pool_destroy(kdf_string_pool *pool)
{
    for (size_t i = 0; i < KDF_STRING_POOL_BUCKETS; i++)
    {
        kdf_string *s = pool->buckets[i];
        while (s)
        {
            kdf_string *next = s->next;
            kdf__free(pool->alloc, s->data, s->length + 1);
            kdf__free(pool->alloc, s, sizeof(kdf_string));
            s = next;
        }
        pool->buckets[i] = NULL;
    }
}

kdf_string *kdf__intern_len(kdf_string_pool *pool, const char *str, size_t len)
{
    size_t hash = kdf__fnv1a(str, len);
    size_t bucket = hash % KDF_STRING_POOL_BUCKETS;

    // check if already interned
    for (kdf_string *s = pool->buckets[bucket]; s; s = s->next)
    {
        if (s->hash == hash && s->length == len && memcmp(s->data, str, len) == 0)
        {
            return s;
        }
    }

    // create new
    kdf_string *s = (kdf_string *)kdf__alloc(pool->alloc, sizeof(kdf_string));
    if (!s)
        return NULL;

    s->data = (char *)kdf__alloc(pool->alloc, len + 1);
    if (!s->data)
    {
        kdf__free(pool->alloc, s, sizeof(kdf_string));
        return NULL;
    }
    memcpy(s->data, str, len);
    s->data[len] = '\0';
    s->length = len;
    s->hash = hash;
    s->next = pool->buckets[bucket];
    pool->buckets[bucket] = s;

    return s;
}

kdf_string *kdf__intern(kdf_string_pool *pool, const char *str)
{
    return kdf__intern_len(pool, str, strlen(str));
}

// document

kdf_document *kdf_doc_create(void)
{
    return kdf_doc_create_alloc(NULL);
}

kdf_document *kdf_doc_create_alloc(const kdf_allocator *alloc)
{
    kdf_allocator a = kdf__alloc_or_default(alloc);
    kdf_document *doc = (kdf_document *)a.alloc(a.userdata, sizeof(kdf_document));
    if (!doc)
        return NULL;

    doc->alloc = a;
    kdf__string_pool_init(&doc->strings, &doc->alloc);
    kdf__object_init(&doc->root, &doc->alloc, &doc->strings);

    return doc;
}

void kdf_doc_destroy(kdf_document *doc)
{
    if (!doc)
        return;
    kdf__object_destroy(&doc->root);
    kdf__string_pool_destroy(&doc->strings);
    kdf_allocator a = doc->alloc;
    kdf__free(&a, doc, sizeof(kdf_document));
}

kdf_object *kdf_doc_root(kdf_document *doc)
{
    return doc ? &doc->root : NULL;
}
