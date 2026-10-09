#include "binary_codec.h"

#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * strings
 * ------------------------------------------------------------------------- */

bool fc_string_eql(fc_string a, fc_string b) {
    if (a.len != b.len) return false;
    if (a.len == 0) return true;
    return memcmp(a.data, b.data, a.len) == 0;
}

bool fc_streq_lit(fc_string a, const char *lit) {
    size_t lit_len = strlen(lit);
    if (a.len != lit_len) return false;
    if (lit_len == 0) return true;
    return memcmp(a.data, lit, lit_len) == 0;
}

bool fc_utf8_valid(const char *data, size_t len) {
    size_t i = 0;
    while (i < len) {
        uint8_t c = (uint8_t)data[i];
        size_t cont; /* continuation bytes expected */
        uint32_t min_cp;
        if (c < 0x80) {
            i += 1;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            cont = 1;
            min_cp = 0x80;
        } else if ((c & 0xF0) == 0xE0) {
            cont = 2;
            min_cp = 0x800;
        } else if ((c & 0xF8) == 0xF0) {
            cont = 3;
            min_cp = 0x10000;
        } else {
            return false;
        }
        if (cont >= len - i) return false; /* need lead + `cont` continuation bytes */
        uint32_t cp = 0;
        switch (cont) {
        case 1:
            cp = (uint32_t)(c & 0x1F);
            break;
        case 2:
            cp = (uint32_t)(c & 0x0F);
            break;
        default:
            cp = (uint32_t)(c & 0x07);
            break;
        }
        for (size_t k = 1; k <= cont; k++) {
            uint8_t cc = (uint8_t)data[i + k];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (uint32_t)(cc & 0x3F);
        }
        if (cp < min_cp) return false;
        if (cp > 0x10FFFF) return false;
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        i += cont + 1;
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * buffer
 * ------------------------------------------------------------------------- */

void fc_buffer_init(fc_buffer *b, int little_endian) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->little_endian = little_endian;
}

void fc_buffer_free(fc_buffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void fc_buffer_reset(fc_buffer *b) { b->len = 0; }

bool fc_buffer_reserve(fc_buffer *b, size_t extra) {
    size_t needed = b->len + extra;
    if (needed <= b->cap) return true;
    size_t cap = b->cap == 0 ? 64 : b->cap;
    while (cap < needed) {
        if (cap > (SIZE_MAX / 2)) {
            cap = needed;
            break;
        }
        cap *= 2;
    }
    uint8_t *grown = (uint8_t *)realloc(b->data, cap);
    if (grown == NULL) return false;
    b->data = grown;
    b->cap = cap;
    return true;
}

bool fc_buffer_write(fc_buffer *b, const void *bytes, size_t n) {
    if (!fc_buffer_reserve(b, n)) return false;
    memcpy(b->data + b->len, bytes, n);
    b->len += n;
    return true;
}

bool fc_buffer_put_u8(fc_buffer *b, uint8_t v) {
    if (!fc_buffer_reserve(b, 1)) return false;
    b->data[b->len++] = v;
    return true;
}

bool fc_buffer_put_u16(fc_buffer *b, uint16_t v) {
    if (!fc_buffer_reserve(b, 2)) return false;
    uint8_t *p = b->data + b->len;
    if (b->little_endian) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
    } else {
        p[0] = (uint8_t)(v >> 8);
        p[1] = (uint8_t)(v);
    }
    b->len += 2;
    return true;
}

bool fc_buffer_put_u32(fc_buffer *b, uint32_t v) {
    if (!fc_buffer_reserve(b, 4)) return false;
    uint8_t *p = b->data + b->len;
    if (b->little_endian) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16);
        p[3] = (uint8_t)(v >> 24);
    } else {
        p[0] = (uint8_t)(v >> 24);
        p[1] = (uint8_t)(v >> 16);
        p[2] = (uint8_t)(v >> 8);
        p[3] = (uint8_t)(v);
    }
    b->len += 4;
    return true;
}

bool fc_buffer_put_u64(fc_buffer *b, uint64_t v) {
    if (!fc_buffer_reserve(b, 8)) return false;
    uint8_t *p = b->data + b->len;
    for (int i = 0; i < 8; i++) {
        int shift = b->little_endian ? i * 8 : (7 - i) * 8;
        p[i] = (uint8_t)(v >> shift);
    }
    b->len += 8;
    return true;
}

bool fc_buffer_put_i8(fc_buffer *b, int8_t v) { return fc_buffer_put_u8(b, (uint8_t)v); }
bool fc_buffer_put_i16(fc_buffer *b, int16_t v) { return fc_buffer_put_u16(b, (uint16_t)v); }
bool fc_buffer_put_i32(fc_buffer *b, int32_t v) { return fc_buffer_put_u32(b, (uint32_t)v); }
bool fc_buffer_put_i64(fc_buffer *b, int64_t v) { return fc_buffer_put_u64(b, (uint64_t)v); }

bool fc_buffer_put_f32(fc_buffer *b, float v) {
    uint32_t bits;
    memcpy(&bits, &v, 4);
    return fc_buffer_put_u32(b, bits);
}

bool fc_buffer_put_f64(fc_buffer *b, double v) {
    uint64_t bits;
    memcpy(&bits, &v, 8);
    return fc_buffer_put_u64(b, bits);
}

bool fc_buffer_patch_u16(fc_buffer *b, size_t pos, uint16_t v) {
    if (pos + 2 > b->len) return false;
    uint8_t *p = b->data + pos;
    if (b->little_endian) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
    } else {
        p[0] = (uint8_t)(v >> 8);
        p[1] = (uint8_t)(v);
    }
    return true;
}

bool fc_buffer_patch_u32(fc_buffer *b, size_t pos, uint32_t v) {
    if (pos + 4 > b->len) return false;
    uint8_t *p = b->data + pos;
    if (b->little_endian) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16);
        p[3] = (uint8_t)(v >> 24);
    } else {
        p[0] = (uint8_t)(v >> 24);
        p[1] = (uint8_t)(v >> 16);
        p[2] = (uint8_t)(v >> 8);
        p[3] = (uint8_t)(v);
    }
    return true;
}

bool fc_buffer_patch_u64(fc_buffer *b, size_t pos, uint64_t v) {
    if (pos + 8 > b->len) return false;
    uint8_t *p = b->data + pos;
    for (int i = 0; i < 8; i++) {
        int shift = b->little_endian ? i * 8 : (7 - i) * 8;
        p[i] = (uint8_t)(v >> shift);
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * reader
 * ------------------------------------------------------------------------- */

void fc_reader_init(fc_reader *r, const uint8_t *data, size_t len, int little_endian) {
    r->data = data;
    r->len = len;
    r->pos = 0;
    r->little_endian = little_endian;
}

size_t fc_reader_remaining(const fc_reader *r) { return r->len - r->pos; }

bool fc_reader_take(fc_reader *r, size_t n, const uint8_t **out) {
    if (fc_reader_remaining(r) < n) return false;
    *out = r->data + r->pos;
    r->pos += n;
    return true;
}

bool fc_reader_get_u8(fc_reader *r, uint8_t *out) {
    const uint8_t *p;
    if (!fc_reader_take(r, 1, &p)) return false;
    *out = p[0];
    return true;
}

bool fc_reader_get_u16(fc_reader *r, uint16_t *out) {
    const uint8_t *p;
    if (!fc_reader_take(r, 2, &p)) return false;
    if (r->little_endian)
        *out = (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    else
        *out = (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
    return true;
}

bool fc_reader_get_u32(fc_reader *r, uint32_t *out) {
    const uint8_t *p;
    if (!fc_reader_take(r, 4, &p)) return false;
    if (r->little_endian)
        *out = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
               ((uint32_t)p[3] << 24);
    else
        *out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
               (uint32_t)p[3];
    return true;
}

bool fc_reader_get_u64(fc_reader *r, uint64_t *out) {
    const uint8_t *p;
    if (!fc_reader_take(r, 8, &p)) return false;
    uint64_t v = 0;
    if (r->little_endian) {
        for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
    } else {
        for (int i = 0; i < 8; i++) v = (v << 8) | p[i];
    }
    *out = v;
    return true;
}

bool fc_reader_get_i8(fc_reader *r, int8_t *out) {
    uint8_t v;
    if (!fc_reader_get_u8(r, &v)) return false;
    *out = (int8_t)v;
    return true;
}

bool fc_reader_get_i16(fc_reader *r, int16_t *out) {
    uint16_t v;
    if (!fc_reader_get_u16(r, &v)) return false;
    *out = (int16_t)v;
    return true;
}

bool fc_reader_get_i32(fc_reader *r, int32_t *out) {
    uint32_t v;
    if (!fc_reader_get_u32(r, &v)) return false;
    *out = (int32_t)v;
    return true;
}

bool fc_reader_get_i64(fc_reader *r, int64_t *out) {
    uint64_t v;
    if (!fc_reader_get_u64(r, &v)) return false;
    *out = (int64_t)v;
    return true;
}

bool fc_reader_get_f32(fc_reader *r, float *out) {
    uint32_t bits;
    if (!fc_reader_get_u32(r, &bits)) return false;
    memcpy(out, &bits, 4);
    return true;
}

bool fc_reader_get_f64(fc_reader *r, double *out) {
    uint64_t bits;
    if (!fc_reader_get_u64(r, &bits)) return false;
    memcpy(out, &bits, 8);
    return true;
}

/* ---------------------------------------------------------------------------
 * fixed char arrays
 * ------------------------------------------------------------------------- */

bool fc_put_char_array_pad(fc_buffer *b, const char *s, size_t s_len, size_t fixed_len,
                           char pad_char, int pad_left) {
    size_t content = s_len < fixed_len ? s_len : fixed_len;
    if (pad_left) {
        for (size_t i = content; i < fixed_len; i++) {
            if (!fc_buffer_put_u8(b, (uint8_t)pad_char)) return false;
        }
        return fc_buffer_write(b, s, content);
    }
    if (!fc_buffer_write(b, s, content)) return false;
    for (size_t i = content; i < fixed_len; i++) {
        if (!fc_buffer_put_u8(b, (uint8_t)pad_char)) return false;
    }
    return true;
}

bool fc_put_char_array(fc_buffer *b, const char *s, size_t s_len, size_t fixed_len) {
    return fc_put_char_array_pad(b, s, s_len, fixed_len, ' ', 0);
}

static fc_string fc_trim(const uint8_t *raw, size_t len, char pad_char, int pad_left) {
    fc_string out;
    if (pad_left) {
        size_t start = 0;
        while (start < len && (char)raw[start] == pad_char) start++;
        out.data = (const char *)raw + start;
        out.len = len - start;
    } else {
        size_t end = len;
        while (end > 0 && (char)raw[end - 1] == pad_char) end--;
        out.data = (const char *)raw;
        out.len = end;
    }
    return out;
}

bool fc_get_char_array_trim(fc_reader *r, size_t fixed_len, char pad_char, int pad_left,
                            fc_string *out) {
    const uint8_t *raw;
    if (!fc_reader_take(r, fixed_len, &raw)) return false;
    *out = fc_trim(raw, fixed_len, pad_char, pad_left);
    return true;
}

bool fc_get_char_array(fc_reader *r, size_t fixed_len, fc_string *out) {
    return fc_get_char_array_trim(r, fixed_len, ' ', 0, out);
}

/* ---------------------------------------------------------------------------
 * dynamic strings
 * ------------------------------------------------------------------------- */

bool fc_put_count(fc_buffer *b, int lbytes, size_t count) {
    switch (lbytes) {
    case 1:
        return fc_buffer_put_u8(b, (uint8_t)count);
    case 2:
        return fc_buffer_put_u16(b, (uint16_t)count);
    case 4:
        return fc_buffer_put_u32(b, (uint32_t)count);
    case 8:
        return fc_buffer_put_u64(b, (uint64_t)count);
    default:
        return false;
    }
}

bool fc_get_count(fc_reader *r, int lbytes, uint64_t *count) {
    switch (lbytes) {
    case 1: {
        uint8_t v;
        if (!fc_reader_get_u8(r, &v)) return false;
        *count = v;
        return true;
    }
    case 2: {
        uint16_t v;
        if (!fc_reader_get_u16(r, &v)) return false;
        *count = v;
        return true;
    }
    case 4: {
        uint32_t v;
        if (!fc_reader_get_u32(r, &v)) return false;
        *count = v;
        return true;
    }
    case 8:
        return fc_reader_get_u64(r, count);
    default:
        return false;
    }
}

bool fc_put_string(fc_buffer *b, int lbytes, const char *s, size_t s_len) {
    if (!fc_put_count(b, lbytes, s_len)) return false;
    return fc_buffer_write(b, s, s_len);
}

bool fc_get_string(fc_reader *r, int lbytes, fc_string *out) {
    uint64_t n;
    if (!fc_get_count(r, lbytes, &n)) return false;
    const uint8_t *raw;
    if (!fc_reader_take(r, (size_t)n, &raw)) return false;
    if (!fc_utf8_valid((const char *)raw, (size_t)n)) return false;
    out->data = (const char *)raw;
    out->len = (size_t)n;
    return true;
}
