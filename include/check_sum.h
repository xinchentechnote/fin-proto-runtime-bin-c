/*
 * Checksum services for fin-proto binary protocols.
 *
 * Mirrors fin-proto-runtime-bin-rs/src/check_sum.rs: the same four
 * algorithms, keyed by the string literals used in the DSL
 * `@calculatedFrom("...")` attribute:
 *
 * - CRC16    fc_crc16    — CRC-16/MODBUS (init 0xFFFF, poly 0xA001) -> uint16
 * - CRC32    fc_crc32    — CRC-32/ISO-HDLC (reflected poly 0xEDB88320)-> uint32
 * - SSE_BIN  fc_sse_bin  — byte sum folded to the low 8 bits        -> uint32
 * - SZSE_BIN fc_szse_bin — i32 byte sum, remainder mod 256           -> int32
 */
#ifndef FC_CHECK_SUM_H
#define FC_CHECK_SUM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t fc_crc16(const uint8_t *input, size_t len);
uint32_t fc_crc32(const uint8_t *input, size_t len);
uint32_t fc_sse_bin(const uint8_t *input, size_t len);
int32_t fc_szse_bin(const uint8_t *input, size_t len);

/*
 * Generated code calls the algorithm matching the field type at codegen
 * time (SSE_BIN -> unsigned fields, SZSE_BIN -> int32, CRC16 -> uint16,
 * CRC32 -> uint32), mirroring the exact-variant match of the generated
 * Rust code; any other combination keeps the field's existing value.
 */

#ifdef __cplusplus
}
#endif

#endif /* FC_CHECK_SUM_H */
