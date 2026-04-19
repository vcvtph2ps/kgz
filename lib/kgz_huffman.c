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
            int32_t zero_index;
            int32_t one_index;
        } internal;

        struct {
            uint16_t symbol;
        } symbol;
    };
};

struct kgz_huffman_tree {
    int32_t root_index;
    huffman_node_t* nodes;
    int32_t nodes_capacity;
    int32_t nodes_count;
};

#define MAX_BITS 15

static inline huffman_node_t* get_node(kgz_huffman_tree_t* tree, int32_t index) {
    if(index < 0 || index >= tree->nodes_count) {
        printf("Lookup failed: index out of bounds (%d)\n", index);
        return nullptr;
    }
    return &tree->nodes[index];
}

static inline int32_t alloc_new_node(kgz_huffman_tree_t* tree, kgz_arena_t* arena) {
    if(tree->nodes_count >= tree->nodes_capacity) {
        int32_t new_capacity = tree->nodes_capacity == 0 ? 16 : tree->nodes_capacity * 2;
        huffman_node_t* new_nodes = KGZ_CALLOC(1, sizeof(huffman_node_t) * new_capacity);
        if(!new_nodes) return -1;

        if(tree->nodes) { memcpy(new_nodes, tree->nodes, sizeof(huffman_node_t) * tree->nodes_count); }
        KGZ_FREE(tree->nodes);
        tree->nodes = new_nodes;
        tree->nodes_capacity = new_capacity;
    }

    int32_t index = tree->nodes_count++;
    tree->nodes[index].type = HUFFMAN_NODE_TYPE_INTERNAL;
    tree->nodes[index].internal.zero_index = -1;
    tree->nodes[index].internal.one_index = -1;
    return index;
}

static inline bool insert_code(kgz_huffman_tree_t* tree, uint32_t code, uint16_t len, uint16_t symbol, kgz_arena_t* arena) {
    int32_t current_index = tree->root_index;

    for(int i = len - 1; i >= 0; i--) {
        uint32_t bit = (code >> i) & 1;

        if(bit == 0) {
            if(get_node(tree, current_index)->internal.zero_index == -1) {
                int32_t new_index = alloc_new_node(tree, arena);
                if(new_index == -1) return false;
                get_node(tree, current_index)->internal.zero_index = new_index;
            }
            current_index = get_node(tree, current_index)->internal.zero_index;
        } else {
            if(get_node(tree, current_index)->internal.one_index == -1) {
                int32_t new_index = alloc_new_node(tree, arena);
                if(new_index == -1) return false;
                get_node(tree, current_index)->internal.one_index = new_index;
            }
            current_index = get_node(tree, current_index)->internal.one_index;
        }
    }

    huffman_node_t* node = get_node(tree, current_index);
    node->type = HUFFMAN_NODE_TYPE_SYMBOL;
    node->symbol.symbol = symbol;
    return true;
}

kgz_huffman_tree_t* kgz_huffman_tree_create(uint16_t* symbol_lengths, uint16_t symbol_count, kgz_arena_t* arena) {
    kgz_huffman_tree_t* tree = kgz_arena_allocate(arena, sizeof(kgz_huffman_tree_t), 8);
    if(!tree) return nullptr;

    tree->root_index = alloc_new_node(tree, arena);
    if(tree->root_index == -1) return nullptr;

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
            if(!insert_code(tree, next_code[symbol_length], symbol_length, sym, arena)) return nullptr;
            next_code[symbol_length]++;
        }
    }

    return tree;
}

bool kgz_huffman_tree_lookup(kgz_huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol) {
    *symbol = 0;
    huffman_node_t* current_node = get_node(tree, tree->root_index);
    if(current_node == nullptr) { return false; }

    int current_length = 0;
    while(1) {
        if(current_node->type == HUFFMAN_NODE_TYPE_SYMBOL) {
            *symbol = current_node->symbol.symbol;
            return true;
        }

        uint8_t bit = kgz_bitstream_getbits(stream, 1);
        current_node = get_node(tree, bit ? current_node->internal.one_index : current_node->internal.zero_index);

        if(current_node == nullptr) { return false; }

        current_length++;
        if(current_length > 15) { return false; }
    }
    return false;
}
