#include "kdf_internal.h"

// memory writer

static size_t mem_writer_cb(void *userdata, const void *data, size_t size)
{
    kdf_mem_writer *mw = (kdf_mem_writer *)userdata;

    if (mw->size + size > mw->capacity)
    {
        size_t new_cap = mw->capacity == 0 ? 256 : mw->capacity;
        while (new_cap < mw->size + size)
        {
            new_cap *= 2;
        }
        uint8_t *new_data = (uint8_t *)realloc(mw->data, new_cap);
        if (!new_data)
            return 0;
        mw->data = new_data;
        mw->capacity = new_cap;
    }

    memcpy(mw->data + mw->size, data, size);
    mw->size += size;
    return size;
}

kdf_writer kdf_mem_writer_create(kdf_mem_writer *mw)
{
    kdf_writer w;
    w.userdata = mw;
    w.write = mem_writer_cb;
    return w;
}

void kdf_mem_writer_finish(kdf_mem_writer *mw)
{
    if (mw->data)
    {
        free(mw->data);
        mw->data = NULL;
    }
    mw->size = 0;
    mw->capacity = 0;
}

// memory reader

static size_t mem_reader_cb(void *userdata, void *dst, size_t size)
{
    kdf_mem_reader *mr = (kdf_mem_reader *)userdata;
    size_t remaining = mr->size - mr->pos;
    if (size > remaining)
        size = remaining;
    if (size == 0)
        return 0;
    memcpy(dst, mr->data + mr->pos, size);
    mr->pos += size;
    return size;
}

static bool mem_reader_seek(void *userdata, int64_t offset, int origin)
{
    kdf_mem_reader *mr = (kdf_mem_reader *)userdata;
    int64_t new_pos;
    switch (origin)
    {
    case 0: // seek_set
        new_pos = offset;
        break;
    case 1: // seek_cur
        new_pos = (int64_t)mr->pos + offset;
        break;
    case 2: // seek_end
        new_pos = (int64_t)mr->size + offset;
        break;
    default:
        return false;
    }
    if (new_pos < 0 || (size_t)new_pos > mr->size)
        return false;
    mr->pos = (size_t)new_pos;
    return true;
}

static int64_t mem_reader_tell(void *userdata)
{
    kdf_mem_reader *mr = (kdf_mem_reader *)userdata;
    return (int64_t)mr->pos;
}

kdf_reader kdf_mem_reader_create(kdf_mem_reader *mr)
{
    kdf_reader r;
    r.userdata = mr;
    r.read = mem_reader_cb;
    r.seek = mem_reader_seek;
    r.tell = mem_reader_tell;
    return r;
}
