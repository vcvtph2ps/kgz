// KernelGZ -- Single Header Build -- Generated With comp2header.pl
// Read the included README.md for how to use this library
// Find the source code at https://git.sr.ht/~evalyn/kgz
/*
==== Dual Licenced under Public Domain & 0BSD ====
==== 0BSD Licence ====
BSD Zero Clause License
Copyright (c) 2026 Evalyn Goemer & Luna
Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.
THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
==== Public Domain ====
This software is released to the public domain.
Anyone and anything may copy, edit, publish, use, compile, sell and
distribute this work and all its parts in any form for any purpose,
commercial and non-commercial, without any restrictions, without complying
with any conditions and by any means.
*/
#pragma once
#include <stdint.h>
#include <stddef.h>
#define KGZ_SINGLE_HEADER
extern void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out);
#ifdef KGZ_IMPLEMENTATION
#ifndef KGZ_USE_OWN_MACROS
#include <stdio.h>
#include <stdlib.h>
#define KGZ_MALLOC(size) malloc((size))
#define KGZ_CALLOC(n, size) calloc((n), (size))
#define KGZ_FREE(ptr) free((ptr))
#define KGZ_MEMSET(ptr, val, size) memset((ptr), (val), (size))
#define KGZ_PRINTF(...) printf(__VA_ARGS__)
#endif
typedef struct kgz_arena {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
} kgz_arena_t;
void kgz_arena_init(kgz_arena_t* arena, size_t capacity);
void kgz_arena_reset(kgz_arena_t* arena);
void* kgz_arena_allocate(kgz_arena_t* arena, size_t size, size_t alignment);
void kgz_arena_free(kgz_arena_t* arena);
typedef struct kgz_bitstream {
    uint8_t* data;
    uint64_t data_len;
    uint64_t current_byte;
    uint8_t current_bit;
} kgz_bitstream_t;
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} kgz_buffer_t;
typedef struct huffman_tree huffman_tree_t;
typedef struct kgz_decompression_context {
    huffman_tree_t* fixed_huffman_tree;
    kgz_bitstream_t* bitstream;
    kgz_buffer_t output_buffer;
    kgz_arena_t arena_alloc;
} kgz_decompression_context_t;
extern uint32_t kgz_bitstream_getbits(kgz_bitstream_t* stream, uint32_t bits);
extern uint8_t kgz_bitstream_getbit_msb(kgz_bitstream_t* stream);
extern uint8_t kgz_bitstream_read_u8(kgz_bitstream_t* stream);
extern uint16_t kgz_bitstream_read_u16(kgz_bitstream_t* stream);
extern uint32_t kgz_bitstream_read_u32(kgz_bitstream_t* stream);
extern void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out);
extern bool kgz_deflate_decompress(kgz_decompression_context_t* context);
extern huffman_tree_t* kgz_huffman_tree_create(uint16_t* codes, uint16_t codes_len, kgz_arena_t* arena);
extern void kgz_huffman_tree_debug(huffman_tree_t* tree);
extern bool kgz_huffman_tree_lookup(huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol);
extern void kgz_buffer_insert(kgz_buffer_t* buffer, uint8_t byte);
uint32_t kgz_bitstream_getbits(kgz_bitstream_t* stream, uint32_t bits) {
    uint32_t result = 0;
    for(uint32_t i = 0; i < bits; i++) {
        uint8_t byte = stream->data[stream->current_byte];
        uint8_t bit = (byte >> stream->current_bit) & 1;
        result |= ((uint32_t) bit << i);
        if(++stream->current_bit == 8) {
            stream->current_bit = 0;
            stream->current_byte++;
        }
    }
    return result;
}
uint8_t kgz_bitstream_read_u8(kgz_bitstream_t* stream) {
    return (uint8_t) kgz_bitstream_getbits(stream, 8);
}
uint16_t kgz_bitstream_read_u16(kgz_bitstream_t* stream) {
    uint16_t b0 = kgz_bitstream_getbits(stream, 8);
    uint16_t b1 = kgz_bitstream_getbits(stream, 8);
    return (uint16_t) (b0 | (b1 << 8));
}
uint32_t kgz_bitstream_read_u32(kgz_bitstream_t* stream) {
    uint32_t b0 = kgz_bitstream_getbits(stream, 8);
    uint32_t b1 = kgz_bitstream_getbits(stream, 8);
    uint32_t b2 = kgz_bitstream_getbits(stream, 8);
    uint32_t b3 = kgz_bitstream_getbits(stream, 8);
    return (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}
#define CHECK_BOUNDS(curr, size, needed)               \
    do {                                               \
        if((curr) + (needed) > (size)) return nullptr; \
    } while(0)
#define FTEXT (1 << 0)
#define FHCRC (1 << 1)
#define FEXTRA (1 << 2)
#define FNAME (1 << 3)
#define FCOMMENT (1 << 4)
void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out) {
    uint8_t* u8data = (uint8_t*) data;
    uint64_t current_byte = 0;
    CHECK_BOUNDS(current_byte, data_size, 2);
    if(u8data[current_byte] != 0x1F || u8data[current_byte + 1] != 0x8B) return nullptr;
    current_byte += 2;
    CHECK_BOUNDS(current_byte, data_size, 1);
    if(u8data[current_byte] != 8) return nullptr;
    current_byte += 1;
    CHECK_BOUNDS(current_byte, data_size, 1);
    uint8_t flag = u8data[current_byte];
    current_byte += 1;
    CHECK_BOUNDS(current_byte, data_size, 6);
    current_byte += 6;
    if(flag & FEXTRA) {
        CHECK_BOUNDS(current_byte, data_size, 2);
        uint16_t xlen = (uint16_t) u8data[current_byte] | ((uint16_t) u8data[current_byte + 1] << 8);
        current_byte += 2;
        CHECK_BOUNDS(current_byte, data_size, xlen);
        current_byte += xlen;
    }
    if(flag & FNAME) {
        while(true) {
            CHECK_BOUNDS(current_byte, data_size, 1);
            if(u8data[current_byte++] == 0) break;
        }
    }
    if(flag & FCOMMENT) {
        while(true) {
            CHECK_BOUNDS(current_byte, data_size, 1);
            if(u8data[current_byte++] == 0) break;
        }
    }
    if(flag & FHCRC) {
        CHECK_BOUNDS(current_byte, data_size, 2);
        current_byte += 2;
    }
    CHECK_BOUNDS(current_byte, data_size, 9);
    uint8_t* deflate_data = u8data + current_byte;
    kgz_bitstream_t deflate_bitstream = { 0 };
    deflate_bitstream.data = deflate_data;
    deflate_bitstream.data_len = data_size - current_byte - 8;
    deflate_bitstream.current_byte = 0;
    deflate_bitstream.current_bit = 0;
    uint64_t footer_offset = data_size - 8;
    uint32_t decompressed_size = 0;
    decompressed_size |= (uint32_t) u8data[footer_offset + 4];
    decompressed_size |= (uint32_t) u8data[footer_offset + 5] << 8;
    decompressed_size |= (uint32_t) u8data[footer_offset + 6] << 16;
    decompressed_size |= (uint32_t) u8data[footer_offset + 7] << 24;
    kgz_decompression_context_t context;
    context.bitstream = &deflate_bitstream;
    context.output_buffer.data = KGZ_CALLOC(1, decompressed_size);
    context.output_buffer.size = 0;
    context.output_buffer.capacity = decompressed_size;
    kgz_arena_init(&context.arena_alloc, 1024 * 24); // 24kb
    uint16_t codes[288] = { 0 };
    for(size_t i = 0; i < 288; i++) {
        if(i <= 143)
            codes[i] = 8;
        else if(i <= 255)
            codes[i] = 9;
        else if(i <= 279)
            codes[i] = 7;
        else
            codes[i] = 8;
    }
    kgz_arena_t fixed_huffman_arena;
    kgz_arena_init(&fixed_huffman_arena, 1024 * 16); // 16kb
    context.fixed_huffman_tree = kgz_huffman_tree_create(codes, 288, &fixed_huffman_arena);
    if(!context.fixed_huffman_tree) {
        kgz_arena_free(&context.arena_alloc);
        kgz_arena_free(&fixed_huffman_arena);
        KGZ_FREE(context.output_buffer.data);
        return nullptr;
    }
    bool success = kgz_deflate_decompress(&context);
    kgz_arena_free(&context.arena_alloc);
    kgz_arena_free(&fixed_huffman_arena);
    if(!success) {
        KGZ_FREE(context.output_buffer.data);
        return nullptr;
    }
    if(data_size_out) *data_size_out = decompressed_size;
    return context.output_buffer.data;
}
static const uint16_t g_base_length[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
};
static const uint8_t g_length_extra_bits[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
};
static const uint16_t g_base_dist[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
};
static const uint8_t g_dist_extra_bits[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
};
static const uint8_t g_clen_alpha_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
bool kgz_dflt_handle_stored(kgz_decompression_context_t* context) {
    if(context->bitstream->current_bit != 0) {
        context->bitstream->current_byte++;
        context->bitstream->current_bit = 0;
    }
    uint16_t len = kgz_bitstream_read_u16(context->bitstream);
    uint16_t nlen = kgz_bitstream_read_u16(context->bitstream);
    if(len != (~nlen & 0xffff)) {
        KGZ_PRINTF("invalid stored block with len %u and nlen %u\n", len, (~nlen & 0xffff));
        return false;
    }
    for(size_t i = 0; i < len; i++) {
        if(context->bitstream->current_byte >= context->bitstream->data_len) { break; }
        uint8_t byte = kgz_bitstream_read_u8(context->bitstream);
        kgz_buffer_insert(&context->output_buffer, byte);
    }
    return true;
}
bool create_dynamic_huffman_tables(kgz_decompression_context_t* context, huffman_tree_t** ltree, huffman_tree_t** dtree) {
    uint8_t hlit = kgz_bitstream_getbits(context->bitstream, 5);
    uint8_t hdist = kgz_bitstream_getbits(context->bitstream, 5);
    uint8_t hclen = kgz_bitstream_getbits(context->bitstream, 4);
    uint16_t hsym_lengths[19] = { 0 };
    for(int i = 0; i < hclen + 4; i++) { hsym_lengths[g_clen_alpha_order[i]] = kgz_bitstream_getbits(context->bitstream, 3); }
    huffman_tree_t* htree = kgz_huffman_tree_create(hsym_lengths, 19, &context->arena_alloc);
    if(!htree) return false;
    uint16_t symbol;
    uint16_t sym_lengths[288 + 32] = { 0 };
    for(int i = 0; i < (hlit + 257) + (hdist + 1);) {
        bool v = kgz_huffman_tree_lookup(htree, context->bitstream, &symbol);
        if(!v) { return false; }
        if(symbol <= 15) {
            sym_lengths[i] = symbol;
            i++;
            continue;
        } else if(symbol == 16) {
            uint8_t times = kgz_bitstream_getbits(context->bitstream, 2) + 3;
            if(i <= 0) { return false; }
            uint16_t value = sym_lengths[i - 1];
            for(int j = 0; j < times; j++, i++) { sym_lengths[i] = value; }
        } else if(symbol == 17) {
            uint8_t times = kgz_bitstream_getbits(context->bitstream, 3) + 3;
            for(int j = 0; j < times && i < (hlit + 257) + (hdist + 1); j++, i++) { sym_lengths[i] = 0; }
        } else if(symbol == 18) {
            uint8_t times = kgz_bitstream_getbits(context->bitstream, 7) + 11;
            for(int j = 0; j < times && i < (hlit + 257) + (hdist + 1); j++, i++) { sym_lengths[i] = 0; }
        }
    }
    *dtree = kgz_huffman_tree_create(&sym_lengths[hlit + 257], hdist + 1, &context->arena_alloc);
    if(!*dtree) return false;
    *ltree = kgz_huffman_tree_create(sym_lengths, hlit + 257, &context->arena_alloc);
    if(!*ltree) return false;
    return true;
}
bool kgz_dflt_handle_huffman(kgz_decompression_context_t* context, bool dynamic) {
    uint16_t symbol = 0;
    huffman_tree_t* dtree;
    huffman_tree_t* ltree;
    if(dynamic) {
        if(!create_dynamic_huffman_tables(context, &ltree, &dtree)) { return false; }
    } else {
        ltree = context->fixed_huffman_tree;
    }
    while(true) {
        if(context->bitstream->current_byte >= context->bitstream->data_len) { break; }
        bool v = kgz_huffman_tree_lookup(ltree, context->bitstream, &symbol);
        if(!v) break;
        if(symbol == 256) { break; }
        if(symbol < 256) {
            kgz_buffer_insert(&context->output_buffer, symbol);
            continue;
        }
        uint8_t extra_length_bits = g_length_extra_bits[symbol - 257];
        uint16_t length = g_base_length[symbol - 257] + kgz_bitstream_getbits(context->bitstream, extra_length_bits);
        uint16_t distance_symbol;
        if(dynamic) {
            if(!kgz_huffman_tree_lookup(dtree, context->bitstream, &distance_symbol)) { return false; }
        } else {
            distance_symbol = kgz_bitstream_getbits(context->bitstream, 5);
            {
                uint16_t resversed_dist_sym = 0;
                for(size_t i = 0; i < 5; i++) {
                    resversed_dist_sym = (resversed_dist_sym << 1) | (distance_symbol & 1);
                    distance_symbol >>= 1;
                }
                distance_symbol = resversed_dist_sym;
            }
        }
        uint16_t extra_dist = kgz_bitstream_getbits(context->bitstream, g_dist_extra_bits[distance_symbol]);
        uint16_t distance = g_base_dist[distance_symbol] + extra_dist;
        if(distance > context->output_buffer.size) { return false; }
        for(size_t i = 0; i < length; i++) { kgz_buffer_insert(&context->output_buffer, context->output_buffer.data[context->output_buffer.size - distance]); }
    }
    return true;
}
bool kgz_deflate_decompress(kgz_decompression_context_t* context) {
    bool last_block = false;
    bool success = true;
    while(!last_block) {
        if(kgz_bitstream_getbits(context->bitstream, 1) == true) last_block = true;
        uint8_t block_type = kgz_bitstream_getbits(context->bitstream, 2);
        switch(block_type) {
            case 0: success = kgz_dflt_handle_stored(context); break;
            case 1: success = kgz_dflt_handle_huffman(context, false); break;
            case 2: success = kgz_dflt_handle_huffman(context, true); break;
            case 3: return false;
        }
        kgz_arena_reset(&context->arena_alloc);
        if(!success) { break; }
    }
    return success;
}
typedef enum {
    HUFFMAN_NODE_TYPE_INTERNAL,
    HUFFMAN_NODE_TYPE_SYMBOL
} huffman_node_type;
typedef struct huffman_node huffman_node_t;
struct huffman_node {
    huffman_node_type type;
    union {
        struct {
            huffman_node_t* zero;
            huffman_node_t* one;
        } internal;
        struct {
            uint16_t symbol;
        } symbol;
    };
};
struct huffman_tree {
    huffman_node_t* root;
};
#define MAX_BITS 15
static inline huffman_node_t* alloc_new_node(kgz_arena_t* arena) {
    huffman_node_t* node = kgz_arena_allocate(arena, sizeof(huffman_node_t), 8);
    if(!node) return nullptr;
    node->type = HUFFMAN_NODE_TYPE_INTERNAL;
    return node;
}
static inline bool insert_code(huffman_node_t* root, uint32_t code, uint16_t len, uint16_t symbol, kgz_arena_t* arena) {
    huffman_node_t* node = root;
    for(int i = len - 1; i >= 0; i--) {
        uint32_t bit = (code >> i) & 1;
        if(bit == 0) {
            if(!node->internal.zero) {
                node->internal.zero = alloc_new_node(arena);
                if(!node->internal.zero) return false;
            }
            node = node->internal.zero;
        } else {
            if(!node->internal.one) {
                node->internal.one = alloc_new_node(arena);
                if(!node->internal.one) return false;
            }
            node = node->internal.one;
        }
    }
    node->type = HUFFMAN_NODE_TYPE_SYMBOL;
    node->symbol.symbol = symbol;
    return true;
}
huffman_tree_t* kgz_huffman_tree_create(uint16_t* symbol_lengths, uint16_t symbol_count, kgz_arena_t* arena) {
    huffman_tree_t* tree = kgz_arena_allocate(arena, sizeof(huffman_tree_t), 8);
    if(!tree) return nullptr;
    tree->root = alloc_new_node(arena);
    if(!tree->root) return nullptr;
    size_t bl_count[MAX_BITS + 1] = { 0 };
    for(size_t i = 0; i < symbol_count; i++) {
        if(symbol_lengths[i] > MAX_BITS) { return nullptr; }
        if(symbol_lengths[i] == 0) { continue; }
        bl_count[symbol_lengths[i]]++;
    }
    uint16_t next_code[MAX_BITS + 1] = { 0 };
    {
        uint32_t code = 0;
        bl_count[0] = 0;
        for(uint32_t bits = 1; bits <= MAX_BITS; bits++) {
            code = (code + bl_count[bits - 1]) << 1;
            next_code[bits] = code;
        }
    }
    for(size_t sym = 0; sym < symbol_count; sym++) {
        uint16_t symbol_length = symbol_lengths[sym];
        if(symbol_length != 0) {
            if(!insert_code(tree->root, next_code[symbol_length], symbol_length, sym, arena)) return nullptr;
            next_code[symbol_length]++;
        }
    }
    return tree;
}
bool kgz_huffman_tree_lookup(huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol) {
    *symbol = 0;
    huffman_node_t* current_node = tree->root;
    if(current_node == nullptr) { return false; }
    int current_length = 0;
    while(1) {
        if(current_node->type == HUFFMAN_NODE_TYPE_SYMBOL) {
            *symbol = current_node->symbol.symbol;
            return true;
        }
        uint8_t bit = kgz_bitstream_getbits(stream, 1);
        current_node = bit ? current_node->internal.one : current_node->internal.zero;
        if(current_node == nullptr) { return false; }
        current_length++;
        if(current_length > 15) { return false; }
    }
    return false;
}
void kgz_buffer_insert(kgz_buffer_t* buffer, uint8_t byte) {
    if(buffer->size >= buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        uint8_t* new_data = KGZ_CALLOC(1, new_capacity);
        for(size_t i = 0; i < buffer->size; i++) { new_data[i] = buffer->data[i]; }
        KGZ_FREE(buffer->data);
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
    buffer->data[buffer->size++] = byte;
}
#include <stdio.h>
#include <string.h>
void kgz_arena_init(kgz_arena_t* arena, size_t capacity) {
    if(!arena) return;
    arena->buffer = (uint8_t*) KGZ_MALLOC(capacity);
    if(!arena->buffer) {
        arena->capacity = 0;
        arena->offset = 0;
        return;
    }
    arena->capacity = capacity;
    arena->offset = 0;
}
void kgz_arena_reset(kgz_arena_t* arena) {
    if(!arena) return;
    arena->offset = 0;
}
void* kgz_arena_allocate(kgz_arena_t* arena, size_t size, size_t alignment) {
    if(!arena || !arena->buffer) return nullptr;
    if(alignment == 0 || (alignment & (alignment - 1)) != 0) return nullptr;
    uintptr_t base = (uintptr_t) arena->buffer;
    uintptr_t current = base + arena->offset;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t new_offset = (size_t) ((aligned - base) + size);
    if(new_offset > arena->capacity) { return nullptr; }
    void* ptr = (void*) aligned;
    KGZ_MEMSET(ptr, 0, size);
    arena->offset = new_offset;
    return ptr;
}
void kgz_arena_free(kgz_arena_t* arena) {
    if(!arena) return;
    KGZ_FREE(arena->buffer);
    arena->buffer = nullptr;
    arena->capacity = 0;
    arena->offset = 0;
}
#endif
