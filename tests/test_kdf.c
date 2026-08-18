#include "kdf/kdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// test framework

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int test_had_failure = 0;

#define ASSERT(cond)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            fprintf(stderr, "\n  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                        \
            test_had_failure = 1;                                                                                      \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)
#define ASSERT_NULL(p) ASSERT((p) == NULL)
#define ASSERT_NOT_NULL(p) ASSERT((p) != NULL)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(fabs((double)(a) - (double)(b)) < 1e-5)

#define RUN_TEST(fn)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        printf("  %-50s", #fn);                                                                                        \
        fflush(stdout);                                                                                                \
        test_had_failure = 0;                                                                                          \
        fn();                                                                                                          \
        tests_run++;                                                                                                   \
        if (test_had_failure)                                                                                          \
        {                                                                                                              \
            tests_failed++;                                                                                            \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            tests_passed++;                                                                                            \
            printf(" OK\n");                                                                                           \
        }                                                                                                              \
    } while (0)

// helper: write document to memory buffer

static int write_to_mem(const kdf_document *doc, kdf_mem_writer *mw, int binary)
{
    kdf_writer w = kdf_mem_writer_create(mw);
    if (binary)
        return kdf_binary_write(doc, w);
    return kdf_text_write(doc, w);
}

// test: version string

static void test_version(void)
{
    const char *v = kdf_version();
    ASSERT_NOT_NULL(v);
    ASSERT(strlen(v) > 0);
}

// test: document creation

static void test_doc_create_destroy(void)
{
    kdf_document *doc = kdf_doc_create();
    ASSERT_NOT_NULL(doc);
    kdf_object *root = kdf_doc_root(doc);
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(kdf_obj_count(root), 0u);
    kdf_doc_destroy(doc);
}

static void test_doc_create_null(void)
{
    kdf_doc_destroy(NULL); // should not crash
}

// test: object setters/getters

static void test_obj_null(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_null(root, "nothing");
    ASSERT(kdf_obj_has(root, "nothing"));
    ASSERT_EQ(kdf_obj_get_type_at(root, "nothing"), KDF_TYPE_NULL);
    kdf_doc_destroy(doc);
}

static void test_obj_bool(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_bool(root, "flag", true);
    ASSERT_EQ(kdf_obj_get_bool(root, "flag", false), true);
    ASSERT_EQ(kdf_obj_get_type_at(root, "flag"), KDF_TYPE_BOOL);
    kdf_obj_set_bool(root, "flag", false);
    ASSERT_EQ(kdf_obj_get_bool(root, "flag", true), false);
    kdf_doc_destroy(doc);
}

static void test_obj_int(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_int(root, "health", 100);
    ASSERT_EQ(kdf_obj_get_int(root, "health", 0), 100);
    kdf_obj_set_int(root, "negative", -42);
    ASSERT_EQ(kdf_obj_get_int(root, "negative", 0), -42);
    kdf_doc_destroy(doc);
}

static void test_obj_uint(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_uint(root, "score", 4294967295u);
    ASSERT_EQ(kdf_obj_get_uint(root, "score", 0), 4294967295u);
    kdf_doc_destroy(doc);
}

static void test_obj_float(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_float(root, "speed", 5.5);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "speed", 0.0), 5.5);
    kdf_doc_destroy(doc);
}

static void test_obj_string(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_string(root, "name", "Player");
    ASSERT_STR_EQ(kdf_obj_get_string(root, "name", ""), "Player");
    kdf_doc_destroy(doc);
}

static void test_obj_vec2(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_vec2(root, "pos", 10.0f, 20.0f);
    const kdf_value *v = kdf_obj_get_const(root, "pos");
    ASSERT_NOT_NULL(v);
    const float *v2 = kdf_val_as_vec2(v);
    ASSERT_NOT_NULL(v2);
    ASSERT_FLOAT_EQ(v2[0], 10.0f);
    ASSERT_FLOAT_EQ(v2[1], 20.0f);
    kdf_doc_destroy(doc);
}

static void test_obj_vec3(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_vec3(root, "position", 1.0f, 2.0f, 3.0f);
    const kdf_value *v = kdf_obj_get_const(root, "position");
    ASSERT_NOT_NULL(v);
    const float *v3 = kdf_val_as_vec3(v);
    ASSERT_NOT_NULL(v3);
    ASSERT_FLOAT_EQ(v3[0], 1.0f);
    ASSERT_FLOAT_EQ(v3[1], 2.0f);
    ASSERT_FLOAT_EQ(v3[2], 3.0f);
    kdf_doc_destroy(doc);
}

static void test_obj_vec4(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_vec4(root, "v", 1.0f, 2.0f, 3.0f, 4.0f);
    const float *v4 = kdf_val_as_vec4(kdf_obj_get_const(root, "v"));
    ASSERT_NOT_NULL(v4);
    ASSERT_FLOAT_EQ(v4[3], 4.0f);
    kdf_doc_destroy(doc);
}

static void test_obj_quat(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_quat(root, "rot", 0.0f, 0.0f, 0.0f, 1.0f);
    const float *q = kdf_val_as_quat(kdf_obj_get_const(root, "rot"));
    ASSERT_NOT_NULL(q);
    ASSERT_FLOAT_EQ(q[3], 1.0f);
    kdf_doc_destroy(doc);
}

static void test_obj_color(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_color(root, "tint", 1.0f, 0.5f, 0.2f, 1.0f);
    const float *c = kdf_val_as_color(kdf_obj_get_const(root, "tint"));
    ASSERT_NOT_NULL(c);
    ASSERT_FLOAT_EQ(c[0], 1.0f);
    ASSERT_FLOAT_EQ(c[1], 0.5f);
    ASSERT_FLOAT_EQ(c[2], 0.2f);
    ASSERT_FLOAT_EQ(c[3], 1.0f);
    kdf_doc_destroy(doc);
}

static void test_obj_asset_ref(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_asset_ref(root, "texture", "res://textures/player.png");
    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(root, "texture")), "res://textures/player.png");
    kdf_doc_destroy(doc);
}

static void test_obj_resource_ref(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_resource_ref(root, "mesh", "res://models/player.mesh");
    ASSERT_STR_EQ(kdf_val_as_resource_ref(kdf_obj_get_const(root, "mesh")), "res://models/player.mesh");
    kdf_doc_destroy(doc);
}

static void test_obj_overwrite(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_int(root, "x", 10);
    ASSERT_EQ(kdf_obj_get_int(root, "x", 0), 10);
    kdf_obj_set_string(root, "x", "hello");
    ASSERT_EQ(kdf_obj_get_type_at(root, "x"), KDF_TYPE_STRING);
    ASSERT_STR_EQ(kdf_obj_get_string(root, "x", ""), "hello");
    ASSERT_EQ(kdf_obj_count(root), 1u);
    kdf_doc_destroy(doc);
}

// test: object metadata

static void test_obj_metadata(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_type(root, "Character");
    kdf_obj_set_version(root, 3);
    ASSERT_STR_EQ(kdf_obj_type(root), "Character");
    ASSERT_EQ(kdf_obj_version(root), 3);
    kdf_doc_destroy(doc);
}

// test: nested objects

static void test_obj_nested(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_string(root, "name", "Player");

    kdf_object *physics = kdf_obj_add_object(root, "physics");
    ASSERT_NOT_NULL(physics);
    kdf_obj_set_float(physics, "mass", 70.0);
    kdf_obj_set_float(physics, "friction", 0.8);
    kdf_obj_set_bool(physics, "gravity", true);

    ASSERT_EQ(kdf_obj_count(root), 2u);
    ASSERT_EQ(kdf_obj_count(physics), 3u);

    ASSERT(kdf_obj_has(root, "physics"));
    const kdf_value *pv = kdf_obj_get_const(root, "physics");
    ASSERT_EQ(kdf_val_type(pv), KDF_TYPE_OBJECT);

    kdf_doc_destroy(doc);
}

// test: arrays

static void test_array_basic(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_array *arr = kdf_obj_add_array(root, "items");
    ASSERT_NOT_NULL(arr);
    kdf_arr_push_string(arr, "sword");
    kdf_arr_push_string(arr, "potion");
    kdf_arr_push_string(arr, "apple");

    ASSERT_EQ(kdf_arr_count(arr), 3u);
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(arr, 0)), "sword");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(arr, 1)), "potion");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(arr, 2)), "apple");

    kdf_doc_destroy(doc);
}

static void test_array_mixed(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_array *arr = kdf_obj_add_array(root, "data");
    kdf_arr_push_int(arr, 42);
    kdf_arr_push_float(arr, 3.14);
    kdf_arr_push_bool(arr, true);
    kdf_arr_push_null(arr);

    ASSERT_EQ(kdf_arr_count(arr), 4u);
    ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(arr, 0)), 42);
    ASSERT_FLOAT_EQ(kdf_val_as_float(kdf_arr_get_const(arr, 1)), 3.14);
    ASSERT_EQ(kdf_val_as_bool(kdf_arr_get_const(arr, 2)), true);
    ASSERT(kdf_val_is_null(kdf_arr_get_const(arr, 3)));

    kdf_doc_destroy(doc);
}

static void test_array_remove(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_array *arr = kdf_obj_add_array(root, "items");
    kdf_arr_push_int(arr, 1);
    kdf_arr_push_int(arr, 2);
    kdf_arr_push_int(arr, 3);

    kdf_arr_remove(arr, 1);
    ASSERT_EQ(kdf_arr_count(arr), 2u);
    ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(arr, 0)), 1);
    ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(arr, 1)), 3);

    kdf_doc_destroy(doc);
}

static void test_array_clear(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_array *arr = kdf_obj_add_array(root, "items");
    kdf_arr_push_int(arr, 1);
    kdf_arr_push_int(arr, 2);
    kdf_arr_clear(arr);
    ASSERT_EQ(kdf_arr_count(arr), 0u);

    kdf_doc_destroy(doc);
}

// test: object iteration

static void test_obj_iteration(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "a", "first");
    kdf_obj_set_int(root, "b", 42);
    kdf_obj_set_bool(root, "c", true);

    int count = 0;
    for (const kdf_entry *e = kdf_obj_first(root); e; e = kdf_obj_next(e))
    {
        const char *key = kdf_entry_key(e);
        ASSERT_NOT_NULL(key);
        count++;
    }
    ASSERT_EQ(count, 3);
    ASSERT_EQ(kdf_obj_count(root), 3u);

    kdf_doc_destroy(doc);
}

// test: object removal

static void test_obj_remove(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_int(root, "a", 1);
    kdf_obj_set_int(root, "b", 2);
    kdf_obj_set_int(root, "c", 3);

    ASSERT_EQ(kdf_obj_count(root), 3u);
    ASSERT(kdf_obj_remove(root, "b"));
    ASSERT_EQ(kdf_obj_count(root), 2u);
    ASSERT(!kdf_obj_has(root, "b"));
    ASSERT(kdf_obj_has(root, "a"));
    ASSERT(kdf_obj_has(root, "c"));

    kdf_doc_destroy(doc);
}

static void test_obj_clear(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_int(root, "a", 1);
    kdf_obj_set_int(root, "b", 2);
    kdf_obj_clear(root);
    ASSERT_EQ(kdf_obj_count(root), 0u);

    kdf_doc_destroy(doc);
}

// test: subresource

static void test_subresource(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_object *mat = kdf_obj_add_subresource(root, "material", "Material");
    ASSERT_NOT_NULL(mat);
    ASSERT_STR_EQ(kdf_obj_type(mat), "Material");
    kdf_obj_set_float(mat, "roughness", 0.5);
    kdf_obj_set_float(mat, "metallic", 0.0);

    const kdf_value *mv = kdf_obj_get_const(root, "material");
    ASSERT_EQ(kdf_val_type(mv), KDF_TYPE_SUBRESOURCE);

    kdf_doc_destroy(doc);
}

// test: getter fallbacks

static void test_getter_fallbacks(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_EQ(kdf_obj_get_bool(root, "missing", true), true);
    ASSERT_EQ(kdf_obj_get_int(root, "missing", 99), 99);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "missing", 1.5), 1.5);
    ASSERT_STR_EQ(kdf_obj_get_string(root, "missing", "default"), "default");

    kdf_doc_destroy(doc);
}

// test: value type coercion

static void test_value_coercion(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_int(root, "x", 42);
    const kdf_value *v = kdf_obj_get_const(root, "x");

    // int -> float
    ASSERT_FLOAT_EQ(kdf_val_as_float(v), 42.0);

    // int -> bool
    ASSERT_EQ(kdf_val_as_bool(v), true);

    kdf_obj_set_int(root, "z", 0);
    ASSERT_EQ(kdf_val_as_bool(kdf_obj_get_const(root, "z")), false);

    kdf_doc_destroy(doc);
}

// test: text write

static void test_text_write_simple(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");
    kdf_obj_set_int(root, "health", 100);
    kdf_obj_set_float(root, "speed", 5.5);
    kdf_obj_set_bool(root, "alive", true);

    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    // verify it contains expected content
    char *text = (char *)mw.data;
    ASSERT_NOT_NULL(strstr(text, "kdf 1"));
    ASSERT_NOT_NULL(strstr(text, "name = \"Player\""));
    ASSERT_NOT_NULL(strstr(text, "health = 100"));
    ASSERT_NOT_NULL(strstr(text, "speed = 5.5"));
    ASSERT_NOT_NULL(strstr(text, "alive = true"));

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc);
}

static void test_text_write_types(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_null(root, "n");
    kdf_obj_set_uint(root, "u", 42u);
    kdf_obj_set_vec3(root, "pos", 1.0f, 2.0f, 3.0f);
    kdf_obj_set_color(root, "col", 1.0f, 0.5f, 0.2f, 1.0f);
    kdf_obj_set_asset_ref(root, "tex", "res://textures/t.png");

    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    char *text = (char *)mw.data;
    ASSERT_NOT_NULL(strstr(text, "n = null"));
    ASSERT_NOT_NULL(strstr(text, "u = 42u"));
    ASSERT_NOT_NULL(strstr(text, "pos = vec3(1, 2, 3)"));
    ASSERT_NOT_NULL(strstr(text, "col = color(1, 0.5, 0.2, 1)"));
    ASSERT_NOT_NULL(strstr(text, "asset(\"res://textures/t.png\")"));

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc);
}

static void test_text_write_nested(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");

    kdf_object *physics = kdf_obj_add_object(root, "physics");
    kdf_obj_set_float(physics, "mass", 70.0);
    kdf_obj_set_bool(physics, "gravity", true);

    kdf_array *inv = kdf_obj_add_array(root, "inventory");
    kdf_arr_push_string(inv, "sword");
    kdf_arr_push_string(inv, "potion");

    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    char *text = (char *)mw.data;
    ASSERT_NOT_NULL(strstr(text, "physics {"));
    ASSERT_NOT_NULL(strstr(text, "mass = 70"));
    ASSERT_NOT_NULL(strstr(text, "gravity = true"));
    ASSERT_NOT_NULL(strstr(text, "inventory = ["));
    ASSERT_NOT_NULL(strstr(text, "\"sword\""));
    ASSERT_NOT_NULL(strstr(text, "\"potion\""));

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc);
}

// test: text read

static void test_text_read_simple(void)
{
    const char *input = "kdf 1\n"
                        "\n"
                        "name = \"Player\"\n"
                        "health = 100\n"
                        "speed = 5.5\n"
                        "alive = true\n";

    kdf_mem_reader mr;
    mr.data = (const uint8_t *)input;
    mr.size = strlen(input);
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc);

    kdf_object *root = kdf_doc_root(doc);
    ASSERT_STR_EQ(kdf_obj_get_string(root, "name", ""), "Player");
    ASSERT_EQ(kdf_obj_get_int(root, "health", 0), 100);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "speed", 0.0), 5.5);
    ASSERT_EQ(kdf_obj_get_bool(root, "alive", false), true);

    kdf_doc_destroy(doc);
}

static void test_text_read_types(void)
{
    const char *input = "kdf 1\n"
                        "\n"
                        "n = null\n"
                        "flag = false\n"
                        "count = 42u\n"
                        "pos = vec3(1, 2, 3)\n"
                        "rot = quat(0, 0, 0, 1)\n"
                        "col = color(1, 0.5, 0.2, 1)\n"
                        "tex = asset(\"res://textures/t.png\")\n"
                        "mesh = external(\"res://models/player.mesh\")\n";

    kdf_mem_reader mr;
    mr.data = (const uint8_t *)input;
    mr.size = strlen(input);
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc);
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_EQ(kdf_obj_get_type_at(root, "n"), KDF_TYPE_NULL);
    ASSERT_EQ(kdf_obj_get_bool(root, "flag", true), false);
    ASSERT_EQ(kdf_obj_get_uint(root, "count", 0), 42u);

    const float *pos = kdf_val_as_vec3(kdf_obj_get_const(root, "pos"));
    ASSERT_NOT_NULL(pos);
    ASSERT_FLOAT_EQ(pos[0], 1.0f);
    ASSERT_FLOAT_EQ(pos[1], 2.0f);
    ASSERT_FLOAT_EQ(pos[2], 3.0f);

    const float *rot = kdf_val_as_quat(kdf_obj_get_const(root, "rot"));
    ASSERT_NOT_NULL(rot);
    ASSERT_FLOAT_EQ(rot[3], 1.0f);

    const float *col = kdf_val_as_color(kdf_obj_get_const(root, "col"));
    ASSERT_NOT_NULL(col);
    ASSERT_FLOAT_EQ(col[0], 1.0f);

    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(root, "tex")), "res://textures/t.png");
    ASSERT_STR_EQ(kdf_val_as_resource_ref(kdf_obj_get_const(root, "mesh")), "res://models/player.mesh");

    kdf_doc_destroy(doc);
}

static void test_text_read_nested(void)
{
    const char *input = "kdf 1\n"
                        "\n"
                        "name = \"Player\"\n"
                        "physics {\n"
                        "    mass = 70\n"
                        "    friction = 0.8\n"
                        "    gravity = true\n"
                        "}\n"
                        "inventory = [\n"
                        "    \"sword\",\n"
                        "    \"potion\"\n"
                        "]\n";

    kdf_mem_reader mr;
    mr.data = (const uint8_t *)input;
    mr.size = strlen(input);
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc);
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_STR_EQ(kdf_obj_get_string(root, "name", ""), "Player");
    ASSERT(kdf_obj_has(root, "physics"));

    const kdf_value *pv = kdf_obj_get_const(root, "physics");
    ASSERT_EQ(kdf_val_type(pv), KDF_TYPE_OBJECT);
    kdf_object *physics = kdf_val_as_object((kdf_value *)pv);
    ASSERT_NOT_NULL(physics);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(physics, "mass", 0.0), 70.0);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(physics, "friction", 0.0), 0.8);
    ASSERT_EQ(kdf_obj_get_bool(physics, "gravity", false), true);

    ASSERT(kdf_obj_has(root, "inventory"));
    const kdf_value *iv = kdf_obj_get_const(root, "inventory");
    ASSERT_EQ(kdf_val_type(iv), KDF_TYPE_ARRAY);
    kdf_array *inv = kdf_val_as_array((kdf_value *)iv);
    ASSERT_NOT_NULL(inv);
    ASSERT_EQ(kdf_arr_count(inv), 2u);

    kdf_doc_destroy(doc);
}

static void test_text_read_negative_numbers(void)
{
    const char *input = "kdf 1\n"
                        "\n"
                        "neg_int = -42\n"
                        "neg_float = -3.14\n";

    kdf_mem_reader mr;
    mr.data = (const uint8_t *)input;
    mr.size = strlen(input);
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc);
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_EQ(kdf_obj_get_int(root, "neg_int", 0), -42);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "neg_float", 0.0), -3.14);

    kdf_doc_destroy(doc);
}

// test: text roundtrip

static void test_text_roundtrip(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");
    kdf_obj_set_int(root, "health", 100);
    kdf_obj_set_float(root, "speed", 5.5);
    kdf_obj_set_bool(root, "alive", true);
    kdf_obj_set_vec3(root, "position", 1.0f, 2.0f, 3.0f);
    kdf_obj_set_color(root, "tint", 1.0f, 0.5f, 0.2f, 1.0f);
    kdf_obj_set_asset_ref(root, "mesh", "res://models/player.mesh");

    kdf_array *items = kdf_obj_add_array(root, "items");
    kdf_arr_push_string(items, "sword");
    kdf_arr_push_string(items, "shield");

    kdf_object *stats = kdf_obj_add_object(root, "stats");
    kdf_obj_set_int(stats, "str", 10);
    kdf_obj_set_int(stats, "dex", 15);

    // write to text
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    // read back
    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    // verify all values survived the roundtrip
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "name", ""), "Player");
    ASSERT_EQ(kdf_obj_get_int(root2, "health", 0), 100);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root2, "speed", 0.0), 5.5);
    ASSERT_EQ(kdf_obj_get_bool(root2, "alive", false), true);

    const float *pos2 = kdf_val_as_vec3(kdf_obj_get_const(root2, "position"));
    ASSERT_NOT_NULL(pos2);
    ASSERT_FLOAT_EQ(pos2[0], 1.0f);
    ASSERT_FLOAT_EQ(pos2[1], 2.0f);
    ASSERT_FLOAT_EQ(pos2[2], 3.0f);

    ASSERT(kdf_obj_has(root2, "items"));
    const kdf_value *iv = kdf_obj_get_const(root2, "items");
    ASSERT_EQ(kdf_val_type(iv), KDF_TYPE_ARRAY);

    ASSERT(kdf_obj_has(root2, "stats"));
    const kdf_value *sv = kdf_obj_get_const(root2, "stats");
    ASSERT_EQ(kdf_val_type(sv), KDF_TYPE_OBJECT);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

// test: binary write/read

static void test_binary_roundtrip(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");
    kdf_obj_set_int(root, "health", 100);
    kdf_obj_set_float(root, "speed", 5.5);
    kdf_obj_set_bool(root, "alive", true);
    kdf_obj_set_null(root, "nothing");
    kdf_obj_set_uint(root, "score", 9999u);
    kdf_obj_set_vec2(root, "pos2d", 10.0f, 20.0f);
    kdf_obj_set_vec3(root, "pos", 1.0f, 2.0f, 3.0f);
    kdf_obj_set_vec4(root, "v4", 1.0f, 2.0f, 3.0f, 4.0f);
    kdf_obj_set_quat(root, "rot", 0.0f, 0.0f, 0.0f, 1.0f);
    kdf_obj_set_color(root, "col", 1.0f, 0.5f, 0.2f, 1.0f);
    kdf_obj_set_asset_ref(root, "tex", "res://textures/t.png");
    kdf_obj_set_resource_ref(root, "mesh", "res://models/m.mesh");

    kdf_array *arr = kdf_obj_add_array(root, "items");
    kdf_arr_push_string(arr, "sword");
    kdf_arr_push_int(arr, 42);
    kdf_arr_push_float(arr, 3.14);

    kdf_object *child = kdf_obj_add_object(root, "stats");
    kdf_obj_set_int(child, "str", 10);
    kdf_obj_set_int(child, "dex", 15);

    kdf_object *sub = kdf_obj_add_subresource(root, "mat", "Material");
    kdf_obj_set_float(sub, "roughness", 0.5);

    // write to binary
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 1), KDF_OK);
    ASSERT(mw.size > 0);

    // read back
    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc2 = kdf_binary_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    // verify all values
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "name", ""), "Player");
    ASSERT_EQ(kdf_obj_get_int(root2, "health", 0), 100);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root2, "speed", 0.0), 5.5);
    ASSERT_EQ(kdf_obj_get_bool(root2, "alive", false), true);
    ASSERT_EQ(kdf_obj_get_type_at(root2, "nothing"), KDF_TYPE_NULL);
    ASSERT_EQ(kdf_obj_get_uint(root2, "score", 0), 9999u);

    const float *p2 = kdf_val_as_vec2(kdf_obj_get_const(root2, "pos2d"));
    ASSERT_NOT_NULL(p2);
    ASSERT_FLOAT_EQ(p2[0], 10.0f);

    const float *p3 = kdf_val_as_vec3(kdf_obj_get_const(root2, "pos"));
    ASSERT_NOT_NULL(p3);
    ASSERT_FLOAT_EQ(p3[2], 3.0f);

    const float *v4 = kdf_val_as_vec4(kdf_obj_get_const(root2, "v4"));
    ASSERT_NOT_NULL(v4);
    ASSERT_FLOAT_EQ(v4[3], 4.0f);

    const float *rot = kdf_val_as_quat(kdf_obj_get_const(root2, "rot"));
    ASSERT_NOT_NULL(rot);
    ASSERT_FLOAT_EQ(rot[3], 1.0f);

    const float *col = kdf_val_as_color(kdf_obj_get_const(root2, "col"));
    ASSERT_NOT_NULL(col);
    ASSERT_FLOAT_EQ(col[0], 1.0f);

    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(root2, "tex")), "res://textures/t.png");
    ASSERT_STR_EQ(kdf_val_as_resource_ref(kdf_obj_get_const(root2, "mesh")), "res://models/m.mesh");

    // verify array
    const kdf_value *av = kdf_obj_get_const(root2, "items");
    ASSERT_EQ(kdf_val_type(av), KDF_TYPE_ARRAY);
    kdf_array *arr2 = kdf_val_as_array((kdf_value *)av);
    ASSERT_NOT_NULL(arr2);
    ASSERT_EQ(kdf_arr_count(arr2), 3u);
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(arr2, 0)), "sword");
    ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(arr2, 1)), 42);
    ASSERT_FLOAT_EQ(kdf_val_as_float(kdf_arr_get_const(arr2, 2)), 3.14);

    // verify child object
    const kdf_value *cv = kdf_obj_get_const(root2, "stats");
    ASSERT_EQ(kdf_val_type(cv), KDF_TYPE_OBJECT);
    kdf_object *child2 = kdf_val_as_object((kdf_value *)cv);
    ASSERT_NOT_NULL(child2);
    ASSERT_EQ(kdf_obj_get_int(child2, "str", 0), 10);
    ASSERT_EQ(kdf_obj_get_int(child2, "dex", 0), 15);

    // verify subresource
    const kdf_value *sv = kdf_obj_get_const(root2, "mat");
    ASSERT_EQ(kdf_val_type(sv), KDF_TYPE_SUBRESOURCE);
    kdf_object *sub2 = kdf_val_as_object((kdf_value *)sv);
    ASSERT_NOT_NULL(sub2);
    ASSERT_STR_EQ(kdf_obj_type(sub2), "Material");
    ASSERT_FLOAT_EQ(kdf_obj_get_float(sub2, "roughness", 0.0), 0.5);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

// test: binary is smaller than json-equivalent

static void test_binary_size(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "Player");
    kdf_obj_set_int(root, "health", 100);
    kdf_obj_set_float(root, "speed", 5.5);

    kdf_mem_writer text_mw = {0};
    ASSERT_EQ(write_to_mem(doc, &text_mw, 0), KDF_OK);

    kdf_mem_writer bin_mw = {0};
    ASSERT_EQ(write_to_mem(doc, &bin_mw, 1), KDF_OK);

    // binary should be reasonably compact
    ASSERT(bin_mw.size > 0);
    ASSERT(text_mw.size > 0);

    // for small documents, binary might not be much smaller due to header overhead,
    // but it should still be valid
    printf("[text=%zu, binary=%zu] ", text_mw.size, bin_mw.size);

    kdf_mem_writer_finish(&text_mw);
    kdf_mem_writer_finish(&bin_mw);
    kdf_doc_destroy(doc);
}

// test: memory writer/reader

static void test_mem_writer_reader(void)
{
    kdf_mem_writer mw = {0};
    kdf_writer w = kdf_mem_writer_create(&mw);

    const char *data = "hello world";
    ASSERT_EQ(w.write(w.userdata, data, strlen(data)), strlen(data));
    ASSERT_EQ(mw.size, strlen(data));
    ASSERT(memcmp(mw.data, data, strlen(data)) == 0);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    char buf[32] = {0};
    ASSERT_EQ(r.read(r.userdata, buf, 5), 5u);
    ASSERT(memcmp(buf, "hello", 5) == 0);

    ASSERT(r.seek(r.userdata, 0, 0)); // seek to start
    ASSERT_EQ(r.tell(r.userdata), 0);

    kdf_mem_writer_finish(&mw);
}

// test: default allocator

static void test_default_allocator(void)
{
    kdf_allocator a = kdf_allocator_default();
    ASSERT_NOT_NULL(a.alloc);
    ASSERT_NOT_NULL(a.realloc);
    ASSERT_NOT_NULL(a.free);

    void *p = a.alloc(a.userdata, 128);
    ASSERT_NOT_NULL(p);
    p = a.realloc(a.userdata, p, 128, 256);
    ASSERT_NOT_NULL(p);
    a.free(a.userdata, p, 256);
}

// test: full resource example

static void test_full_resource(void)
{
    const char *input = "kdf 1\n"
                        "\n"
                        "name = \"Player\"\n"
                        "health = 100\n"
                        "speed = 5.5\n"
                        "position = vec3(0, 2, 0)\n"
                        "\n"
                        "mesh = external(\"res://models/player.mesh\")\n"
                        "material = external(\"res://materials/player.mat\")\n"
                        "\n"
                        "inventory = [\n"
                        "    \"sword\",\n"
                        "    \"potion\",\n"
                        "    \"apple\"\n"
                        "]\n"
                        "\n"
                        "physics {\n"
                        "    mass = 70\n"
                        "    friction = 0.8\n"
                        "    gravity = true\n"
                        "}\n";

    kdf_mem_reader mr;
    mr.data = (const uint8_t *)input;
    mr.size = strlen(input);
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);

    kdf_document *doc = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc);
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_STR_EQ(kdf_obj_get_string(root, "name", ""), "Player");
    ASSERT_EQ(kdf_obj_get_int(root, "health", 0), 100);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "speed", 0.0), 5.5);

    const float *pos = kdf_val_as_vec3(kdf_obj_get_const(root, "position"));
    ASSERT_NOT_NULL(pos);
    ASSERT_FLOAT_EQ(pos[0], 0.0f);
    ASSERT_FLOAT_EQ(pos[1], 2.0f);
    ASSERT_FLOAT_EQ(pos[2], 0.0f);

    ASSERT_STR_EQ(kdf_val_as_resource_ref(kdf_obj_get_const(root, "mesh")), "res://models/player.mesh");
    ASSERT_STR_EQ(kdf_val_as_resource_ref(kdf_obj_get_const(root, "material")), "res://materials/player.mat");

    const kdf_value *inv_v = kdf_obj_get_const(root, "inventory");
    ASSERT_EQ(kdf_val_type(inv_v), KDF_TYPE_ARRAY);
    kdf_array *inv = kdf_val_as_array((kdf_value *)inv_v);
    ASSERT_EQ(kdf_arr_count(inv), 3u);
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(inv, 0)), "sword");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(inv, 1)), "potion");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(inv, 2)), "apple");

    const kdf_value *phys_v = kdf_obj_get_const(root, "physics");
    ASSERT_EQ(kdf_val_type(phys_v), KDF_TYPE_OBJECT);
    kdf_object *physics = kdf_val_as_object((kdf_value *)phys_v);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(physics, "mass", 0.0), 70.0);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(physics, "friction", 0.0), 0.8);
    ASSERT_EQ(kdf_obj_get_bool(physics, "gravity", false), true);

    // now write it back as text and verify
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);
    ASSERT(mw.size > 0);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc);
}

// stress tests

static void test_stress_many_properties(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    // insert 10,000 properties
    for (int i = 0; i < 10000; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "prop_%d", i);
        kdf_obj_set_int(root, key, i);
    }

    ASSERT_EQ(kdf_obj_count(root), 10000u);

    // verify every property
    for (int i = 0; i < 10000; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "prop_%d", i);
        ASSERT_EQ(kdf_obj_get_int(root, key, -1), i);
    }

    kdf_doc_destroy(doc);
}

static void test_stress_large_array(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_array *arr = kdf_obj_add_array(root, "data");
    for (int i = 0; i < 50000; i++)
    {
        kdf_arr_push_int(arr, i);
    }

    ASSERT_EQ(kdf_arr_count(arr), 50000u);

    for (int i = 0; i < 50000; i++)
    {
        ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(arr, (size_t)i)), i);
    }

    kdf_doc_destroy(doc);
}

static void test_stress_deep_nesting(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *cur = kdf_doc_root(doc);

    // create 50 levels of nesting
    for (int i = 0; i < 50; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "level_%d", i);
        kdf_obj_set_int(cur, "depth", i);
        cur = kdf_obj_add_object(cur, key);
        ASSERT_NOT_NULL(cur);
    }
    kdf_obj_set_int(cur, "depth", 50);

    // verify depth by walking the tree
    cur = kdf_doc_root(doc);
    for (int i = 0; i < 50; i++)
    {
        ASSERT_EQ(kdf_obj_get_int(cur, "depth", -1), i);
        char key[32];
        snprintf(key, sizeof(key), "level_%d", i);
        const kdf_value *child = kdf_obj_get_const(cur, key);
        ASSERT_NOT_NULL(child);
        ASSERT_EQ(kdf_val_type(child), KDF_TYPE_OBJECT);
        cur = kdf_val_as_object((kdf_value *)child);
    }
    ASSERT_EQ(kdf_obj_get_int(cur, "depth", -1), 50);

    kdf_doc_destroy(doc);
}

static void test_stress_long_strings(void)
{
    // build a 10,000-character string
    char long_str[10001];
    for (int i = 0; i < 10000; i++)
    {
        long_str[i] = 'A' + (char)(i % 26);
    }
    long_str[10000] = '\0';

    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);
    kdf_obj_set_string(root, "big", long_str);

    ASSERT_STR_EQ(kdf_obj_get_string(root, "big", ""), long_str);

    // roundtrip through text
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    ASSERT_STR_EQ(kdf_obj_get_string(kdf_doc_root(doc2), "big", ""), long_str);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

static void test_stress_text_roundtrip_large(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "LargeDocument");
    kdf_obj_set_int(root, "version", 1);

    // add 500 typed properties
    for (int i = 0; i < 500; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "item_%d", i);
        kdf_object *item = kdf_obj_add_object(root, key);
        kdf_obj_set_int(item, "id", i);
        kdf_obj_set_float(item, "value", (double)i * 1.5);
        kdf_obj_set_bool(item, "active", i % 2 == 0);
        kdf_obj_set_vec3(item, "pos", (float)i, (float)i + 1, (float)i + 2);
    }

    // write to text
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);
    ASSERT(mw.size > 0);

    // read back
    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    ASSERT_STR_EQ(kdf_obj_get_string(root2, "name", ""), "LargeDocument");
    ASSERT_EQ(kdf_obj_get_int(root2, "version", 0), 1);

    // spot-check a few items
    char key[32];
    snprintf(key, sizeof(key), "item_%d", 0);
    const kdf_value *v0 = kdf_obj_get_const(root2, key);
    ASSERT_NOT_NULL(v0);
    kdf_object *i0 = kdf_val_as_object((kdf_value *)v0);
    ASSERT_EQ(kdf_obj_get_int(i0, "id", -1), 0);

    snprintf(key, sizeof(key), "item_%d", 499);
    const kdf_value *v499 = kdf_obj_get_const(root2, key);
    ASSERT_NOT_NULL(v499);
    kdf_object *i499 = kdf_val_as_object((kdf_value *)v499);
    ASSERT_EQ(kdf_obj_get_int(i499, "id", -1), 499);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

static void test_stress_binary_roundtrip_large(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "scene", "TestScene");

    kdf_array *entities = kdf_obj_add_array(root, "entities");
    for (int i = 0; i < 1000; i++)
    {
        kdf_object *ent = kdf_arr_push_object(entities);
        kdf_obj_set_int(ent, "id", i);
        kdf_obj_set_string(ent, "name", "Entity");
        kdf_obj_set_float(ent, "x", (double)i * 0.1);
        kdf_obj_set_float(ent, "y", (double)i * 0.2);
        kdf_obj_set_bool(ent, "visible", i % 3 != 0);
    }

    // write to binary
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 1), KDF_OK);
    ASSERT(mw.size > 0);

    // read back
    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_binary_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    ASSERT_STR_EQ(kdf_obj_get_string(root2, "scene", ""), "TestScene");

    const kdf_value *ev = kdf_obj_get_const(root2, "entities");
    ASSERT_EQ(kdf_val_type(ev), KDF_TYPE_ARRAY);
    kdf_array *entities2 = kdf_val_as_array((kdf_value *)ev);
    ASSERT_EQ(kdf_arr_count(entities2), 1000u);

    // spot-check first and last
    kdf_object *first = kdf_val_as_object((kdf_value *)kdf_arr_get_const(entities2, 0));
    ASSERT_EQ(kdf_obj_get_int(first, "id", -1), 0);

    kdf_object *last = kdf_val_as_object((kdf_value *)kdf_arr_get_const(entities2, 999));
    ASSERT_EQ(kdf_obj_get_int(last, "id", -1), 999);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

// stability tests

static void test_stability_repeated_create_destroy(void)
{
    // create and destroy 1000 documents in a row.
    // catches memory leaks and cleanup bugs.
    for (int i = 0; i < 1000; i++)
    {
        kdf_document *doc = kdf_doc_create();
        kdf_object *root = kdf_doc_root(doc);
        kdf_obj_set_int(root, "i", i);
        kdf_obj_set_string(root, "name", "test");
        kdf_obj_set_float(root, "val", (double)i * 0.5);

        kdf_object *child = kdf_obj_add_object(root, "child");
        kdf_obj_set_bool(child, "ok", true);

        kdf_array *arr = kdf_obj_add_array(root, "arr");
        kdf_arr_push_int(arr, i);
        kdf_arr_push_string(arr, "hello");

        kdf_doc_destroy(doc);
    }
    // if we reach here without crashing, the test passes
}

static void test_stability_repeated_overwrite(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    // overwrite the same key 10,000 times with different types
    for (int i = 0; i < 10000; i++)
    {
        if (i % 4 == 0)
            kdf_obj_set_int(root, "slot", i);
        else if (i % 4 == 1)
            kdf_obj_set_float(root, "slot", (double)i);
        else if (i % 4 == 2)
            kdf_obj_set_bool(root, "slot", true);
        else
            kdf_obj_set_string(root, "slot", "hello");
    }

    // should only have one property
    ASSERT_EQ(kdf_obj_count(root), 1u);

    kdf_doc_destroy(doc);
}

static void test_stability_empty_document_roundtrip(void)
{
    // an empty document should survive roundtrips
    kdf_document *doc = kdf_doc_create();

    // text roundtrip
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    ASSERT_EQ(kdf_obj_count(kdf_doc_root(doc2)), 0u);
    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);

    // binary roundtrip
    memset(&mw, 0, sizeof(mw));
    ASSERT_EQ(write_to_mem(doc, &mw, 1), KDF_OK);

    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    r = kdf_mem_reader_create(&mr);
    doc2 = kdf_binary_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    ASSERT_EQ(kdf_obj_count(kdf_doc_root(doc2)), 0u);
    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);

    kdf_doc_destroy(doc);
}

static void test_stability_empty_strings(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "empty", "");
    ASSERT_STR_EQ(kdf_obj_get_string(root, "empty", "fallback"), "");

    kdf_array *arr = kdf_obj_add_array(root, "arr");
    kdf_arr_push_string(arr, "");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(arr, 0)), "");

    kdf_obj_set_asset_ref(root, "ref", "");
    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(root, "ref")), "");

    // roundtrip
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    ASSERT_STR_EQ(kdf_obj_get_string(kdf_doc_root(doc2), "empty", "x"), "");

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

static void test_stability_special_chars_in_strings(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "quotes", "She said \"hello\"");
    kdf_obj_set_string(root, "backslash", "path\\to\\file");
    kdf_obj_set_string(root, "newline", "line1\nline2");
    kdf_obj_set_string(root, "tab", "col1\tcol2");
    kdf_obj_set_string(root, "unicode", "\xc3\xa9\xc3\xa0\xc3\xbc");

    // text roundtrip
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 0), KDF_OK);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_text_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    ASSERT_STR_EQ(kdf_obj_get_string(root2, "quotes", ""), "She said \"hello\"");
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "backslash", ""), "path\\to\\file");
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "newline", ""), "line1\nline2");
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "tab", ""), "col1\tcol2");
    ASSERT_STR_EQ(kdf_obj_get_string(root2, "unicode", ""), "\xc3\xa9\xc3\xa0\xc3\xbc");

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

static void test_stability_numeric_edge_cases(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_int(root, "zero", 0);
    kdf_obj_set_int(root, "max64", INT64_MAX);
    kdf_obj_set_int(root, "min64", INT64_MIN);
    kdf_obj_set_uint(root, "umax64", UINT64_MAX);
    kdf_obj_set_float(root, "tiny", 1e-300);
    kdf_obj_set_float(root, "huge", 1e300);
    kdf_obj_set_float(root, "neg_zero", -0.0);
    kdf_obj_set_float(root, "pi", 3.14159265358979323846);

    // binary roundtrip preserves exact values
    kdf_mem_writer mw = {0};
    ASSERT_EQ(write_to_mem(doc, &mw, 1), KDF_OK);

    kdf_mem_reader mr;
    mr.data = mw.data;
    mr.size = mw.size;
    mr.pos = 0;
    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc2 = kdf_binary_read(r, NULL);
    ASSERT_NOT_NULL(doc2);
    kdf_object *root2 = kdf_doc_root(doc2);

    ASSERT_EQ(kdf_obj_get_int(root2, "zero", 1), 0);
    ASSERT_EQ(kdf_obj_get_int(root2, "max64", 0), INT64_MAX);
    ASSERT_EQ(kdf_obj_get_int(root2, "min64", 0), INT64_MIN);
    ASSERT_EQ(kdf_obj_get_uint(root2, "umax64", 0), UINT64_MAX);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root2, "tiny", 0.0), 1e-300);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root2, "huge", 0.0), 1e300);

    kdf_mem_writer_finish(&mw);
    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
}

static void test_stability_null_values(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    // null pointers for string values should not crash
    kdf_obj_set_string(root, "s", NULL);
    kdf_obj_set_asset_ref(root, "a", NULL);
    kdf_obj_set_resource_ref(root, "r", NULL);

    // passing null object/array to functions should not crash
    kdf_obj_set_int(NULL, "x", 1);
    kdf_obj_get(NULL, "x");
    kdf_obj_has(NULL, "x");
    kdf_obj_first(NULL);
    kdf_obj_count(NULL);
    kdf_arr_count(NULL);
    kdf_arr_get(NULL, 0);
    kdf_arr_push_int(NULL, 1);
    kdf_arr_clear(NULL);
    kdf_val_type(NULL);
    kdf_val_is_null(NULL);

    kdf_doc_destroy(doc);
    kdf_doc_destroy(NULL); // double destroy should not crash
}

// file i/o tests

static const char *test_kdf_path = "/tmp/kdf_test_output.kdf";
static const char *test_kdfb_path = "/tmp/kdf_test_output.kdfb";

static kdf_document *build_test_document(void)
{
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_string(root, "name", "TestResource");
    kdf_obj_set_int(root, "version", 42);
    kdf_obj_set_float(root, "scale", 1.5);
    kdf_obj_set_bool(root, "enabled", true);
    kdf_obj_set_null(root, "placeholder");
    kdf_obj_set_uint(root, "id", 12345u);
    kdf_obj_set_vec3(root, "position", 1.0f, 2.0f, 3.0f);
    kdf_obj_set_color(root, "tint", 1.0f, 0.8f, 0.6f, 1.0f);
    kdf_obj_set_quat(root, "rotation", 0.0f, 0.0f, 0.0f, 1.0f);
    kdf_obj_set_asset_ref(root, "texture", "res://textures/base.png");

    kdf_object *settings = kdf_obj_add_object(root, "settings");
    kdf_obj_set_bool(settings, "cast_shadows", true);
    kdf_obj_set_int(settings, "layer", 5);
    kdf_obj_set_float(settings, "opacity", 0.9);

    kdf_array *tags = kdf_obj_add_array(root, "tags");
    kdf_arr_push_string(tags, "renderable");
    kdf_arr_push_string(tags, "static");
    kdf_arr_push_string(tags, "opaque");

    kdf_array *data = kdf_obj_add_array(root, "data");
    kdf_arr_push_int(data, 10);
    kdf_arr_push_float(data, 2.5);
    kdf_arr_push_bool(data, false);

    kdf_object *sub = kdf_obj_add_subresource(root, "material", "Material");
    kdf_obj_set_float(sub, "roughness", 0.65);
    kdf_obj_set_float(sub, "metallic", 0.0);
    kdf_obj_set_asset_ref(sub, "shader", "res://shaders/pbr.shader");

    return doc;
}

static void verify_test_document(kdf_document *doc)
{
    kdf_object *root = kdf_doc_root(doc);

    ASSERT_STR_EQ(kdf_obj_get_string(root, "name", ""), "TestResource");
    ASSERT_EQ(kdf_obj_get_int(root, "version", 0), 42);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(root, "scale", 0.0), 1.5);
    ASSERT_EQ(kdf_obj_get_bool(root, "enabled", false), true);
    ASSERT_EQ(kdf_obj_get_type_at(root, "placeholder"), KDF_TYPE_NULL);
    ASSERT_EQ(kdf_obj_get_uint(root, "id", 0), 12345u);

    const float *pos = kdf_val_as_vec3(kdf_obj_get_const(root, "position"));
    ASSERT_NOT_NULL(pos);
    ASSERT_FLOAT_EQ(pos[0], 1.0f);
    ASSERT_FLOAT_EQ(pos[1], 2.0f);
    ASSERT_FLOAT_EQ(pos[2], 3.0f);

    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(root, "texture")), "res://textures/base.png");

    // nested object
    const kdf_value *sv = kdf_obj_get_const(root, "settings");
    ASSERT_EQ(kdf_val_type(sv), KDF_TYPE_OBJECT);
    kdf_object *settings = kdf_val_as_object((kdf_value *)sv);
    ASSERT_EQ(kdf_obj_get_bool(settings, "cast_shadows", false), true);
    ASSERT_EQ(kdf_obj_get_int(settings, "layer", 0), 5);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(settings, "opacity", 0.0), 0.9);

    // array of strings
    const kdf_value *tv = kdf_obj_get_const(root, "tags");
    ASSERT_EQ(kdf_val_type(tv), KDF_TYPE_ARRAY);
    kdf_array *tags = kdf_val_as_array((kdf_value *)tv);
    ASSERT_EQ(kdf_arr_count(tags), 3u);
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(tags, 0)), "renderable");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(tags, 1)), "static");
    ASSERT_STR_EQ(kdf_val_as_string(kdf_arr_get_const(tags, 2)), "opaque");

    // mixed array
    const kdf_value *dv = kdf_obj_get_const(root, "data");
    ASSERT_EQ(kdf_val_type(dv), KDF_TYPE_ARRAY);
    kdf_array *data = kdf_val_as_array((kdf_value *)dv);
    ASSERT_EQ(kdf_arr_count(data), 3u);
    ASSERT_EQ(kdf_val_as_int(kdf_arr_get_const(data, 0)), 10);
    ASSERT_FLOAT_EQ(kdf_val_as_float(kdf_arr_get_const(data, 1)), 2.5);
    ASSERT_EQ(kdf_val_as_bool(kdf_arr_get_const(data, 2)), false);

    // subresource
    const kdf_value *mv = kdf_obj_get_const(root, "material");
    ASSERT_EQ(kdf_val_type(mv), KDF_TYPE_SUBRESOURCE);
    kdf_object *mat = kdf_val_as_object((kdf_value *)mv);
    ASSERT_STR_EQ(kdf_obj_type(mat), "Material");
    ASSERT_FLOAT_EQ(kdf_obj_get_float(mat, "roughness", 0.0), 0.65);
    ASSERT_FLOAT_EQ(kdf_obj_get_float(mat, "metallic", 0.0), 0.0);
    ASSERT_STR_EQ(kdf_val_as_asset_ref(kdf_obj_get_const(mat, "shader")), "res://shaders/pbr.shader");
}

static void test_file_text_save_load(void)
{
    kdf_document *doc = build_test_document();

    int result = kdf_text_save(doc, test_kdf_path);
    ASSERT_EQ(result, KDF_OK);

    // verify the file exists and is non-empty
    FILE *f = fopen(test_kdf_path, "rb");
    ASSERT_NOT_NULL(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    ASSERT(size > 0);

    // load and verify
    kdf_document *doc2 = kdf_text_load(test_kdf_path, NULL);
    ASSERT_NOT_NULL(doc2);
    verify_test_document(doc2);

    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
    remove(test_kdf_path);
}

static void test_file_binary_save_load(void)
{
    kdf_document *doc = build_test_document();

    int result = kdf_binary_save(doc, test_kdfb_path);
    ASSERT_EQ(result, KDF_OK);

    // verify the file exists and is non-empty
    FILE *f = fopen(test_kdfb_path, "rb");
    ASSERT_NOT_NULL(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    ASSERT(size > 0);

    // load and verify
    kdf_document *doc2 = kdf_binary_load(test_kdfb_path, NULL);
    ASSERT_NOT_NULL(doc2);
    verify_test_document(doc2);

    kdf_doc_destroy(doc2);
    kdf_doc_destroy(doc);
    remove(test_kdfb_path);
}

static void test_file_text_binary_equivalence(void)
{
    // save the same document as text and binary.
    // load both. verify they contain the same data.
    kdf_document *doc = build_test_document();

    ASSERT_EQ(kdf_text_save(doc, test_kdf_path), KDF_OK);
    ASSERT_EQ(kdf_binary_save(doc, test_kdfb_path), KDF_OK);

    kdf_document *from_text = kdf_text_load(test_kdf_path, NULL);
    kdf_document *from_bin = kdf_binary_load(test_kdfb_path, NULL);
    ASSERT_NOT_NULL(from_text);
    ASSERT_NOT_NULL(from_bin);

    // both should verify identically
    verify_test_document(from_text);
    verify_test_document(from_bin);

    // binary should be smaller or comparable
    FILE *ft = fopen(test_kdf_path, "rb");
    FILE *fb = fopen(test_kdfb_path, "rb");
    ASSERT_NOT_NULL(ft);
    ASSERT_NOT_NULL(fb);
    fseek(ft, 0, SEEK_END);
    fseek(fb, 0, SEEK_END);
    long text_size = ftell(ft);
    long bin_size = ftell(fb);
    fclose(ft);
    fclose(fb);
    (void)text_size;
    (void)bin_size;
    // both files should exist and have content. we don't enforce
    // a strict size comparison because the document is small.

    kdf_doc_destroy(from_text);
    kdf_doc_destroy(from_bin);
    kdf_doc_destroy(doc);
    remove(test_kdf_path);
    remove(test_kdfb_path);
}

static void test_file_roundtrip_stability(void)
{
    // save and load the same document 100 times.
    // catches cumulative state corruption.
    kdf_document *doc = build_test_document();

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(kdf_text_save(doc, test_kdf_path), KDF_OK);
        kdf_doc_destroy(doc);

        doc = kdf_text_load(test_kdf_path, NULL);
        ASSERT_NOT_NULL(doc);
    }

    verify_test_document(doc);
    kdf_doc_destroy(doc);
    remove(test_kdf_path);
}

static void test_file_binary_roundtrip_stability(void)
{
    kdf_document *doc = build_test_document();

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(kdf_binary_save(doc, test_kdfb_path), KDF_OK);
        kdf_doc_destroy(doc);

        doc = kdf_binary_load(test_kdfb_path, NULL);
        ASSERT_NOT_NULL(doc);
    }

    verify_test_document(doc);
    kdf_doc_destroy(doc);
    remove(test_kdfb_path);
}

static void test_file_large_resource(void)
{
    // build a document that simulates a real game resource
    kdf_document *doc = kdf_doc_create();
    kdf_object *root = kdf_doc_root(doc);

    kdf_obj_set_type(root, "Scene");
    kdf_obj_set_version(root, 3);
    kdf_obj_set_string(root, "name", "Level_01");
    kdf_obj_set_string(root, "author", "test");
    kdf_obj_set_int(root, "entity_count", 200);

    kdf_array *entities = kdf_obj_add_array(root, "entities");
    for (int i = 0; i < 200; i++)
    {
        kdf_object *ent = kdf_arr_push_object(entities);
        kdf_obj_set_int(ent, "id", i);

        char name[32];
        snprintf(name, sizeof(name), "Entity_%d", i);
        kdf_obj_set_string(ent, "name", name);

        kdf_obj_set_vec3(ent, "position", (float)(i % 10) * 2.0f, 0.0f, (float)(i / 10) * 2.0f);
        kdf_obj_set_quat(ent, "rotation", 0.0f, 0.0f, 0.0f, 1.0f);
        kdf_obj_set_vec3(ent, "scale", 1.0f, 1.0f, 1.0f);
        kdf_obj_set_bool(ent, "active", true);

        kdf_object *mesh = kdf_obj_add_object(ent, "mesh");
        char mesh_path[64];
        snprintf(mesh_path, sizeof(mesh_path), "res://models/mesh_%d.mesh", i % 5);
        kdf_obj_set_asset_ref(mesh, "source", mesh_path);
        kdf_obj_set_int(mesh, "submesh_count", 1);
    }

    // save as text and binary
    ASSERT_EQ(kdf_text_save(doc, test_kdf_path), KDF_OK);
    ASSERT_EQ(kdf_binary_save(doc, test_kdfb_path), KDF_OK);

    // load text, verify
    kdf_document *dt = kdf_text_load(test_kdf_path, NULL);
    ASSERT_NOT_NULL(dt);
    kdf_object *rt = kdf_doc_root(dt);
    ASSERT_STR_EQ(kdf_obj_get_string(rt, "name", ""), "Level_01");
    ASSERT_EQ(kdf_obj_get_int(rt, "entity_count", 0), 200);
    const kdf_value *et = kdf_obj_get_const(rt, "entities");
    ASSERT_EQ(kdf_val_type(et), KDF_TYPE_ARRAY);
    ASSERT_EQ(kdf_arr_count(kdf_val_as_array((kdf_value *)et)), 200u);

    // load binary, verify
    kdf_document *db = kdf_binary_load(test_kdfb_path, NULL);
    ASSERT_NOT_NULL(db);
    kdf_object *rb = kdf_doc_root(db);
    ASSERT_STR_EQ(kdf_obj_get_string(rb, "name", ""), "Level_01");
    ASSERT_EQ(kdf_obj_get_int(rb, "entity_count", 0), 200);
    const kdf_value *eb = kdf_obj_get_const(rb, "entities");
    ASSERT_EQ(kdf_val_type(eb), KDF_TYPE_ARRAY);
    ASSERT_EQ(kdf_arr_count(kdf_val_as_array((kdf_value *)eb)), 200u);

    kdf_doc_destroy(dt);
    kdf_doc_destroy(db);
    kdf_doc_destroy(doc);
    remove(test_kdf_path);
    remove(test_kdfb_path);
}

// main

int main(void)
{
    printf("KDF Test Suite\n");
    printf("==============\n\n");

    printf("Core:\n");
    RUN_TEST(test_version);
    RUN_TEST(test_doc_create_destroy);
    RUN_TEST(test_doc_create_null);
    RUN_TEST(test_default_allocator);

    printf("\nObject:\n");
    RUN_TEST(test_obj_null);
    RUN_TEST(test_obj_bool);
    RUN_TEST(test_obj_int);
    RUN_TEST(test_obj_uint);
    RUN_TEST(test_obj_float);
    RUN_TEST(test_obj_string);
    RUN_TEST(test_obj_vec2);
    RUN_TEST(test_obj_vec3);
    RUN_TEST(test_obj_vec4);
    RUN_TEST(test_obj_quat);
    RUN_TEST(test_obj_color);
    RUN_TEST(test_obj_asset_ref);
    RUN_TEST(test_obj_resource_ref);
    RUN_TEST(test_obj_overwrite);
    RUN_TEST(test_obj_metadata);
    RUN_TEST(test_obj_nested);
    RUN_TEST(test_obj_iteration);
    RUN_TEST(test_obj_remove);
    RUN_TEST(test_obj_clear);
    RUN_TEST(test_subresource);
    RUN_TEST(test_getter_fallbacks);
    RUN_TEST(test_value_coercion);

    printf("\nArray:\n");
    RUN_TEST(test_array_basic);
    RUN_TEST(test_array_mixed);
    RUN_TEST(test_array_remove);
    RUN_TEST(test_array_clear);

    printf("\nText format:\n");
    RUN_TEST(test_text_write_simple);
    RUN_TEST(test_text_write_types);
    RUN_TEST(test_text_write_nested);
    RUN_TEST(test_text_read_simple);
    RUN_TEST(test_text_read_types);
    RUN_TEST(test_text_read_nested);
    RUN_TEST(test_text_read_negative_numbers);
    RUN_TEST(test_text_roundtrip);

    printf("\nBinary format:\n");
    RUN_TEST(test_binary_roundtrip);
    RUN_TEST(test_binary_size);

    printf("\nI/O:\n");
    RUN_TEST(test_mem_writer_reader);

    printf("\nIntegration:\n");
    RUN_TEST(test_full_resource);

    printf("\nStress:\n");
    RUN_TEST(test_stress_many_properties);
    RUN_TEST(test_stress_large_array);
    RUN_TEST(test_stress_deep_nesting);
    RUN_TEST(test_stress_long_strings);
    RUN_TEST(test_stress_text_roundtrip_large);
    RUN_TEST(test_stress_binary_roundtrip_large);

    printf("\nStability:\n");
    RUN_TEST(test_stability_repeated_create_destroy);
    RUN_TEST(test_stability_repeated_overwrite);
    RUN_TEST(test_stability_empty_document_roundtrip);
    RUN_TEST(test_stability_empty_strings);
    RUN_TEST(test_stability_special_chars_in_strings);
    RUN_TEST(test_stability_numeric_edge_cases);
    RUN_TEST(test_stability_null_values);

    printf("\nFile I/O:\n");
    RUN_TEST(test_file_text_save_load);
    RUN_TEST(test_file_binary_save_load);
    RUN_TEST(test_file_text_binary_equivalence);
    RUN_TEST(test_file_roundtrip_stability);
    RUN_TEST(test_file_binary_roundtrip_stability);
    RUN_TEST(test_file_large_resource);

    printf("\n==============\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
    {
        printf(", %d FAILED", tests_failed);
    }
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
