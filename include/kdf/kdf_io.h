#ifndef KDF_IO_H
#define KDF_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // write callback: write size bytes from data to the stream.
    // returns number of bytes written, or 0 on error.
    typedef struct kdf_writer
    {
        void *userdata;
        size_t (*write)(void *userdata, const void *data, size_t size);
    } kdf_writer;

    // read callback: read up to size bytes into dst.
    // returns number of bytes actually read, or 0 on error/eof.
    typedef struct kdf_reader
    {
        void *userdata;
        size_t (*read)(void *userdata, void *dst, size_t size);
        bool (*seek)(void *userdata, int64_t offset, int origin);
        int64_t (*tell)(void *userdata);
    } kdf_reader;

    // convenience: create a writer that appends to a dynamically growing buffer.
    // free the buffer with kdf_mem_writer_finish().
    typedef struct kdf_mem_writer
    {
        uint8_t *data;
        size_t size;
        size_t capacity;
    } kdf_mem_writer;

    kdf_writer kdf_mem_writer_create(kdf_mem_writer *mw);
    void kdf_mem_writer_finish(kdf_mem_writer *mw);

    // convenience: create a reader that reads from a memory buffer.
    typedef struct kdf_mem_reader
    {
        const uint8_t *data;
        size_t size;
        size_t pos;
    } kdf_mem_reader;

    kdf_reader kdf_mem_reader_create(kdf_mem_reader *mr);

#ifdef __cplusplus
}
#endif

#endif /* KDF_IO_H */
