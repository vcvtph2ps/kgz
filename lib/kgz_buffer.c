#include "kgz_priv.h"

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
