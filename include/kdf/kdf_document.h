#ifndef KDF_DOCUMENT_H
#define KDF_DOCUMENT_H

#include "kdf_types.h"
#include "kdf_allocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // create a document with the default allocator (malloc/realloc/free).
    kdf_document *kdf_doc_create(void);

    // create a document with a custom allocator.
    kdf_document *kdf_doc_create_alloc(const kdf_allocator *alloc);

    // destroy a document and free all associated memory.
    void kdf_doc_destroy(kdf_document *doc);

    // get the root object of the document. never returns null for a valid doc.
    kdf_object *kdf_doc_root(kdf_document *doc);

#ifdef __cplusplus
}
#endif

#endif /* KDF_DOCUMENT_H */
