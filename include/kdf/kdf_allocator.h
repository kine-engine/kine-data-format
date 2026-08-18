#ifndef KDF_ALLOCATOR_H
#define KDF_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct kdf_allocator
    {
        void *(*alloc)(void *userdata, size_t size);
        void *(*realloc)(void *userdata, void *ptr, size_t old_size, size_t new_size);
        void (*free)(void *userdata, void *ptr, size_t size);
        void *userdata;
    } kdf_allocator;

    // returns a default allocator that uses malloc/realloc/free
    kdf_allocator kdf_allocator_default(void);

#ifdef __cplusplus
}
#endif

#endif /* KDF_ALLOCATOR_H */
