#include "kdf_internal.h"

// text writer state

typedef struct
{
    kdf_writer writer;
    int indent;
    int error;
} tw;

static void tw_write(tw *w, const char *data, size_t len)
{
    if (w->error)
        return;
    if (w->writer.write(w->writer.userdata, data, len) != len)
    {
        w->error = KDF_ERROR_IO;
    }
}

static void tw_str(tw *w, const char *s)
{
    tw_write(w, s, strlen(s));
}

static void tw_char(tw *w, char c)
{
    tw_write(w, &c, 1);
}

static void tw_indent(tw *w)
{
    for (int i = 0; i < w->indent; i++)
    {
        tw_str(w, "    ");
    }
}

static void tw_newline(tw *w)
{
    tw_char(w, '\n');
}

static void tw_int64(tw *w, int64_t v)
{
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%lld", (long long)v);
    tw_write(w, buf, (size_t)n);
}

static void tw_uint64(tw *w, uint64_t v)
{
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    tw_write(w, buf, (size_t)n);
}

static void tw_float(tw *w, double v)
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%.17g", v);
    tw_write(w, buf, (size_t)n);
}

static void tw_quoted(tw *w, const char *s)
{
    tw_char(w, '"');
    if (s)
    {
        for (const char *p = s; *p; p++)
        {
            switch (*p)
            {
            case '"':
                tw_str(w, "\\\"");
                break;
            case '\\':
                tw_str(w, "\\\\");
                break;
            case '\n':
                tw_str(w, "\\n");
                break;
            case '\r':
                tw_str(w, "\\r");
                break;
            case '\t':
                tw_str(w, "\\t");
                break;
            default:
                tw_char(w, *p);
                break;
            }
        }
    }
    tw_char(w, '"');
}

// forward declarations

static void tw_write_value(tw *w, const kdf_value *val);
static void tw_write_object_body(tw *w, const kdf_object *obj);

// value writer

static void tw_write_float_list4(tw *w, const char *prefix, const float *v, int n)
{
    tw_str(w, prefix);
    tw_char(w, '(');
    for (int i = 0; i < n; i++)
    {
        if (i > 0)
            tw_str(w, ", ");
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%.6g", v[i]);
        tw_write(w, buf, (size_t)len);
    }
    tw_char(w, ')');
}

static void tw_write_value(tw *w, const kdf_value *val)
{
    if (!val)
    {
        tw_str(w, "null");
        return;
    }
    switch (val->type)
    {
    case KDF_TYPE_NULL:
        tw_str(w, "null");
        break;
    case KDF_TYPE_BOOL:
        tw_str(w, val->as.boolean ? "true" : "false");
        break;
    case KDF_TYPE_INT:
        tw_int64(w, val->as.i64);
        break;
    case KDF_TYPE_UINT:
        tw_uint64(w, val->as.u64);
        tw_char(w, 'u');
        break;
    case KDF_TYPE_FLOAT:
        tw_float(w, val->as.f64);
        break;
    case KDF_TYPE_STRING:
        tw_quoted(w, val->as.str ? val->as.str->data : "");
        break;
    case KDF_TYPE_VEC2:
        tw_write_float_list4(w, "vec2", val->as.vec2, 2);
        break;
    case KDF_TYPE_VEC3:
        tw_write_float_list4(w, "vec3", val->as.vec3, 3);
        break;
    case KDF_TYPE_VEC4:
        tw_write_float_list4(w, "vec4", val->as.vec4, 4);
        break;
    case KDF_TYPE_QUAT:
        tw_write_float_list4(w, "quat", val->as.vec4, 4);
        break;
    case KDF_TYPE_COLOR:
        tw_write_float_list4(w, "color", val->as.vec4, 4);
        break;
    case KDF_TYPE_ASSET_REF:
        tw_str(w, "asset(");
        tw_quoted(w, val->as.str ? val->as.str->data : "");
        tw_char(w, ')');
        break;
    case KDF_TYPE_RESOURCE_REF:
        tw_str(w, "external(");
        tw_quoted(w, val->as.str ? val->as.str->data : "");
        tw_char(w, ')');
        break;
    case KDF_TYPE_ARRAY:
    {
        kdf_array *arr = val->as.array;
        if (!arr || kdf_arr_count(arr) == 0)
        {
            tw_str(w, "[]");
            break;
        }
        tw_char(w, '[');
        tw_newline(w);
        w->indent++;
        for (size_t i = 0; i < arr->count; i++)
        {
            tw_indent(w);
            tw_write_value(w, &arr->items[i]);
            if (i + 1 < arr->count)
                tw_char(w, ',');
            tw_newline(w);
        }
        w->indent--;
        tw_indent(w);
        tw_char(w, ']');
        break;
    }
    case KDF_TYPE_OBJECT:
    case KDF_TYPE_SUBRESOURCE:
    {
        kdf_object *obj = val->as.object;
        if (!obj)
        {
            tw_str(w, "{}");
            break;
        }
        if (obj->type_name)
        {
            tw_str(w, "subresource ");
            tw_str(w, obj->type_name->data);
            tw_char(w, ' ');
        }
        tw_char(w, '{');
        tw_newline(w);
        w->indent++;
        tw_write_object_body(w, obj);
        w->indent--;
        tw_indent(w);
        tw_char(w, '}');
        break;
    }
    }
}

// object body writer

static void tw_write_object_body(tw *w, const kdf_object *obj)
{
    for (const kdf_entry *e = kdf_obj_first(obj); e; e = kdf_obj_next(e))
    {
        const kdf_value *val = kdf_entry_value(e);
        tw_indent(w);
        tw_str(w, kdf_entry_key(e));
        // use shorthand for nested objects without type: key { ... }
        if (val->type == KDF_TYPE_OBJECT && val->as.object && !val->as.object->type_name)
        {
            tw_char(w, ' ');
            tw_char(w, '{');
            tw_newline(w);
            w->indent++;
            tw_write_object_body(w, val->as.object);
            w->indent--;
            tw_indent(w);
            tw_char(w, '}');
        }
        else
        {
            tw_str(w, " = ");
            tw_write_value(w, val);
        }
        tw_newline(w);
    }
}

// public api

int kdf_text_write(const kdf_document *doc, kdf_writer writer)
{
    if (!doc)
        return KDF_ERROR_INVALID_ARGUMENT;

    tw w;
    w.writer = writer;
    w.indent = 0;
    w.error = 0;

    // header
    tw_str(&w, "kdf 1");
    tw_newline(&w);

    // root object body
    const kdf_object *root = kdf_doc_root((kdf_document *)doc);
    if (kdf_obj_count(root) > 0)
    {
        tw_newline(&w);
        tw_write_object_body(&w, root);
    }

    return w.error;
}

int kdf_text_save(const kdf_document *doc, const char *path)
{
    if (!doc || !path)
        return KDF_ERROR_INVALID_ARGUMENT;

    FILE *f = fopen(path, "wb");
    if (!f)
        return KDF_ERROR_IO;

    kdf_mem_writer mw = {0};
    kdf_writer w = kdf_mem_writer_create(&mw);

    int result = kdf_text_write(doc, w);

    if (result == KDF_OK && mw.size > 0)
    {
        if (fwrite(mw.data, 1, mw.size, f) != mw.size)
        {
            result = KDF_ERROR_IO;
        }
    }

    kdf_mem_writer_finish(&mw);
    fclose(f);
    return result;
}
