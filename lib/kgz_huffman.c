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

#define TABLE_BITS (KGZ_HUFFMAN_CACHE)

struct kgz_huffman_tree {
    huffman_cache_entry_t table[1 << TABLE_BITS];

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
        huffman_node_t* new_nodes = kgz_arena_allocate(arena, sizeof(huffman_node_t) * new_capacity, 8);
        if(!new_nodes) return -1;

        if(tree->nodes) { memcpy(new_nodes, tree->nodes, sizeof(huffman_node_t) * tree->nodes_count); }
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

uint32_t bit_reverse(uint32_t code, uint32_t bits) {
    uint32_t result = 0;
    for(uint32_t i = 0; i < bits; i++) {
        result <<= 1;
        if(code & 1) result |= 1;
        code >>= 1;
    }
    return result;
}

kgz_huffman_tree_t* kgz_huffman_tree_create(uint16_t* symbol_lengths, uint16_t symbol_count, kgz_arena_t* arena) {
    kgz_huffman_tree_t* tree = kgz_arena_allocate(arena, sizeof(kgz_huffman_tree_t), 8);
    tree->nodes = nullptr;
    tree->nodes_capacity = 0;
    tree->nodes_count = 0;
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

    memset(tree->table, 0xff, sizeof(tree->table));

    for(size_t sym = 0; sym < symbol_count; sym++) {
        uint16_t symbol_length = symbol_lengths[sym];
        if(symbol_length == 0) { continue; }

        uint32_t code = next_code[symbol_length];
        if(!insert_code(tree, next_code[symbol_length], symbol_length, sym, arena)) return nullptr;
        next_code[symbol_length]++;

        if(symbol_length > TABLE_BITS) continue;

        int32_t stride = 1 << symbol_length;
        uint32_t rev_code = bit_reverse(code, symbol_length);
        for(uint32_t fill = rev_code; fill < (1 << TABLE_BITS); fill += stride) {
            tree->table[fill].symbol = sym;
            tree->table[fill].length = symbol_length;
        }
    }

    return tree;
}

bool kgz_huffman_tree_lookup_slow(kgz_huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol) {
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

bool kgz_huffman_tree_lookup(kgz_huffman_tree_t* tree, kgz_bitstream_t* stream, uint16_t* symbol) {
    *symbol = 0;

    uint32_t bits = kgz_bitstream_peek(stream, TABLE_BITS);
    huffman_cache_entry_t entry = tree->table[bits & ((1 << TABLE_BITS) - 1)];

    if(entry.length > 0) {
        kgz_bitstream_getbits(stream, entry.length);
        *symbol = entry.symbol;
        return true;
    }

    return kgz_huffman_tree_lookup_slow(tree, stream, symbol);
}
