/*
 * binary_codec - runtime primitives for fin-proto C message libraries.
 *
 * Mirrors fin-proto-runtime-bin-rs / fin-proto-runtime-bin-zig:
 * a growable encode buffer and a cursor over an immutable byte slice,
 * endian-aware scalar access, fixed/dynamic string helpers with padding
 * semantics, and length-prefixed list support.
 *
 * Decoding never copies string payloads: fixed char arrays and dynamic
 * strings are returned as (pointer, length) views into the caller's input
 * buffer, so they stay valid only while the input bytes do. Only `repeat`
 * fields allocate (plain malloc/calloc); every generated message provides
 * a matching `<msg>_free` that releases exactly those allocations.
 *
 * Buffer/reader carry the configured byte order, so generated code never
 * needs `_le` call variants.
 */
#ifndef FC_BINARY_CODEC_H
#define FC_BINARY_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A borrowed, non-NUL-terminated byte string (usually UTF-8 text). */
typedef struct fc_string {
    const char *data;
    size_t len;
} fc_string;

/* Build an fc_string from a C string literal. */
#define FC_STR(lit) ((fc_string){(lit), sizeof(lit) - 1})

/* Deep byte comparison of two views. */
bool fc_string_eql(fc_string a, fc_string b);
/* Compare a view against a NUL-terminated literal. */
bool fc_streq_lit(fc_string a, const char *lit);

/* Validate that bytes are well-formed UTF-8. */
bool fc_utf8_valid(const char *data, size_t len);

/* ---------------------------------------------------------------------------
 * Encode buffer
 * ------------------------------------------------------------------------- */
typedef struct fc_buffer {
    uint8_t *data;
    size_t len;
    size_t cap;
    int little_endian; /* 0 = big endian (DSL default), 1 = little endian */
} fc_buffer;

void fc_buffer_init(fc_buffer *b, int little_endian);
void fc_buffer_free(fc_buffer *b);
/* Reset for another encode round, keeping the allocated capacity. */
void fc_buffer_reset(fc_buffer *b);
bool fc_buffer_reserve(fc_buffer *b, size_t extra);
bool fc_buffer_write(fc_buffer *b, const void *bytes, size_t n);

bool fc_buffer_put_u8(fc_buffer *b, uint8_t v);
bool fc_buffer_put_u16(fc_buffer *b, uint16_t v);
bool fc_buffer_put_u32(fc_buffer *b, uint32_t v);
bool fc_buffer_put_u64(fc_buffer *b, uint64_t v);
bool fc_buffer_put_i8(fc_buffer *b, int8_t v);
bool fc_buffer_put_i16(fc_buffer *b, int16_t v);
bool fc_buffer_put_i32(fc_buffer *b, int32_t v);
bool fc_buffer_put_i64(fc_buffer *b, int64_t v);
bool fc_buffer_put_f32(fc_buffer *b, float v);
bool fc_buffer_put_f64(fc_buffer *b, double v);

/* Overwrite a previously written scalar in place (used by @lengthOf fields). */
bool fc_buffer_patch_u16(fc_buffer *b, size_t pos, uint16_t v);
bool fc_buffer_patch_u32(fc_buffer *b, size_t pos, uint32_t v);
bool fc_buffer_patch_u64(fc_buffer *b, size_t pos, uint64_t v);

/* ---------------------------------------------------------------------------
 * Decode reader
 * ------------------------------------------------------------------------- */
typedef struct fc_reader {
    const uint8_t *data;
    size_t len;
    size_t pos;
    int little_endian;
} fc_reader;

void fc_reader_init(fc_reader *r, const uint8_t *data, size_t len, int little_endian);
size_t fc_reader_remaining(const fc_reader *r);
bool fc_reader_take(fc_reader *r, size_t n, const uint8_t **out);

bool fc_reader_get_u8(fc_reader *r, uint8_t *out);
bool fc_reader_get_u16(fc_reader *r, uint16_t *out);
bool fc_reader_get_u32(fc_reader *r, uint32_t *out);
bool fc_reader_get_u64(fc_reader *r, uint64_t *out);
bool fc_reader_get_i8(fc_reader *r, int8_t *out);
bool fc_reader_get_i16(fc_reader *r, int16_t *out);
bool fc_reader_get_i32(fc_reader *r, int32_t *out);
bool fc_reader_get_i64(fc_reader *r, int64_t *out);
bool fc_reader_get_f32(fc_reader *r, float *out);
bool fc_reader_get_f64(fc_reader *r, double *out);

/* ---------------------------------------------------------------------------
 * Fixed char arrays (DSL char[N] / zchar[N])
 * ------------------------------------------------------------------------- */
/* Right-pad with spaces; overlong input is truncated. */
bool fc_put_char_array(fc_buffer *b, const char *s, size_t s_len, size_t fixed_len);
/* Explicit pad character and side. */
bool fc_put_char_array_pad(fc_buffer *b, const char *s, size_t s_len, size_t fixed_len,
                           char pad_char, int pad_left);
/* Read a fixed-width char array and trim trailing spaces (borrows input). */
bool fc_get_char_array(fc_reader *r, size_t fixed_len, fc_string *out);
/* Read and trim the configured pad character. */
bool fc_get_char_array_trim(fc_reader *r, size_t fixed_len, char pad_char, int pad_left,
                            fc_string *out);

/* ---------------------------------------------------------------------------
 * Dynamic strings (DSL string / char[]) with a length prefix
 * ------------------------------------------------------------------------- */
/* lbytes: width of the length prefix in bytes (1, 2, 4 or 8). */
bool fc_put_string(fc_buffer *b, int lbytes, const char *s, size_t s_len);
/* Borrows from the input; fails on invalid UTF-8. */
bool fc_get_string(fc_reader *r, int lbytes, fc_string *out);

/* Length-prefix helpers used by generated repeat-field loops. */
bool fc_put_count(fc_buffer *b, int lbytes, size_t count);
bool fc_get_count(fc_reader *r, int lbytes, uint64_t *count);

#ifdef __cplusplus
}
#endif

#endif /* FC_BINARY_CODEC_H */
