# File: postgres_url.pxd
cdef extern from "fast_purl.h":
    ctypedef struct PostgresUrl:
        char host[64]
        char port[8]
        char dbname[64]
        char user[32]
        char pwd[32]  # pass is a Python keyword
        char opts[128]
        unsigned int flags

    size_t parse_postgres_url(const char* url, size_t len, PostgresUrl* out) nogil
    size_t postgres_url_to_string(const PostgresUrl* url, char* buffer) nogil
