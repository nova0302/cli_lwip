/* b64_min.c --- 
 * 
 * Filename: b64_min.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Mar  5 20:24:18 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Thu Mar  5 20:27:08 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 4
 * URL: 
 * Doc URL: 
 * Keywords: 
 * Compatibility: 
 * 
 */

/* Commentary: 
 * 
 * 
 * 
 */

/* Change Log:
 * 
 * 
 */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Code: */
/*
 * b64_min.c ? tiny Base64 encoder/decoder for embedded/bare?metal
 *
 * License: MIT-0 (No Attribution)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>

/* -----------------------------------------------------------------------------
   Configuration
   ----------------------------------------------------------------------------- */

/* Define B64_URL_SAFE to use URL-safe alphabet: '-' and '_' instead of '+' and '/' */
/* #define B64_URL_SAFE */

/* -----------------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------------- */

/* Returns required output size (not including terminator) for Base64 encoding of src_len. */
size_t b64_encoded_size(size_t src_len)
{
    /* 4 chars for every 3 bytes, padded up */
    return ((src_len + 2u) / 3u) * 4u;
}

/* A safe upper bound of decoded size for a Base64 string of length src_len. */
size_t b64_max_decoded_size(size_t src_len)
{
    /* Worst case: ignoring whitespace, every 4 chars -> 3 bytes */
    return (src_len / 4u) * 3u + 3u; /* +3 for padding/missing padding tolerance */
}

/* Encode 'src' into Base64.
 * - dst: output buffer (no null-terminator is written by default).
 * - dst_cap: capacity in bytes; must be >= b64_encoded_size(src_len).
 * - out_len: set to number of bytes written (may be NULL).
 * Returns 0 on success, non-zero on error (e.g., insufficient dst_cap).
 */
int b64_encode(const void *src, size_t src_len, char *dst, size_t dst_cap, size_t *out_len);

/* Decode Base64 string 'src' into raw bytes.
 * Accepts optional padding ('=') and ignores ASCII whitespace (spaces/tabs/CR/LF).
 * Also tolerates missing padding at the end.
 * - dst: output buffer; capacity must be >= b64_max_decoded_size(src_len) or exact bound.
 * - out_len: set to decoded byte count (may be NULL).
 * Returns 0 on success, non-zero on invalid input or insufficient dst_cap.
 */
int b64_decode(const char *src, size_t src_len, void *dst, size_t dst_cap, size_t *out_len);

/* -----------------------------------------------------------------------------
   Implementation
   ----------------------------------------------------------------------------- */

static inline char b64_enc_char(uint8_t x)
{
#ifdef B64_URL_SAFE
    /* URL-safe alphabet */
    static const char T[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_";
#else
    static const char T[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/";
#endif
    return T[x & 63u];
}

/* Return 0..63 for valid Base64 char, 64 for '=', 0xFF for invalid/non-data.
 * Whitespace (space, tab, CR, LF) returns 0xFE to signal "skip".
 */
static inline uint8_t b64_dec_val(char c)
{
    /* Whitespace that we skip */
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return 0xFE;

#ifdef B64_URL_SAFE
    if (c == '-') return 62;
    if (c == '_') return 63;
#else
    if (c == '+') return 62;
    if (c == '/') return 63;
#endif
    if (c == '=') return 64; /* padding */

    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');          /* 0..25 */
    if (c >= 'a' && c <= 'z') return (uint8_t)(26 + c - 'a');     /* 26..51 */
    if (c >= '0' && c <= '9') return (uint8_t)(52 + c - '0');     /* 52..61 */

    return 0xFF; /* invalid */
}

int b64_encode(const void *src, size_t len, char *dst, size_t dst_cap, size_t *out_len)
{
    const uint8_t *s = (const uint8_t *)src;
    size_t need = b64_encoded_size(len);
    size_t di = 0;

    if (dst_cap < need) return -1;

    for (size_t i = 0; i + 2 < len; i += 3) {
        uint32_t v = ((uint32_t)s[i] << 16) | ((uint32_t)s[i+1] << 8) | s[i+2];
        dst[di++] = b64_enc_char((uint8_t)(v >> 18));
        dst[di++] = b64_enc_char((uint8_t)(v >> 12));
        dst[di++] = b64_enc_char((uint8_t)(v >> 6));
        dst[di++] = b64_enc_char((uint8_t)(v));
    }

    size_t rem = len % 3u;
    if (rem == 1u) {
        uint32_t v = (uint32_t)s[len - 1] << 16;
        dst[di++] = b64_enc_char((uint8_t)(v >> 18));
        dst[di++] = b64_enc_char((uint8_t)(v >> 12));
        dst[di++] = '=';
        dst[di++] = '=';
    } else if (rem == 2u) {
        uint32_t v = ((uint32_t)s[len - 2] << 16) | ((uint32_t)s[len - 1] << 8);
        dst[di++] = b64_enc_char((uint8_t)(v >> 18));
        dst[di++] = b64_enc_char((uint8_t)(v >> 12));
        dst[di++] = b64_enc_char((uint8_t)(v >> 6));
        dst[di++] = '=';
    }

    if (out_len) *out_len = di;
    return 0;
}

int b64_decode(const char *src, size_t src_len, void *dst, size_t dst_cap, size_t *out_len)
{
    uint8_t *d = (uint8_t *)dst;
    size_t di = 0;

    uint8_t q[4];
    int qn = 0;

    for (size_t i = 0; i < src_len; ++i) {
        uint8_t v = b64_dec_val(src[i]);
        if (v == 0xFE) continue;       /* skip whitespace */
        if (v == 0xFF) return -2;      /* invalid char */

        q[qn++] = v;

        if (qn == 4) {
            if (q[0] > 63u || q[1] > 63u) return -3;

            if (q[2] == 64u && q[3] == 64u) {
                /* xx== : 1 byte */
                if (di + 1 > dst_cap) return -4;
                d[di++] = (uint8_t)((q[0] << 2) | (q[1] >> 4));
                qn = 0;
                /* nothing should follow except whitespace */
            } else if (q[3] == 64u) {
                /* xxx= : 2 bytes */
                if (q[2] > 63u) return -3;
                if (di + 2 > dst_cap) return -4;
                d[di++] = (uint8_t)((q[0] << 2) | (q[1] >> 4));
                d[di++] = (uint8_t)(((q[1] & 0x0F) << 4) | (q[2] >> 2));
                qn = 0;
            } else {
                /* xxxx : 3 bytes */
                if (q[2] > 63u || q[3] > 63u) return -3;
                if (di + 3 > dst_cap) return -4;
                d[di++] = (uint8_t)((q[0] << 2) | (q[1] >> 4));
                d[di++] = (uint8_t)(((q[1] & 0x0F) << 4) | (q[2] >> 2));
                d[di++] = (uint8_t)(((q[2] & 0x03) << 6) | q[3]);
                qn = 0;
            }
        }
    }

    /* Handle missing padding at end: allow 2 or 3 sextets */
    if (qn != 0) {
        if (qn == 2) {
            if (q[0] > 63u || q[1] > 63u) return -3;
            if (di + 1 > dst_cap) return -4;
            d[di++] = (uint8_t)((q[0] << 2) | (q[1] >> 4));
        } else if (qn == 3) {
            if (q[0] > 63u || q[1] > 63u || q[2] > 63u) return -3;
            if (di + 2 > dst_cap) return -4;
            d[di++] = (uint8_t)((q[0] << 2) | (q[1] >> 4));
            d[di++] = (uint8_t)(((q[1] & 0x0F) << 4) | (q[2] >> 2));
        } else {
            return -5; /* qn==1 is invalid */
        }
    }

    if (out_len) *out_len = di;
    return 0;
}


/* b64_min.c ends here */
