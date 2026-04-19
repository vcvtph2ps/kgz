#include <stdint.h>

#include "kgz_priv.h"

#define CHECK_BOUNDS(curr, size, needed)               \
    do {                                               \
        if((curr) + (needed) > (size)) return nullptr; \
    } while(0)

#define FTEXT (1 << 0)
#define FHCRC (1 << 1)
#define FEXTRA (1 << 2)
#define FNAME (1 << 3)
#define FCOMMENT (1 << 4)

void* kgz_gzip_decompress(void* data, uint64_t data_size, uint64_t* data_size_out, uint64_t* buffer_size_out) {
    uint8_t* u8data = (uint8_t*) data;
    uint64_t current_byte = 0;

    // magic
    CHECK_BOUNDS(current_byte, data_size, 2);
    if(u8data[current_byte] != 0x1F || u8data[current_byte + 1] != 0x8B) return nullptr;
    current_byte += 2;

    // compression method
    CHECK_BOUNDS(current_byte, data_size, 1);
    if(u8data[current_byte] != 8) return nullptr;
    current_byte += 1;

    // flags
    CHECK_BOUNDS(current_byte, data_size, 1);
    uint8_t flag = u8data[current_byte];
    current_byte += 1;

    // skip time & extra flags & os type
    CHECK_BOUNDS(current_byte, data_size, 6);
    current_byte += 6;

    // skip extra
    if(flag & FEXTRA) {
        CHECK_BOUNDS(current_byte, data_size, 2);
        uint16_t xlen = (uint16_t) u8data[current_byte] | ((uint16_t) u8data[current_byte + 1] << 8);
        current_byte += 2;
        CHECK_BOUNDS(current_byte, data_size, xlen);
        current_byte += xlen;
    }

    // skip filename
    if(flag & FNAME) {
        while(true) {
            CHECK_BOUNDS(current_byte, data_size, 1);
            if(u8data[current_byte++] == 0) break;
        }
    }

    // skip comment
    if(flag & FCOMMENT) {
        while(true) {
            CHECK_BOUNDS(current_byte, data_size, 1);
            if(u8data[current_byte++] == 0) break;
        }
    }

    // skip crc16
    if(flag & FHCRC) {
        CHECK_BOUNDS(current_byte, data_size, 2);
        current_byte += 2;
    }

    // ensure there is some deflate data + footer
    CHECK_BOUNDS(current_byte, data_size, 9);
    uint8_t* deflate_data = u8data + current_byte;
    kgz_bitstream_t deflate_bitstream = { 0 };
    deflate_bitstream.data = deflate_data;
    deflate_bitstream.data_len = data_size - current_byte - 8;
    deflate_bitstream.current_byte = 0;
    deflate_bitstream.current_bit = 0;
    deflate_bitstream.bit_buffer = 0;
    deflate_bitstream.bits_in_buffer = 0;

    uint64_t footer_offset = data_size - 8;
    uint32_t decompressed_size = 0;
    decompressed_size |= (uint32_t) u8data[footer_offset + 4];
    decompressed_size |= (uint32_t) u8data[footer_offset + 5] << 8;
    decompressed_size |= (uint32_t) u8data[footer_offset + 6] << 16;
    decompressed_size |= (uint32_t) u8data[footer_offset + 7] << 24;

    kgz_decompression_context_t context;
    context.bitstream = &deflate_bitstream;
    context.output_buffer.data = KGZ_CALLOC(1, decompressed_size + 1024);
    context.output_buffer.size = 0;
    context.output_buffer.capacity = decompressed_size + 1024;
    kgz_arena_init(&context.arena_alloc, (1024 * 32) + ((sizeof(huffman_cache_entry_t) * 3) * (1 << (KGZ_HUFFMAN_CACHE)))); // 32kb base + cache augment

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
    kgz_arena_init(&fixed_huffman_arena, (1024 * 24) + ((sizeof(huffman_cache_entry_t) * 1) * (1 << (KGZ_HUFFMAN_CACHE)))); // 24kb base + cache augment
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
    if(buffer_size_out) *buffer_size_out = context.output_buffer.capacity;

    return context.output_buffer.data;
}
