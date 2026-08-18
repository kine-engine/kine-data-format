#ifndef KDF_TYPES_H
#define KDF_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define KDF_VERSION_MAJOR 0
#define KDF_VERSION_MINOR 1
#define KDF_VERSION_PATCH 0

    // opaque types
    typedef struct kdf_document kdf_document;
    typedef struct kdf_object kdf_object;
    typedef struct kdf_array kdf_array;
    typedef struct kdf_value kdf_value;
    typedef struct kdf_entry kdf_entry;

    // value type tags
    typedef enum
    {
        KDF_TYPE_NULL = 0,
        KDF_TYPE_BOOL,
        KDF_TYPE_INT,
        KDF_TYPE_UINT,
        KDF_TYPE_FLOAT,
        KDF_TYPE_STRING,
        KDF_TYPE_VEC2,
        KDF_TYPE_VEC3,
        KDF_TYPE_VEC4,
        KDF_TYPE_QUAT,
        KDF_TYPE_COLOR,
        KDF_TYPE_ARRAY,
        KDF_TYPE_OBJECT,
        KDF_TYPE_ASSET_REF,
        KDF_TYPE_RESOURCE_REF,
        KDF_TYPE_SUBRESOURCE
    } kdf_type;

    // error codes
    typedef enum
    {
        KDF_OK = 0,
        KDF_ERROR_INVALID_ARGUMENT = -1,
        KDF_ERROR_OUT_OF_MEMORY = -2,
        KDF_ERROR_PARSE = -3,
        KDF_ERROR_IO = -4,
        KDF_ERROR_INVALID_TYPE = -5,
        KDF_ERROR_NOT_FOUND = -6,
        KDF_ERROR_INVALID_FORMAT = -7,
        KDF_ERROR_UNSUPPORTED_VERSION = -8,
        KDF_ERROR_BUFFER_TOO_SMALL = -9
    } kdf_error;

#ifdef __cplusplus
}
#endif

#endif /* KDF_TYPES_H */
