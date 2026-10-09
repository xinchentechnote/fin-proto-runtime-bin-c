/* Runtime self-tests: mirrors the unit tests of the zig/rust runtimes. */
#include "binary_codec.h"
#include "check_sum.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #x, __FILE__, __LINE__);                 \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_scalar_roundtrip(int little_endian) {
    fc_buffer b;
    fc_buffer_init(&b, little_endian);
    CHECK(fc_buffer_put_u8(&b, 0xAB));
    CHECK(fc_buffer_put_i8(&b, -42));
    CHECK(fc_buffer_put_u16(&b, 0x1234));
    CHECK(fc_buffer_put_i16(&b, -1234));
    CHECK(fc_buffer_put_u32(&b, 0x12345678u));
    CHECK(fc_buffer_put_i32(&b, -123456));
    CHECK(fc_buffer_put_u64(&b, 0x0123456789ABCDEFull));
    CHECK(fc_buffer_put_i64(&b, -123456789));
    CHECK(fc_buffer_put_f32(&b, 1.5f));
    CHECK(fc_buffer_put_f64(&b, -2.25));

    fc_reader r;
    fc_reader_init(&r, b.data, b.len, little_endian);
    uint8_t u8v;
    int8_t i8v;
    uint16_t u16v;
    int16_t i16v;
    uint32_t u32v;
    int32_t i32v;
    uint64_t u64v;
    int64_t i64v;
    float f32v;
    double f64v;
    CHECK(fc_reader_get_u8(&r, &u8v));
    CHECK(fc_reader_get_i8(&r, &i8v));
    CHECK(fc_reader_get_u16(&r, &u16v));
    CHECK(fc_reader_get_i16(&r, &i16v));
    CHECK(fc_reader_get_u32(&r, &u32v));
    CHECK(fc_reader_get_i32(&r, &i32v));
    CHECK(fc_reader_get_u64(&r, &u64v));
    CHECK(fc_reader_get_i64(&r, &i64v));
    CHECK(fc_reader_get_f32(&r, &f32v));
    CHECK(fc_reader_get_f64(&r, &f64v));
    CHECK(u8v == 0xAB && i8v == -42 && u16v == 0x1234 && i16v == -1234);
    CHECK(u32v == 0x12345678u && i32v == -123456);
    CHECK(u64v == 0x0123456789ABCDEFull && i64v == -123456789);
    CHECK(f32v == 1.5f && f64v == -2.25);
    CHECK(fc_reader_remaining(&r) == 0);
    fc_buffer_free(&b);
    return 0;
}

static int test_wire_order(void) {
    fc_buffer b;
    fc_buffer_init(&b, 0);
    CHECK(fc_buffer_put_u32(&b, 0x01020304u));
    CHECK(b.len == 4 && b.data[0] == 1 && b.data[1] == 2 && b.data[2] == 3 && b.data[3] == 4);
    fc_buffer_reset(&b);
    b.little_endian = 1;
    CHECK(fc_buffer_put_u32(&b, 0x01020304u));
    CHECK(b.data[0] == 4 && b.data[3] == 1);
    fc_buffer_free(&b);
    return 0;
}

static int test_patch(void) {
    fc_buffer b;
    fc_buffer_init(&b, 0);
    CHECK(fc_buffer_put_u16(&b, 0));
    CHECK(fc_buffer_put_u32(&b, 99));
    CHECK(fc_buffer_patch_u16(&b, 0, 7));
    CHECK(b.len == 6 && b.data[0] == 0 && b.data[1] == 7 && b.data[5] == 99);
    CHECK(!fc_buffer_patch_u32(&b, 3, 1));
    fc_buffer_free(&b);
    return 0;
}

static int test_char_arrays(void) {
    fc_buffer b;
    fc_buffer_init(&b, 0);
    CHECK(fc_put_char_array(&b, "abc", 3, 6));
    CHECK(fc_put_char_array_pad(&b, "xy", 2, 4, '\0', 0));
    CHECK(fc_put_char_array_pad(&b, "xy", 2, 4, '0', 1));
    CHECK(fc_put_char_array(&b, "toolongstring", 13, 4)); /* truncated */

    fc_reader r;
    fc_reader_init(&r, b.data, b.len, 0);
    fc_string s;
    CHECK(fc_get_char_array(&r, 6, &s));
    CHECK(s.len == 3 && memcmp(s.data, "abc", 3) == 0);
    CHECK(fc_get_char_array_trim(&r, 4, '\0', 0, &s));
    CHECK(s.len == 2 && memcmp(s.data, "xy", 2) == 0);
    CHECK(fc_get_char_array_trim(&r, 4, '0', 1, &s));
    CHECK(s.len == 2 && memcmp(s.data, "xy", 2) == 0);
    CHECK(fc_get_char_array(&r, 4, &s));
    CHECK(s.len == 4 && memcmp(s.data, "tool", 4) == 0);
    CHECK(!fc_get_char_array(&r, 4, &s));
    fc_buffer_free(&b);
    return 0;
}

static int test_dynamic_string(void) {
    fc_buffer b;
    fc_buffer_init(&b, 0);
    CHECK(fc_put_string(&b, 2, "hello", 5));
    fc_reader r;
    fc_reader_init(&r, b.data, b.len, 0);
    fc_string s;
    CHECK(fc_get_string(&r, 2, &s));
    CHECK(s.len == 5 && memcmp(s.data, "hello", 5) == 0);

    fc_buffer bad;
    fc_buffer_init(&bad, 0);
    CHECK(fc_buffer_put_u16(&bad, 1));
    CHECK(fc_buffer_put_u8(&bad, 0xFF));
    fc_reader bad_r;
    fc_reader_init(&bad_r, bad.data, bad.len, 0);
    CHECK(!fc_get_string(&bad_r, 2, &s));
    fc_buffer_free(&b);
    fc_buffer_free(&bad);
    return 0;
}

static int test_counts(void) {
    fc_buffer b;
    fc_buffer_init(&b, 0);
    CHECK(fc_put_count(&b, 2, 300));
    CHECK(fc_put_count(&b, 4, 70000));
    fc_reader r;
    fc_reader_init(&r, b.data, b.len, 0);
    uint64_t n;
    CHECK(fc_get_count(&r, 2, &n) && n == 300);
    CHECK(fc_get_count(&r, 4, &n) && n == 70000);
    fc_buffer_free(&b);
    return 0;
}

static int test_checksums(void) {
    const uint8_t vec[] = "123456789";
    CHECK(fc_crc16(vec, 9) == 0x4B37);
    CHECK(fc_crc16(vec, 0) == 0xFFFF);
    CHECK(fc_crc32(vec, 9) == 0xCBF43926u);
    /* byte sum of "123456789" is 0x1DD, folded to the low 8 bits */
    CHECK(fc_sse_bin(vec, 9) == 0xDD);
    CHECK(fc_szse_bin(vec, 9) == 0xDD);
    CHECK(fc_sse_bin(vec, 0) == 0);
    return 0;
}

static int test_string_helpers(void) {
    CHECK(fc_string_eql(FC_STR("abc"), FC_STR("abc")));
    CHECK(!fc_string_eql(FC_STR("abc"), FC_STR("abd")));
    CHECK(fc_streq_lit(FC_STR("fast"), "fast"));
    CHECK(!fc_streq_lit(FC_STR("fastx"), "fast"));
    CHECK(fc_utf8_valid("hé-llo", 7));
    CHECK(!fc_utf8_valid("\xff", 1));
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_scalar_roundtrip(0);
    rc |= test_scalar_roundtrip(1);
    rc |= test_wire_order();
    rc |= test_patch();
    rc |= test_char_arrays();
    rc |= test_dynamic_string();
    rc |= test_counts();
    rc |= test_checksums();
    rc |= test_string_helpers();
    if (rc == 0) printf("all runtime tests passed\n");
    return rc;
}
