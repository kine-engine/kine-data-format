#ifndef KDF_ARRAY_H
#define KDF_ARRAY_H

#include "kdf_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // push values onto the array
    void kdf_arr_push_null(kdf_array *arr);
    void kdf_arr_push_bool(kdf_array *arr, bool value);
    void kdf_arr_push_int(kdf_array *arr, int64_t value);
    void kdf_arr_push_uint(kdf_array *arr, uint64_t value);
    void kdf_arr_push_float(kdf_array *arr, double value);
    void kdf_arr_push_string(kdf_array *arr, const char *value);
    void kdf_arr_push_vec2(kdf_array *arr, float x, float y);
    void kdf_arr_push_vec3(kdf_array *arr, float x, float y, float z);
    void kdf_arr_push_vec4(kdf_array *arr, float x, float y, float z, float w);
    void kdf_arr_push_quat(kdf_array *arr, float x, float y, float z, float w);
    void kdf_arr_push_color(kdf_array *arr, float r, float g, float b, float a);
    void kdf_arr_push_asset_ref(kdf_array *arr, const char *path);

    // push a child object or array
    kdf_object *kdf_arr_push_object(kdf_array *arr);
    kdf_array *kdf_arr_push_array(kdf_array *arr);

    // access
    size_t kdf_arr_count(const kdf_array *arr);
    kdf_value *kdf_arr_get(kdf_array *arr, size_t index);
    const kdf_value *kdf_arr_get_const(const kdf_array *arr, size_t index);
    kdf_type kdf_arr_get_type(const kdf_array *arr, size_t index);

    // modification
    void kdf_arr_clear(kdf_array *arr);
    void kdf_arr_remove(kdf_array *arr, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* KDF_ARRAY_H */
