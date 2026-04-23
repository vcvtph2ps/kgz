# kGZ (KernelGZ)
A public domain library for freestanding environments for decompressing .gz files

## Usage
Simply add the `kgz_singleheader.h` to your project (or build it with `comp2header.pl`) and in one source file define `KGZ_IMPLEMENTATION` like this.

```c
#define KGZ_IMPLEMENTATION
#include "kgz_singleheader.h"

void* file_ptr;
uint64_t file_size;

// fill in file_ptr and file_size as needed on your platform

...

// once you have a file call
// void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out, uint64_t* buffer_size_out);
// to decompress and get a pointer to the decompressed data. make sure to call free() on it after using 

uint64_t result_size; // size of decompressed data
uint64_t buffer_size; // size of allocated buffer (may be larger than result_size)
// on any failure this will return NULL
void* result = kgz_gzip_decompress(file_ptr, file_size, &result_size, &buffer_size);

// do some work

...

free(result);
```

## Portability
This library requires the following standard functions

- malloc()
- free() (may be sized or unsized)
- memcpy()
- memset()
- printf() (may be stubbed)

If these are not located in the standard locations (or you wish to stub printf alone) you need do a bit of extra work

before including the main header you need to define `KGZ_USE_OWN_MACROS` and then define the following macros like this

```c
// include the headers for your platform
#include <kalloc.h>
#include <kprintf.h>
#include <string.h>

#define KGZ_USE_OWN_MACROS
#define KGZ_MALLOC(size) kmalloc((size))
#define KGZ_FREE(ptr, size) kfree((ptr))
#define KGZ_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define KGZ_MEMSET(ptr, val, size) memset((ptr), (val), (size))
// this may become a NOP
#define KGZ_PRINTF(...) kprintf(__VA_ARGS__)

// continue to use
#define KGZ_IMPLEMENTATION
#include "kgz_singleheader.h"
```

## User Options
You may also define the following things

- `KGZ_HUFFMAN_CACHE` ; adjusts how much of the huffman tree is cached, can affect speed & memory usage ; must be between `3` and `15`
