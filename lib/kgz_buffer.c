#include "kgz_priv.h"

bool kgz_buffer_insert(kgz_buffer_t* buffer, uint8_t byte) {
    if(buffer->size >= buffer->capacity) {
        KGZ_PRINTF("output buffer overflow: size %zu capacity %zu\n", buffer->size, buffer->capacity);
        return false;
    }
    buffer->data[buffer->size++] = byte;
    return true;
}

bool kgz_buffer_insert_bulk(kgz_buffer_t* buffer, const uint8_t* src, size_t len) {
    if(buffer->size + len > buffer->capacity) {
        KGZ_PRINTF("output buffer overflow: need %zu have %zu\n", len, buffer->capacity - buffer->size);
        return false;
    }
    KGZ_MEMCPY(buffer->data + buffer->size, src, len);
    buffer->size += len;
    return true;
}

bool kgz_buffer_lz77copy(kgz_buffer_t* buffer, size_t distance, size_t length) {
    if(distance > buffer->size) return false;
    if(buffer->size + length > buffer->capacity) {
        KGZ_PRINTF("output buffer overflow during backcopy\n");
        return false;
    }

    uint8_t* dest = buffer->data + buffer->size;
    const uint8_t* src = dest - distance;
    if(distance == 1) {
        memset(dest, src[0], length);
    } else if(distance >= length) {
        KGZ_MEMCPY(dest, src, length);
    } else {
        for(size_t i = 0; i < length; i++) { dest[i] = src[i]; }
    }

    buffer->size += length;
    return true;
}
