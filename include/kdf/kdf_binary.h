#ifndef KDF_BINARY_H
#define KDF_BINARY_H

#include "kdf_types.h"
#include "kdf_io.h"
#include "kdf_allocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

// binary format magic: "kdfb"
#define KDF_BINARY_MAGIC 0x4246444B

    // write a document in kdf binary format to a writer stream.
    // returns kdf_ok on success, or a negative error code.
    int kdf_binary_write(const kdf_document *doc, kdf_writer writer);

    // read a document in kdf binary format from a reader stream.
    // returns a new document, or null on error.
    kdf_document *kdf_binary_read(kdf_reader reader, const kdf_allocator *alloc);

    // file convenience wrappers
    int kdf_binary_save(const kdf_document *doc, const char *path);
    kdf_document *kdf_binary_load(const char *path, const kdf_allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif /* KDF_BINARY_H */
