#include "check_sum.h"

#include <string.h>

uint16_t fc_crc16(const uint8_t *input, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)input[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x0001)
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            else
                crc = (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

uint32_t fc_crc32(const uint8_t *input, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)input[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x00000001)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

uint32_t fc_sse_bin(const uint8_t *input, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) sum = (sum + input[i]) & 0xFF;
    return sum;
}

int32_t fc_szse_bin(const uint8_t *input, size_t len) {
    int32_t sum = 0;
    for (size_t i = 0; i < len; i++) sum += (int32_t)input[i];
    return sum % 256;
}
