#include "kdf_internal.h"

// value getters

kdf_type kdf_val_type(const kdf_value *val)
{
    return val ? val->type : KDF_TYPE_NULL;
}

bool kdf_val_is_null(const kdf_value *val)
{
    return !val || val->type == KDF_TYPE_NULL;
}

bool kdf_val_as_bool(const kdf_value *val)
{
    if (!val)
        return false;
    switch (val->type)
    {
    case KDF_TYPE_BOOL:
        return val->as.boolean;
    case KDF_TYPE_INT:
        return val->as.i64 != 0;
    case KDF_TYPE_UINT:
        return val->as.u64 != 0;
    case KDF_TYPE_FLOAT:
        return val->as.f64 != 0.0;
    default:
        return false;
    }
}

int64_t kdf_val_as_int(const kdf_value *val)
{
    if (!val)
        return 0;
    switch (val->type)
    {
    case KDF_TYPE_INT:
        return val->as.i64;
    case KDF_TYPE_UINT:
        return (int64_t)val->as.u64;
    case KDF_TYPE_FLOAT:
        return (int64_t)val->as.f64;
    case KDF_TYPE_BOOL:
        return val->as.boolean ? 1 : 0;
    default:
        return 0;
    }
}

uint64_t kdf_val_as_uint(const kdf_value *val)
{
    if (!val)
        return 0;
    switch (val->type)
    {
    case KDF_TYPE_UINT:
        return val->as.u64;
    case KDF_TYPE_INT:
        return (uint64_t)val->as.i64;
    case KDF_TYPE_FLOAT:
        return (uint64_t)val->as.f64;
    case KDF_TYPE_BOOL:
        return val->as.boolean ? 1 : 0;
    default:
        return 0;
    }
}

double kdf_val_as_float(const kdf_value *val)
{
    if (!val)
        return 0.0;
    switch (val->type)
    {
    case KDF_TYPE_FLOAT:
        return val->as.f64;
    case KDF_TYPE_INT:
        return (double)val->as.i64;
    case KDF_TYPE_UINT:
        return (double)val->as.u64;
    case KDF_TYPE_BOOL:
        return val->as.boolean ? 1.0 : 0.0;
    default:
        return 0.0;
    }
}

const char *kdf_val_as_string(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_STRING)
        return NULL;
    return val->as.str ? val->as.str->data : NULL;
}

const float *kdf_val_as_vec2(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_VEC2)
        return NULL;
    return val->as.vec2;
}

const float *kdf_val_as_vec3(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_VEC3)
        return NULL;
    return val->as.vec3;
}

const float *kdf_val_as_vec4(const kdf_value *val)
{
    if (!val || (val->type != KDF_TYPE_VEC4 && val->type != KDF_TYPE_QUAT))
        return NULL;
    return val->as.vec4;
}

const float *kdf_val_as_quat(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_QUAT)
        return NULL;
    return val->as.vec4;
}

const float *kdf_val_as_color(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_COLOR)
        return NULL;
    return val->as.vec4;
}

kdf_object *kdf_val_as_object(kdf_value *val)
{
    if (!val || (val->type != KDF_TYPE_OBJECT && val->type != KDF_TYPE_SUBRESOURCE))
        return NULL;
    return val->as.object;
}

kdf_array *kdf_val_as_array(kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_ARRAY)
        return NULL;
    return val->as.array;
}

const char *kdf_val_as_asset_ref(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_ASSET_REF)
        return NULL;
    return val->as.str ? val->as.str->data : NULL;
}

const char *kdf_val_as_resource_ref(const kdf_value *val)
{
    if (!val || val->type != KDF_TYPE_RESOURCE_REF)
        return NULL;
    return val->as.str ? val->as.str->data : NULL;
}

// value destruction

void kdf__value_destroy(kdf_value *val, kdf_allocator *alloc)
{
    if (!val)
        return;
    switch (val->type)
    {
    case KDF_TYPE_ARRAY:
        if (val->as.array)
        {
            kdf__array_destroy(val->as.array);
            kdf__free(alloc, val->as.array, sizeof(kdf_array));
        }
        break;
    case KDF_TYPE_OBJECT:
    case KDF_TYPE_SUBRESOURCE:
        if (val->as.object)
        {
            kdf__object_destroy(val->as.object);
            kdf__free(alloc, val->as.object, sizeof(kdf_object));
        }
        break;
    default:
        break;
    }
    val->type = KDF_TYPE_NULL;
}
