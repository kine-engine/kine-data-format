#ifndef KDF_H
#define KDF_H

#include "kdf_types.h"
#include "kdf_allocator.h"
#include "kdf_io.h"
#include "kdf_document.h"
#include "kdf_value.h"
#include "kdf_object.h"
#include "kdf_array.h"
#include "kdf_text.h"
#include "kdf_binary.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // get the library version string
    const char *kdf_version(void);

#ifdef __cplusplus
}
#endif

#endif /* KDF_H */
