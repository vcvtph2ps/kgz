#include <stdint.h>

#include "kgz_priv.h"

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
    kgz_bitstream_align(context->bitstream);

    uint16_t len = kgz_bitstream_read_u16(context->bitstream);
    uint16_t nlen = kgz_bitstream_read_u16(context->bitstream);
    if(len != (~nlen & 0xffff)) {
        KGZ_PRINTF("invalid stored block with len %u and nlen %u\n", len, (~nlen & 0xffff));
        return false;
    }

    uint64_t avail = context->bitstream->data_len - context->bitstream->current_byte;
    if(KGZ_UNLIKELY((uint64_t) len > avail)) len = (uint16_t) avail;

    const uint8_t* src = context->bitstream->data + context->bitstream->current_byte;
    if(KGZ_UNLIKELY(!kgz_buffer_insert_bulk(&context->output_buffer, src, len))) return false;
    context->bitstream->current_byte += len;
    return true;
}

bool create_dynamic_huffman_tables(kgz_decompression_context_t* context, kgz_huffman_tree_t** ltree, kgz_huffman_tree_t** dtree) {
    uint32_t data = kgz_bitstream_peek(context->bitstream, 5 + 5 + 4);
    kgz_bitstream_consume(context->bitstream, 5 + 5 + 4);
    uint8_t hlit = data & 0x1f;
    uint8_t hdist = (data >> 5) & 0x1f;
    uint8_t hclen = (data >> 10) & 0xf;
    uint16_t hsym_lengths[19] = { 0 };

    for(int i = 0; i < hclen + 4; i++) { hsym_lengths[g_clen_alpha_order[i]] = kgz_bitstream_getbits(context->bitstream, 3); }

    kgz_huffman_tree_t* htree = kgz_huffman_tree_create(hsym_lengths, 19, &context->arena_alloc);
    if(KGZ_UNLIKELY(!htree)) return false;

    uint16_t symbol;
    uint16_t sym_lengths[288 + 32] = { 0 };
    for(int i = 0; i < (hlit + 257) + (hdist + 1);) {
        bool v = kgz_huffman_tree_lookup(htree, context->bitstream, &symbol);
        if(KGZ_UNLIKELY(!v)) { return false; }

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

    kgz_huffman_tree_t* dtree_tmp = kgz_huffman_tree_create(&sym_lengths[hlit + 257], hdist + 1, &context->arena_alloc);
    kgz_huffman_tree_t* ltree_tmp = kgz_huffman_tree_create(sym_lengths, hlit + 257, &context->arena_alloc);
    if(KGZ_UNLIKELY(!dtree_tmp)) return false;
    if(KGZ_UNLIKELY(!ltree_tmp)) return false;

    *dtree = dtree_tmp;
    *ltree = ltree_tmp;
    return true;
}

bool kgz_dflt_handle_huffman(kgz_decompression_context_t* context, bool dynamic) {
    uint16_t symbol = 0;

    kgz_huffman_tree_t* dtree;
    kgz_huffman_tree_t* ltree;

    if(dynamic) {
        if(KGZ_UNLIKELY(!create_dynamic_huffman_tables(context, &ltree, &dtree))) return false;
    } else {
        ltree = context->fixed_huffman_tree;
    }

    while(true) {
        if(context->bitstream->current_byte >= context->bitstream->data_len) break;

        bool v = kgz_huffman_tree_lookup(ltree, context->bitstream, &symbol);
        if(KGZ_UNLIKELY(!v)) break;
        if(KGZ_LIKELY(symbol < 256)) {
            if(!kgz_buffer_insert(&context->output_buffer, (uint8_t) symbol)) return false;
            continue;
        }
        if(KGZ_UNLIKELY(symbol == 256)) break;

        uint8_t extra_length_bits = g_length_extra_bits[symbol - 257];
        uint16_t length = g_base_length[symbol - 257] + kgz_bitstream_getbits(context->bitstream, extra_length_bits);

        uint16_t distance_symbol;
        if(dynamic) {
            if(!kgz_huffman_tree_lookup(dtree, context->bitstream, &distance_symbol)) return false;
        } else {
            distance_symbol = kgz_bitstream_getbits(context->bitstream, 5);
            {
                uint16_t rev = 0;
                for(size_t i = 0; i < 5; i++) {
                    rev = (rev << 1) | (distance_symbol & 1);
                    distance_symbol >>= 1;
                }
                distance_symbol = rev;
            }
        }

        uint16_t extra_dist = kgz_bitstream_getbits(context->bitstream, g_dist_extra_bits[distance_symbol]);
        uint16_t distance = g_base_dist[distance_symbol] + extra_dist;
        if(KGZ_UNLIKELY(!kgz_buffer_lz77copy(&context->output_buffer, distance, length))) return false;
    }

    return true;
}

bool kgz_deflate_decompress(kgz_decompression_context_t* context) {
    bool last_block = false;
    bool success = true;
    while(KGZ_UNLIKELY(!last_block)) {
        if(KGZ_UNLIKELY(kgz_bitstream_getbits(context->bitstream, 1) == true)) last_block = true;

        uint8_t block_type = kgz_bitstream_getbits(context->bitstream, 2);

        switch(block_type) {
            case 0: success = kgz_dflt_handle_stored(context); break;
            case 1: success = kgz_dflt_handle_huffman(context, false); break;
            case 2: success = kgz_dflt_handle_huffman(context, true); break;
            case 3: return false;
        }

        kgz_arena_reset(&context->arena_alloc);
        if(KGZ_UNLIKELY(!success)) { break; }
    }

    return success;
}
