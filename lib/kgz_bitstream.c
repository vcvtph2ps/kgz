#include <stdint.h>

#include "kgz_priv.h"

static inline void kgz_bitstream_fill(kgz_bitstream_t* stream) {
    uint8_t bits = stream->bits_in_buffer;
    uint64_t cur_byte = stream->current_byte + ((stream->current_bit + bits) >> 3);
    const uint8_t* data = stream->data;
    uint64_t data_len = stream->data_len;
    uint64_t bit_buffer = stream->bit_buffer;

    while(bits <= 56 && cur_byte < data_len) {
        bit_buffer |= ((uint64_t) data[cur_byte++] << bits);
        bits += 8;
    }

    stream->bits_in_buffer = bits;
    stream->bit_buffer = bit_buffer;
}

uint32_t kgz_bitstream_getbits(kgz_bitstream_t* stream, uint32_t bits) {
    if(KGZ_UNLIKELY(bits == 0)) return 0;
    if(KGZ_UNLIKELY(stream->bits_in_buffer < bits)) kgz_bitstream_fill(stream);
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
    if(KGZ_LIKELY(stream->current_bit != 0)) {
        stream->current_byte++;
        stream->current_bit = 0;
    }
    stream->bit_buffer = 0;
    stream->bits_in_buffer = 0;
}

uint8_t kgz_bitstream_read_u8(kgz_bitstream_t* stream) {
    return (uint8_t) kgz_bitstream_getbits(stream, 8);
}

uint16_t kgz_bitstream_read_u16(kgz_bitstream_t* stream) {
    if(KGZ_LIKELY(stream->current_bit == 0 && stream->current_byte + 1 < stream->data_len)) {
        uint16_t word = (uint16_t) stream->data[stream->current_byte] | ((uint16_t) stream->data[stream->current_byte + 1] << 8);
        stream->current_byte += 2;
        return word;
    }
    return (uint16_t) kgz_bitstream_getbits(stream, 16);
}

uint32_t kgz_bitstream_peek(kgz_bitstream_t* stream, uint32_t bits) {
    if(KGZ_UNLIKELY(bits == 0)) return 0;
    if(KGZ_UNLIKELY(stream->bits_in_buffer < bits)) kgz_bitstream_fill(stream);
    uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1);
    return (uint32_t) (stream->bit_buffer & mask);
}

void kgz_bitstream_consume(kgz_bitstream_t* stream, uint32_t bits) {
    if(KGZ_UNLIKELY(bits == 0)) return;
    stream->bit_buffer >>= bits;
    stream->bits_in_buffer -= bits;
    stream->current_bit += bits;
    stream->current_byte += stream->current_bit / 8;
    stream->current_bit &= 7;
}
