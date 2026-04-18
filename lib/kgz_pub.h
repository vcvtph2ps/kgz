#pragma once
#include <stdint.h>

extern void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out);
