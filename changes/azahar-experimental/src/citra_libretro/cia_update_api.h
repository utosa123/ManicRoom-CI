// C-only, versioned experimental update importer. Not the legacy void CIA ABI.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {
    MCIA_SUCCESS = 0, MCIA_INVALID_FILE = 1, MCIA_UNSUPPORTED = 2,
    MCIA_WRONG_ENVIRONMENT = 3, MCIA_INSTALL_FAILED = 4,
    MCIA_ALREADY_INSTALLED = 5, MCIA_BUSY = 6, MCIA_CANCELLED = 7,
    MCIA_NO_SPACE = 8
};
typedef struct manic_cia_result_v1 {
    uint32_t size;
    int32_t status;
    uint64_t title_id;
    uint32_t title_version; // Raw TMD value, NOT a marketing version such as "1.2".
    uint32_t content_count;
    uint64_t content_bytes;
} manic_cia_result_v1;
typedef int32_t (*manic_cia_cancel_v1)(void *context);
// Synchronous, exclusive stopped-core worker. All pointers are borrowed until return.
// No retro_init, emulation, other core API, or unload may overlap this call.
// Cancellation is polled before commit; success after commit wins over a late cancel.
typedef int32_t (*manic_install_update_cia_v1)(const char *root, const char *path,
    manic_cia_result_v1 *result, uint32_t result_size,
    manic_cia_cancel_v1 cancel, void *context);
#ifdef __cplusplus
}
#endif
