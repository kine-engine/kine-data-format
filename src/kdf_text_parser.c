#include "kdf_internal.h"

// parser state

typedef struct
{
    kdf_reader reader;
    char buf[65536];
    size_t buf_len;
    size_t buf_pos;
    int line;
    int col;
    char cur; // current character, 0 = eof
    int error;
    kdf_document *doc;
    kdf_allocator *alloc;
    kdf_string_pool *strings;
    char *str_buf; // dynamic buffer for string reading
    size_t str_buf_cap;
} parser;

static void parser_advance(parser *p)
{
    if (p->buf_pos >= p->buf_len)
    {
        p->buf_len = p->reader.read(p->reader.userdata, p->buf, sizeof(p->buf));
        p->buf_pos = 0;
        if (p->buf_len == 0)
        {
            p->cur = 0;
            return;
        }
    }
    p->cur = p->buf[p->buf_pos++];
    if (p->cur == '\n')
    {
        p->line++;
        p->col = 0;
    }
    else
    {
        p->col++;
    }
}

static void parser_skip_spaces(parser *p)
{
    while (p->cur == ' ' || p->cur == '\t' || p->cur == '\r')
    {
        parser_advance(p);
    }
}

static void parser_skip_line(parser *p)
{
    while (p->cur && p->cur != '\n')
    {
        parser_advance(p);
    }
    if (p->cur == '\n')
        parser_advance(p);
}

static void parser_skip_blank_lines(parser *p)
{
    while (p->cur)
    {
        parser_skip_spaces(p);
        if (p->cur == '\n')
        {
            parser_advance(p);
        }
        else if (p->cur == '#')
        {
            parser_skip_line(p);
        }
        else
        {
            break;
        }
    }
}

static bool parser_try_keyword(parser *p, const char *kw)
{
    // check if current position starts with keyword kw followed by non-alnum
    size_t kw_len = strlen(kw);
    char lookahead[64];
    size_t have = 0;

    // first character is p->cur
    if (p->cur)
    {
        lookahead[have++] = p->cur;
    }

    // save current buffer position
    size_t saved_buf_pos = p->buf_pos;
    int saved_line = p->line;
    int saved_col = p->col;
    size_t tmp_pos = p->buf_pos;

    // read more characters from buffer
    while (have < kw_len + 1)
    {
        if (tmp_pos >= p->buf_len)
        {
            break;
        }
        lookahead[have++] = p->buf[tmp_pos++];
    }

    if (have < kw_len)
        return false;
    if (memcmp(lookahead, kw, kw_len) != 0)
        return false;
    // check that next char is not alnum (it's a full word match)
    if (have > kw_len)
    {
        char next = lookahead[kw_len];
        if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
        {
            return false;
        }
    }

    // restore position, then consume the keyword
    p->buf_pos = saved_buf_pos;
    p->line = saved_line;
    p->col = saved_col;

    for (size_t i = 0; i < kw_len; i++)
    {
        parser_advance(p);
    }
    return true;
}

static int parser_expect_char(parser *p, char c)
{
    parser_skip_spaces(p);
    if (p->cur != c)
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p);
    return KDF_OK;
}

// identifier parsing

static int parser_read_ident(parser *p, char *out, size_t out_size)
{
    size_t i = 0;
    if (!((p->cur >= 'a' && p->cur <= 'z') || (p->cur >= 'A' && p->cur <= 'Z') || p->cur == '_'))
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    while (((p->cur >= 'a' && p->cur <= 'z') || (p->cur >= 'A' && p->cur <= 'Z') || (p->cur >= '0' && p->cur <= '9') ||
            p->cur == '_') &&
           i + 1 < out_size)
    {
        out[i++] = p->cur;
        parser_advance(p);
    }
    out[i] = '\0';
    return KDF_OK;
}

// string parsing

static int parser_ensure_str_buf(parser *p, size_t needed)
{
    if (p->str_buf_cap >= needed)
        return KDF_OK;
    size_t new_cap = p->str_buf_cap == 0 ? 4096 : p->str_buf_cap;
    while (new_cap < needed)
        new_cap *= 2;
    char *new_buf = (char *)kdf__realloc(p->alloc, p->str_buf, p->str_buf_cap, new_cap);
    if (!new_buf)
    {
        p->error = KDF_ERROR_OUT_OF_MEMORY;
        return KDF_ERROR_OUT_OF_MEMORY;
    }
    p->str_buf = new_buf;
    p->str_buf_cap = new_cap;
    return KDF_OK;
}

static int parser_read_quoted_string(parser *p, char *out, size_t out_size)
{
    if (p->cur != '"')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p); // skip opening quote

    // use dynamic buffer for potentially long strings
    size_t cap = out_size > 0 ? out_size : 4096;
    if (parser_ensure_str_buf(p, cap) != KDF_OK)
        return p->error;

    size_t i = 0;
    while (p->cur && p->cur != '"')
    {
        char c = p->cur;
        if (c == '\\')
        {
            parser_advance(p);
            switch (p->cur)
            {
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case '\\':
                c = '\\';
                break;
            case '"':
                c = '"';
                break;
            default:
                c = p->cur;
                break;
            }
        }
        if (i + 1 >= p->str_buf_cap)
        {
            if (parser_ensure_str_buf(p, p->str_buf_cap * 2) != KDF_OK)
                return p->error;
        }
        p->str_buf[i++] = c;
        parser_advance(p);
    }
    if (p->cur != '"')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p); // skip closing quote
    p->str_buf[i] = '\0';

    // copy to output if provided, otherwise use str_buf
    if (out && out_size > 0)
    {
        size_t copy_len = i < out_size - 1 ? i : out_size - 1;
        memcpy(out, p->str_buf, copy_len);
        out[copy_len] = '\0';
    }
    return KDF_OK;
}

// number parsing

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int parser_read_number(parser *p, kdf_value *out)
{
    bool negative = false;
    if (p->cur == '-')
    {
        negative = true;
        parser_advance(p);
    }

    if (!is_digit(p->cur))
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }

    int64_t int_val = 0;
    while (is_digit(p->cur))
    {
        int_val = int_val * 10 + (p->cur - '0');
        parser_advance(p);
    }

    // check for unsigned suffix
    if (p->cur == 'u' || p->cur == 'U')
    {
        parser_advance(p);
        out->type = KDF_TYPE_UINT;
        out->as.u64 = negative ? (uint64_t)(-int_val) : (uint64_t)int_val;
        return KDF_OK;
    }

    // check for float (decimal point)
    if (p->cur == '.')
    {
        parser_advance(p);
        double frac = 0.0;
        double div = 10.0;
        while (is_digit(p->cur))
        {
            frac += (p->cur - '0') / div;
            div *= 10.0;
            parser_advance(p);
        }
        double float_val = (double)int_val + frac;
        if (negative)
            float_val = -float_val;

        // check for exponent
        if (p->cur == 'e' || p->cur == 'E')
        {
            parser_advance(p);
            bool exp_neg = false;
            if (p->cur == '-')
            {
                exp_neg = true;
                parser_advance(p);
            }
            else if (p->cur == '+')
            {
                parser_advance(p);
            }
            int64_t exp = 0;
            while (is_digit(p->cur))
            {
                exp = exp * 10 + (p->cur - '0');
                parser_advance(p);
            }
            double mul = 1.0;
            for (int64_t i = 0; i < exp; i++)
                mul *= 10.0;
            if (exp_neg)
                float_val /= mul;
            else
                float_val *= mul;
        }

        out->type = KDF_TYPE_FLOAT;
        out->as.f64 = float_val;
        return KDF_OK;
    }

    out->type = KDF_TYPE_INT;
    out->as.i64 = negative ? -int_val : int_val;
    return KDF_OK;
}

// constructor parsing (vec2, vec3, etc.)

static int parser_read_float_list(parser *p, float *out, int count)
{
    // current char should be '('
    if (p->cur != '(')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p);
    parser_skip_spaces(p);

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            if (p->cur != ',')
            {
                p->error = KDF_ERROR_PARSE;
                return KDF_ERROR_PARSE;
            }
            parser_advance(p);
            parser_skip_spaces(p);
        }
        // read a float value
        kdf_value num;
        memset(&num, 0, sizeof(num));
        if (parser_read_number(p, &num) != KDF_OK)
            return p->error;
        out[i] = (float)kdf_val_as_float(&num);
        parser_skip_spaces(p);
    }

    if (p->cur != ')')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p);
    return KDF_OK;
}

// value parsing

static int parser_parse_value(parser *p, kdf_value *out);
static int parser_parse_object_body(parser *p, kdf_object *obj);

static int parser_parse_array(parser *p, kdf_array *arr)
{
    // current char is '['
    parser_advance(p); // skip '['
    parser_skip_spaces(p);

    if (p->cur == ']')
    {
        parser_advance(p);
        return KDF_OK;
    }

    for (;;)
    {
        parser_skip_blank_lines(p);
        if (p->cur == ']')
        {
            parser_advance(p);
            return KDF_OK;
        }

        // parse value and push it
        kdf_value val;
        memset(&val, 0, sizeof(val));
        if (parser_parse_value(p, &val) != KDF_OK)
            return p->error;

        // push the value onto the array
        switch (val.type)
        {
        case KDF_TYPE_NULL:
            kdf_arr_push_null(arr);
            break;
        case KDF_TYPE_BOOL:
            kdf_arr_push_bool(arr, val.as.boolean);
            break;
        case KDF_TYPE_INT:
            kdf_arr_push_int(arr, val.as.i64);
            break;
        case KDF_TYPE_UINT:
            kdf_arr_push_uint(arr, val.as.u64);
            break;
        case KDF_TYPE_FLOAT:
            kdf_arr_push_float(arr, val.as.f64);
            break;
        case KDF_TYPE_STRING:
            kdf_arr_push_string(arr, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_VEC2:
            kdf_arr_push_vec2(arr, val.as.vec2[0], val.as.vec2[1]);
            break;
        case KDF_TYPE_VEC3:
            kdf_arr_push_vec3(arr, val.as.vec3[0], val.as.vec3[1], val.as.vec3[2]);
            break;
        case KDF_TYPE_VEC4:
            kdf_arr_push_vec4(arr, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_QUAT:
            kdf_arr_push_quat(arr, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_COLOR:
            kdf_arr_push_color(arr, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_ASSET_REF:
            kdf_arr_push_asset_ref(arr, val.as.str ? val.as.str->data : "");
            break;
        default:
            // objects, arrays, subresources: transfer ownership
            kdf__arr_push_value(arr, &val);
            break;
        }

        parser_skip_spaces(p);
        if (p->cur == ',')
        {
            parser_advance(p);
        }
    }
}

static int parser_parse_block(parser *p, kdf_object *obj)
{
    if (p->cur != '{')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p); // skip '{'
    parser_skip_blank_lines(p);

    int result = parser_parse_object_body(p, obj);
    if (result != KDF_OK)
        return result;

    parser_skip_spaces(p);
    if (p->cur != '}')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_advance(p);
    return KDF_OK;
}

static int parser_parse_value(parser *p, kdf_value *out)
{
    parser_skip_spaces(p);

    if (p->cur == 0 || p->cur == '\n')
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }

    // null
    if (parser_try_keyword(p, "null"))
    {
        out->type = KDF_TYPE_NULL;
        return KDF_OK;
    }

    // true
    if (parser_try_keyword(p, "true"))
    {
        out->type = KDF_TYPE_BOOL;
        out->as.boolean = true;
        return KDF_OK;
    }

    // false
    if (parser_try_keyword(p, "false"))
    {
        out->type = KDF_TYPE_BOOL;
        out->as.boolean = false;
        return KDF_OK;
    }

    // vec2/vec3/vec4/quat/color constructors
    if (parser_try_keyword(p, "vec2"))
    {
        out->type = KDF_TYPE_VEC2;
        return parser_read_float_list(p, out->as.vec2, 2);
    }
    if (parser_try_keyword(p, "vec3"))
    {
        out->type = KDF_TYPE_VEC3;
        return parser_read_float_list(p, out->as.vec3, 3);
    }
    if (parser_try_keyword(p, "vec4"))
    {
        out->type = KDF_TYPE_VEC4;
        return parser_read_float_list(p, out->as.vec4, 4);
    }
    if (parser_try_keyword(p, "quat"))
    {
        out->type = KDF_TYPE_QUAT;
        return parser_read_float_list(p, out->as.vec4, 4);
    }
    if (parser_try_keyword(p, "color"))
    {
        out->type = KDF_TYPE_COLOR;
        return parser_read_float_list(p, out->as.vec4, 4);
    }

    // asset("...")
    if (parser_try_keyword(p, "asset"))
    {
        if (parser_expect_char(p, '(') != KDF_OK)
            return p->error;
        if (parser_read_quoted_string(p, NULL, 0) != KDF_OK)
            return p->error;
        parser_skip_spaces(p);
        if (parser_expect_char(p, ')') != KDF_OK)
            return p->error;
        out->type = KDF_TYPE_ASSET_REF;
        out->as.str = kdf__intern_len(p->strings, p->str_buf, strlen(p->str_buf));
        return KDF_OK;
    }

    // external("...")
    if (parser_try_keyword(p, "external"))
    {
        if (parser_expect_char(p, '(') != KDF_OK)
            return p->error;
        if (parser_read_quoted_string(p, NULL, 0) != KDF_OK)
            return p->error;
        parser_skip_spaces(p);
        if (parser_expect_char(p, ')') != KDF_OK)
            return p->error;
        out->type = KDF_TYPE_RESOURCE_REF;
        out->as.str = kdf__intern_len(p->strings, p->str_buf, strlen(p->str_buf));
        return KDF_OK;
    }

    // subresource type { ... }
    if (parser_try_keyword(p, "subresource"))
    {
        parser_skip_spaces(p);
        char type_name[256];
        if (parser_read_ident(p, type_name, sizeof(type_name)) != KDF_OK)
            return p->error;
        parser_skip_spaces(p);

        out->type = KDF_TYPE_SUBRESOURCE;
        out->as.object = (kdf_object *)kdf__alloc(p->alloc, sizeof(kdf_object));
        if (!out->as.object)
        {
            p->error = KDF_ERROR_OUT_OF_MEMORY;
            return p->error;
        }
        kdf__object_init(out->as.object, p->alloc, p->strings);
        out->as.object->type_name = kdf__intern(p->strings, type_name);

        return parser_parse_block(p, out->as.object);
    }

    // array
    if (p->cur == '[')
    {
        out->type = KDF_TYPE_ARRAY;
        out->as.array = (kdf_array *)kdf__alloc(p->alloc, sizeof(kdf_array));
        if (!out->as.array)
        {
            p->error = KDF_ERROR_OUT_OF_MEMORY;
            return p->error;
        }
        kdf__array_init(out->as.array, p->alloc, p->strings);
        return parser_parse_array(p, out->as.array);
    }

    // inline object
    if (p->cur == '{')
    {
        out->type = KDF_TYPE_OBJECT;
        out->as.object = (kdf_object *)kdf__alloc(p->alloc, sizeof(kdf_object));
        if (!out->as.object)
        {
            p->error = KDF_ERROR_OUT_OF_MEMORY;
            return p->error;
        }
        kdf__object_init(out->as.object, p->alloc, p->strings);
        return parser_parse_block(p, out->as.object);
    }

    // string
    if (p->cur == '"')
    {
        if (parser_read_quoted_string(p, NULL, 0) != KDF_OK)
            return p->error;
        out->type = KDF_TYPE_STRING;
        out->as.str = kdf__intern_len(p->strings, p->str_buf, strlen(p->str_buf));
        return KDF_OK;
    }

    // number
    if (is_digit(p->cur) || p->cur == '-')
    {
        return parser_read_number(p, out);
    }

    p->error = KDF_ERROR_PARSE;
    return KDF_ERROR_PARSE;
}

// object body parsing

static int parser_parse_object_body(parser *p, kdf_object *obj)
{
    for (;;)
    {
        parser_skip_blank_lines(p);
        if (!p->cur || p->cur == '}' || p->cur == ']')
            break;

        // read key
        char key[256];
        if (parser_read_ident(p, key, sizeof(key)) != KDF_OK)
            return p->error;
        parser_skip_spaces(p);

        // check for shorthand: key { ... } (nested object without '=')
        if (p->cur == '{')
        {
            kdf_object *child = kdf_obj_add_object(obj, key);
            if (!child)
            {
                p->error = KDF_ERROR_OUT_OF_MEMORY;
                return p->error;
            }
            if (parser_parse_block(p, child) != KDF_OK)
                return p->error;
            // ensure child has strings pointer
            child->strings = obj->strings;
            goto next_line;
        }

        // expect '='
        if (parser_expect_char(p, '=') != KDF_OK)
            return p->error;
        parser_skip_spaces(p);

        // parse value
        kdf_value val;
        memset(&val, 0, sizeof(val));
        if (parser_parse_value(p, &val) != KDF_OK)
            return p->error;

        // assign to object
        switch (val.type)
        {
        case KDF_TYPE_NULL:
            kdf_obj_set_null(obj, key);
            break;
        case KDF_TYPE_BOOL:
            kdf_obj_set_bool(obj, key, val.as.boolean);
            break;
        case KDF_TYPE_INT:
            kdf_obj_set_int(obj, key, val.as.i64);
            break;
        case KDF_TYPE_UINT:
            kdf_obj_set_uint(obj, key, val.as.u64);
            break;
        case KDF_TYPE_FLOAT:
            kdf_obj_set_float(obj, key, val.as.f64);
            break;
        case KDF_TYPE_STRING:
            kdf_obj_set_string(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_VEC2:
            kdf_obj_set_vec2(obj, key, val.as.vec2[0], val.as.vec2[1]);
            break;
        case KDF_TYPE_VEC3:
            kdf_obj_set_vec3(obj, key, val.as.vec3[0], val.as.vec3[1], val.as.vec3[2]);
            break;
        case KDF_TYPE_VEC4:
            kdf_obj_set_vec4(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_QUAT:
            kdf_obj_set_quat(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_COLOR:
            kdf_obj_set_color(obj, key, val.as.vec4[0], val.as.vec4[1], val.as.vec4[2], val.as.vec4[3]);
            break;
        case KDF_TYPE_ASSET_REF:
            kdf_obj_set_asset_ref(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_RESOURCE_REF:
            kdf_obj_set_resource_ref(obj, key, val.as.str ? val.as.str->data : "");
            break;
        case KDF_TYPE_ARRAY:
        {
            // transfer the temporary array directly to the object
            kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_ARRAY);
            if (e && val.as.array)
            {
                e->value.as.array = val.as.array;
                val.as.array = NULL; // prevent double-free
            }
            break;
        }
        case KDF_TYPE_OBJECT:
        {
            // transfer the temporary object directly
            kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_OBJECT);
            if (e && val.as.object)
            {
                e->value.as.object = val.as.object;
                val.as.object = NULL; // prevent double-free
            }
            break;
        }
        case KDF_TYPE_SUBRESOURCE:
        {
            // transfer the temporary subresource directly
            kdf_entry *e = kdf__object_insert(obj, key, KDF_TYPE_SUBRESOURCE);
            if (e && val.as.object)
            {
                e->value.as.object = val.as.object;
                val.as.object = NULL; // prevent double-free
            }
            break;
        }
        default:
            break;
        }

    next_line:
        parser_skip_spaces(p);
        if (p->cur == '\n')
        {
            parser_advance(p);
        }
    }

    return KDF_OK;
}

// file-level parsing

static int parser_parse_file(parser *p)
{
    // expect "kdf <version>" header
    parser_skip_spaces(p);
    if (!parser_try_keyword(p, "kdf"))
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    parser_skip_spaces(p);

    // read version number (just consume it, we support version 1)
    if (!is_digit(p->cur))
    {
        p->error = KDF_ERROR_PARSE;
        return KDF_ERROR_PARSE;
    }
    while (is_digit(p->cur))
    {
        parser_advance(p);
    }

    // skip to end of line
    parser_skip_line(p);
    parser_skip_blank_lines(p);

    // parse the root object body
    return parser_parse_object_body(p, &p->doc->root);
}

// public api

kdf_document *kdf_text_read(kdf_reader reader, const kdf_allocator *alloc)
{
    kdf_allocator a = kdf__alloc_or_default(alloc);
    kdf_document *doc = kdf_doc_create_alloc(&a);
    if (!doc)
        return NULL;

    parser p;
    memset(&p, 0, sizeof(p));
    p.reader = reader;
    p.line = 1;
    p.col = 0;
    p.doc = doc;
    p.alloc = &doc->alloc;
    p.strings = &doc->strings;

    // prime the parser
    parser_advance(&p);

    if (parser_parse_file(&p) != KDF_OK)
    {
        if (p.str_buf)
            kdf__free(p.alloc, p.str_buf, p.str_buf_cap);
        kdf_doc_destroy(doc);
        return NULL;
    }

    if (p.str_buf)
        kdf__free(p.alloc, p.str_buf, p.str_buf_cap);
    return doc;
}

kdf_document *kdf_text_load(const char *path, const kdf_allocator *alloc)
{
    if (!path)
        return NULL;

    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    // read entire file into memory
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0)
    {
        fclose(f);
        return NULL;
    }

    kdf_allocator a = kdf__alloc_or_default(alloc);
    char *data = (char *)a.alloc(a.userdata, (size_t)file_size);
    if (!data)
    {
        fclose(f);
        return NULL;
    }

    size_t read = fread(data, 1, (size_t)file_size, f);
    fclose(f);

    if (read != (size_t)file_size)
    {
        a.free(a.userdata, data, (size_t)file_size);
        return NULL;
    }

    // create a memory reader
    kdf_mem_reader mr;
    mr.data = (const uint8_t *)data;
    mr.size = (size_t)file_size;
    mr.pos = 0;

    kdf_reader r = kdf_mem_reader_create(&mr);
    kdf_document *doc = kdf_text_read(r, &a);

    a.free(a.userdata, data, (size_t)file_size);
    return doc;
}
