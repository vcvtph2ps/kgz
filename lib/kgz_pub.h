#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out, uint64_t* buffer_size_out);

#ifdef __cplusplus
}
#endif
