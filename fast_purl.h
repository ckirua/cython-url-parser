#pragma once

#include <stdint.h>
#include <stddef.h>

// Cache line aligned struct (assumes 64-byte cache lines)
// These defines size of url instance
typedef struct __attribute__((aligned(64))) {
    char host[64]     __attribute__((aligned(16)));
    char port[8]      __attribute__((aligned(8)));
    char dbname[64]   __attribute__((aligned(16)));
    char user[32]     __attribute__((aligned(16)));
    char pwd[32]     __attribute__((aligned(16)));
    char opts[128]    __attribute__((aligned(16)));
    uint32_t flags;   // Bitflags for features
} PostgresUrl;

// Bitflags for URL features
enum {
    PG_HAS_USER     = 1 << 0,
    PG_HAS_PASS     = 1 << 1,
    PG_HAS_PORT     = 1 << 2,
    PG_HAS_DB       = 1 << 3,
    PG_HAS_OPTS     = 1 << 4,
    PG_SSL_ENABLED  = 1 << 5
};

// Public function declarations
size_t parse_postgres_url(const char* url, size_t len, PostgresUrl* out);
size_t postgres_url_to_string(const PostgresUrl* url, char* buffer);
