#include <stdint.h>

#include "kgz_priv.h"

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
