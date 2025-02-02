#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <immintrin.h>

#include "fast_purl.h"

// Compile-time string length checking
#define STATIC_STRLEN(s) (sizeof(s) - 1)


// Fast string copy using SIMD
static inline void fast_strcpy_16(char* __restrict dst, const char* __restrict src, size_t n) {
    __m128i* d = (__m128i*)dst;
    const __m128i* s = (const __m128i*)src;
    size_t blocks = n >> 4;
    
    for (size_t i = 0; i < blocks; i++) {
        _mm_store_si128(d + i, _mm_load_si128(s + i));
    }
    
    // Copy remaining bytes
    size_t remaining = n & 15;
    if (remaining) {
        memcpy(dst + (blocks << 4), src + (blocks << 4), remaining);
    }
}

// Find character using SIMD
static inline const char* fast_strchr(const char* s, int c, size_t max_len) {
    const __m128i needle = _mm_set1_epi8(c);
    
    // Check 16 bytes at a time
    while (max_len >= 16) {
        __m128i hay = _mm_loadu_si128((__m128i*)s);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(hay, needle));
        
        if (mask) {
            return s + __builtin_ctz(mask);
        }
        
        s += 16;
        max_len -= 16;
    }
    
    // Check remaining bytes
    while (max_len--) {
        if (*s == c) return s;
        s++;
    }
    
    return NULL;
}

// Parse URL into struct (returns length of processed URL)
__attribute__((hot))
size_t parse_postgres_url(const char* __restrict url, size_t len, PostgresUrl* __restrict out) {
    if (!url || !out || len < 11) return 0;
    
    const char* cursor = url;
    size_t remaining = len;
    out->flags = 0;
    
    // Quick protocol check using unaligned load
    uint64_t protocol;
    memcpy(&protocol, cursor, 8);
    if ((protocol & 0xFFFFFFFFFFFF) == 0x2F2F736572677470) {  // "postgres" in little-endian
        cursor += 11;  // Skip "postgres://"
        remaining -= 11;
    } else {
        cursor += 13;  // Skip "postgresql://"
        remaining -= 13;
    }
    
    // Find authentication section using SIMD
    const char* at = fast_strchr(cursor, '@', remaining);
    if (at) {
        size_t auth_len = at - cursor;
        const char* colon = fast_strchr(cursor, ':', auth_len);
        
        if (colon) {
            // Copy username
            size_t user_len = colon - cursor;
            fast_strcpy_16(out->user, cursor, user_len);
            out->user[user_len] = 0;
            out->flags |= PG_HAS_USER;
            
            // Copy password
            size_t pass_len = at - (colon + 1);
            fast_strcpy_16(out->pwd, colon + 1, pass_len);
            out->pwd[pass_len] = 0;
            out->flags |= PG_HAS_PASS;
        } else {
            // Username only
            fast_strcpy_16(out->user, cursor, auth_len);
            out->user[auth_len] = 0;
            out->flags |= PG_HAS_USER;
        }
        
        cursor = at + 1;
        remaining -= auth_len + 1;
    }
    
    // Parse host and port
    const char* slash = fast_strchr(cursor, '/', remaining);
    const char* colon = fast_strchr(cursor, ':', slash ? slash - cursor : remaining);
    
    if (colon && (!slash || colon < slash)) {
        // Host with port
        size_t host_len = colon - cursor;
        fast_strcpy_16(out->host, cursor, host_len);
        out->host[host_len] = 0;
        
        size_t port_len = slash ? (slash - (colon + 1)) : remaining - (colon - cursor + 1);
        memcpy(out->port, colon + 1, port_len);  // Port is small, regular memcpy is fine
        out->port[port_len] = 0;
        out->flags |= PG_HAS_PORT;
        
        cursor = slash ? slash : cursor + remaining;
        remaining = slash ? remaining - (slash - cursor) : 0;
    } else {
        // Host only
        size_t host_len = slash ? slash - cursor : remaining;
        fast_strcpy_16(out->host, cursor, host_len);
        out->host[host_len] = 0;
        memcpy(out->port, "5432", 5);  // Default port
        
        cursor = slash ? slash : cursor + remaining;
        remaining = slash ? remaining - (slash - cursor) : 0;
    }
    
    // Parse database and options if present
    if (remaining > 0) {
        cursor++;  // Skip slash
        remaining--;
        
        const char* question = fast_strchr(cursor, '?', remaining);
        if (question) {
            // Copy database name
            size_t db_len = question - cursor;
            fast_strcpy_16(out->dbname, cursor, db_len);
            out->dbname[db_len] = 0;
            out->flags |= PG_HAS_DB;
            
            // Copy options
            size_t opt_len = remaining - (question - cursor + 1);
            fast_strcpy_16(out->opts, question + 1, opt_len);
            out->opts[opt_len] = 0;
            out->flags |= PG_HAS_OPTS;
            
            // Quick SSL check using SIMD
            if (opt_len >= 7) {
                __m128i hay = _mm_loadu_si128((__m128i*)out->opts);
                __m128i needle = _mm_loadu_si128((__m128i*)"sslmode=require");
                if (_mm_movemask_epi8(_mm_cmpeq_epi8(hay, needle)) & 0x7F) {
                    out->flags |= PG_SSL_ENABLED;
                }
            }
        } else {
            // Database name only
            fast_strcpy_16(out->dbname, cursor, remaining);
            out->dbname[remaining] = 0;
            out->flags |= PG_HAS_DB;
        }
    }
    
    return len;
}

// Fast URL generation using SIMD
__attribute__((hot))
size_t postgres_url_to_string(const PostgresUrl* __restrict url, char* __restrict buffer) {
    char* cursor = buffer;
    
    // Copy protocol prefix using SIMD
    _mm_store_si128((__m128i*)cursor, _mm_loadu_si128((__m128i*)"postgresql://"));
    cursor += 13;
    
    // Add authentication if present
    if (url->flags & PG_HAS_USER) {
        size_t user_len = strlen(url->user);
        fast_strcpy_16(cursor, url->user, user_len);
        cursor += user_len;
        
        if (url->flags & PG_HAS_PASS) {
            *cursor++ = ':';
            size_t pass_len = strlen(url->pwd);
            fast_strcpy_16(cursor, url->pwd, pass_len);
            cursor += pass_len;
        }
        
        *cursor++ = '@';
    }
    
    // Add host
    size_t host_len = strlen(url->host);
    fast_strcpy_16(cursor, url->host, host_len);
    cursor += host_len;
    
    // Add port if not default
    if ((url->flags & PG_HAS_PORT) && memcmp(url->port, "5432", 4) != 0) {
        *cursor++ = ':';
        size_t port_len = strlen(url->port);
        memcpy(cursor, url->port, port_len);
        cursor += port_len;
    }
    
    // Add database if present
    if (url->flags & PG_HAS_DB) {
        *cursor++ = '/';
        size_t db_len = strlen(url->dbname);
        fast_strcpy_16(cursor, url->dbname, db_len);
        cursor += db_len;
    }
    
    // Add options if present
    if (url->flags & PG_HAS_OPTS) {
        *cursor++ = '?';
        size_t opt_len = strlen(url->opts);
        fast_strcpy_16(cursor, url->opts, opt_len);
        cursor += opt_len;
    }
    
    *cursor = 0;
    return cursor - buffer;
}