#include <stdint.h>

#include "kgz_priv.h"

static inline void kgz_bitstream_fill(kgz_bitstream_t* stream) {
    while(stream->bits_in_buffer <= 56) {
        uint64_t byte_idx = stream->current_byte + (stream->current_bit + stream->bits_in_buffer) / 8;
        if(byte_idx >= stream->data_len) break;
        stream->bit_buffer |= ((uint64_t) stream->data[byte_idx] << stream->bits_in_buffer);
        stream->bits_in_buffer += 8;
    }
}

uint32_t kgz_bitstream_getbits(kgz_bitstream_t* stream, uint32_t bits) {
    if(bits == 0) return 0;
    if(stream->bits_in_buffer < bits) kgz_bitstream_fill(stream);
    uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1);
    uint32_t result = (uint32_t) (stream->bit_buffer & mask);
    stream->bit_buffer >>= bits;
    stream->bits_in_buffer -= bits;
    stream->current_bit += bits;
    stream->current_byte += stream->current_bit / 8;
    stream->current_bit &= 7;
    return result;
}

void kgz_bitstream_align(kgz_bitstream_t* stream) {
    if(stream->current_bit != 0) {
        uint8_t skip = 8 - stream->current_bit;
        stream->bit_buffer >>= skip;
        stream->bits_in_buffer -= (stream->bits_in_buffer >= skip) ? skip : stream->bits_in_buffer;
        stream->current_byte++;
        stream->current_bit = 0;
    }
}

uint8_t kgz_bitstream_read_u8(kgz_bitstream_t* stream) {
    return (uint8_t) kgz_bitstream_getbits(stream, 8);
}

uint16_t kgz_bitstream_read_u16(kgz_bitstream_t* stream) {
    return (uint16_t) kgz_bitstream_getbits(stream, 16);
}
