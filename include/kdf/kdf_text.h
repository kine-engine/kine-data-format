#ifndef KDF_TEXT_H
#define KDF_TEXT_H

#include "kdf_types.h"
#include "kdf_io.h"
#include "kdf_allocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // write a document in kdf text format to a writer stream.
    // returns kdf_ok on success, or a negative error code.
    int kdf_text_write(const kdf_document *doc, kdf_writer writer);

    // read a document in kdf text format from a reader stream.
    // returns a new document, or null on error.
    kdf_document *kdf_text_read(kdf_reader reader, const kdf_allocator *alloc);

    // file convenience wrappers
    int kdf_text_save(const kdf_document *doc, const char *path);
    kdf_document *kdf_text_load(const char *path, const kdf_allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif /* KDF_TEXT_H */
