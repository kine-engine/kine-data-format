#ifndef KDF_VALUE_H
#define KDF_VALUE_H

#include "kdf_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    kdf_type kdf_val_type(const kdf_value *val);
    bool kdf_val_is_null(const kdf_value *val);

    bool kdf_val_as_bool(const kdf_value *val);
    int64_t kdf_val_as_int(const kdf_value *val);
    uint64_t kdf_val_as_uint(const kdf_value *val);
    double kdf_val_as_float(const kdf_value *val);
    const char *kdf_val_as_string(const kdf_value *val);
    const float *kdf_val_as_vec2(const kdf_value *val);
    const float *kdf_val_as_vec3(const kdf_value *val);
    const float *kdf_val_as_vec4(const kdf_value *val);
    const float *kdf_val_as_quat(const kdf_value *val);
    const float *kdf_val_as_color(const kdf_value *val);
    kdf_object *kdf_val_as_object(kdf_value *val);
    kdf_array *kdf_val_as_array(kdf_value *val);
    const char *kdf_val_as_asset_ref(const kdf_value *val);
    const char *kdf_val_as_resource_ref(const kdf_value *val);

#ifdef __cplusplus
}
#endif

#endif /* KDF_VALUE_H */
