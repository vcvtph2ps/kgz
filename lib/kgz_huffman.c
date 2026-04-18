#include <stdint.h>

#include "kgz_priv.h"

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
    node->type = HUFFMAN_NODE_TYPE_INTERNAL;
    return node;
}

static inline void insert_code(huffman_node_t* root, uint32_t code, uint16_t len, uint16_t symbol, kgz_arena_t* arena) {
    huffman_node_t* node = root;

    for(int i = len - 1; i >= 0; i--) {
        uint32_t bit = (code >> i) & 1;

        if(bit == 0) {
            if(!node->internal.zero) { node->internal.zero = alloc_new_node(arena); }
            node = node->internal.zero;
        } else {
            if(!node->internal.one) { node->internal.one = alloc_new_node(arena); }
            node = node->internal.one;
        }
    }

    node->type = HUFFMAN_NODE_TYPE_SYMBOL;
    node->symbol.symbol = symbol;
}

huffman_tree_t* kgz_huffman_tree_create(uint16_t* symbol_lengths, uint16_t symbol_count, kgz_arena_t* arena) {
    huffman_tree_t* tree = kgz_arena_allocate(arena, sizeof(huffman_tree_t), 8);
    tree->root = alloc_new_node(arena);

    size_t bl_count[MAX_BITS + 1] = { 0 };
    for(size_t i = 0; i < symbol_count; i++) {
        if(symbol_lengths[i] > MAX_BITS) {
            return nullptr;
        }
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
            insert_code(tree->root, next_code[symbol_length], symbol_length, sym, arena);
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
