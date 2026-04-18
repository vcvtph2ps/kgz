#pragma once
#include <stddef.h>
#include <stdint.h>

/* portability stuff */
#ifndef KGZ_USE_OWN_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KGZ_MALLOC(size) malloc((size))
#define KGZ_CALLOC(n, size) calloc((n), (size))
#define KGZ_FREE(ptr) free((ptr))
#define KGZ_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define KGZ_MEMSET(ptr, val, size) memset((ptr), (val), (size))
#define KGZ_PRINTF(...) printf(__VA_ARGS__)
#endif

/* kgz_arena.c */
typedef struct kgz_arena {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
} kgz_arena_t;

void kgz_arena_init(kgz_arena_t* arena, size_t capacity);
void kgz_arena_reset(kgz_arena_t* arena);
void* kgz_arena_allocate(kgz_arena_t* arena, size_t size, size_t alignment);
void kgz_arena_free(kgz_arena_t* arena);

/* kgz_bitstream.c */
typedef struct kgz_bitstream {
    uint8_t* data;
    uint64_t data_len;
    uint64_t current_byte;
    uint8_t current_bit;
    uint64_t bit_buffer;
    uint8_t bits_in_buffer;
} kgz_bitstream_t;

/* kgz_buffer.c */
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} kgz_buffer_t;

extern bool kgz_buffer_insert(kgz_buffer_t* buffer, uint8_t byte);
extern bool kgz_buffer_insert_bulk(kgz_buffer_t* buffer, const uint8_t* src, size_t len);
extern bool kgz_buffer_lz77copy(kgz_buffer_t* buffer, size_t distance, size_t length);

/* kgz_huffman.c */
typedef struct huffman_tree huffman_tree_t;

typedef struct kgz_decompression_context {
    huffman_tree_t* fixed_huffman_tree;
    kgz_bitstream_t* bitstream;
    kgz_buffer_t output_buffer;
    kgz_arena_t arena_alloc;
} kgz_decompression_context_t;

extern uint32_t kgz_bitstream_getbits(kgz_bitstream_t* stream, uint32_t bits);
extern uint8_t kgz_bitstream_read_u8(kgz_bitstream_t* stream);
extern uint16_t kgz_bitstream_read_u16(kgz_bitstream_t* stream);
extern void kgz_bitstream_align(kgz_bitstream_t* stream);

/* kgz_gzip.c */
extern void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out);

/* kgz_deflate.c */
extern bool kgz_deflate_decompress(kgz_decompression_context_t* context);

/* kgz_huffman.c */
extern huffman_tree_t* kgz_huffman_tree_create(uint16_t* codes, uint16_t codes_len, kgz_arena_t* arena);
extern void kgz_huffman_tree_debug(huffman_tree_t* tree);
extern bool kgz_huffman_tree_lookup(huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol);
