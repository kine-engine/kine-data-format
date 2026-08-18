#ifndef KDF_OBJECT_H
#define KDF_OBJECT_H

#include "kdf_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // object metadata
    void kdf_obj_set_type(kdf_object *obj, const char *type);
    void kdf_obj_set_version(kdf_object *obj, int version);
    const char *kdf_obj_type(const kdf_object *obj);
    int kdf_obj_version(const kdf_object *obj);

    // setters - add or replace a property
    void kdf_obj_set_null(kdf_object *obj, const char *key);
    void kdf_obj_set_bool(kdf_object *obj, const char *key, bool value);
    void kdf_obj_set_int(kdf_object *obj, const char *key, int64_t value);
    void kdf_obj_set_uint(kdf_object *obj, const char *key, uint64_t value);
    void kdf_obj_set_float(kdf_object *obj, const char *key, double value);
    void kdf_obj_set_string(kdf_object *obj, const char *key, const char *value);
    void kdf_obj_set_vec2(kdf_object *obj, const char *key, float x, float y);
    void kdf_obj_set_vec3(kdf_object *obj, const char *key, float x, float y, float z);
    void kdf_obj_set_vec4(kdf_object *obj, const char *key, float x, float y, float z, float w);
    void kdf_obj_set_quat(kdf_object *obj, const char *key, float x, float y, float z, float w);
    void kdf_obj_set_color(kdf_object *obj, const char *key, float r, float g, float b, float a);
    void kdf_obj_set_asset_ref(kdf_object *obj, const char *key, const char *path);
    void kdf_obj_set_resource_ref(kdf_object *obj, const char *key, const char *path);

    // add a child object or array. returns the new child.
    kdf_object *kdf_obj_add_object(kdf_object *obj, const char *key);
    kdf_array *kdf_obj_add_array(kdf_object *obj, const char *key);

    // add a typed subresource (object with type name).
    kdf_object *kdf_obj_add_subresource(kdf_object *obj, const char *key, const char *type);

    // getters - return pointer into the object, or null if not found.
    kdf_value *kdf_obj_get(kdf_object *obj, const char *key);
    const kdf_value *kdf_obj_get_const(const kdf_object *obj, const char *key);
    bool kdf_obj_has(const kdf_object *obj, const char *key);
    kdf_type kdf_obj_get_type_at(const kdf_object *obj, const char *key);

    // typed getters with fallback values
    bool kdf_obj_get_bool(const kdf_object *obj, const char *key, bool fallback);
    int64_t kdf_obj_get_int(const kdf_object *obj, const char *key, int64_t fallback);
    uint64_t kdf_obj_get_uint(const kdf_object *obj, const char *key, uint64_t fallback);
    double kdf_obj_get_float(const kdf_object *obj, const char *key, double fallback);
    const char *kdf_obj_get_string(const kdf_object *obj, const char *key, const char *fallback);

    // iteration - order follows insertion order
    const kdf_entry *kdf_obj_first(const kdf_object *obj);
    const kdf_entry *kdf_obj_next(const kdf_entry *entry);
    const char *kdf_entry_key(const kdf_entry *entry);
    const kdf_value *kdf_entry_value(const kdf_entry *entry);
    size_t kdf_obj_count(const kdf_object *obj);

    // removal
    bool kdf_obj_remove(kdf_object *obj, const char *key);
    void kdf_obj_clear(kdf_object *obj);

#ifdef __cplusplus
}
#endif

#endif /* KDF_OBJECT_H */
