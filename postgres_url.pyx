# File: postgres_url.pyx
# distutils: language = c
# cython: language_level=3
# cython: boundscheck=False
# cython: wraparound=False
# cython: cdivision=True
# cython: nonecheck=False

from libc.string cimport memcpy, strlen, memset
from cpython.bytes cimport PyBytes_FromStringAndSize
from libc.stdlib cimport malloc, free

# Add at the top of your .pyx file
cdef extern from "fast_purl.h":
    pass  # The struct is already defined in .pxd

cdef class URL:
    cdef PostgresUrl c_url
    cdef char* buffer

    def __cinit__(self):
        self.buffer = <char*>malloc(512)  # Fixed buffer for URL generation
        if not self.buffer:
            raise MemoryError()
        memset(&self.c_url, 0, sizeof(PostgresUrl))
        
    def __dealloc__(self):
        if self.buffer:
            free(self.buffer)

    @staticmethod
    def parse(str url_string):
        """Parse a PostgreSQL URL string into a URL object."""
        cdef URL url = URL()
        py_bytes = url_string.encode('utf8')
        cdef const char* c_str = py_bytes
        cdef size_t length = len(py_bytes)
        
        with nogil:
            if parse_postgres_url(c_str, length, &url.c_url) == 0:
                with gil:
                    raise ValueError("Failed to parse URL")
        
        return url

    def to_string(self):
        """Convert URL object back to string representation."""
        cdef size_t length
        with nogil:
            length = postgres_url_to_string(&self.c_url, self.buffer)
        return self.buffer[:length].decode('utf8')

    @property
    def host(self):
        return self.c_url.host.decode('utf8')

    @host.setter
    def host(self, str value):
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 64:
            raise ValueError("Host name too long (max 63 characters)")
        memset(self.c_url.host, 0, 64)
        memcpy(self.c_url.host, <char*>py_bytes, len(py_bytes))

    @property
    def port(self):
        return self.c_url.port.decode('utf8')

    @port.setter
    def port(self, str value):
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 8:
            raise ValueError("Port too long (max 7 characters)")
        memset(self.c_url.port, 0, 8)
        memcpy(self.c_url.port, <char*>py_bytes, len(py_bytes))
        self.c_url.flags |= 0x4  # Set has_port flag

    @property
    def database(self):
        if self.c_url.flags & 0x8:  # Has database flag
            return self.c_url.dbname.decode('utf8')
        return None

    @database.setter
    def database(self, str value):
        if value is None:
            memset(self.c_url.dbname, 0, 64)
            self.c_url.flags &= ~0x8  # Clear has_database flag
            return
        
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 64:
            raise ValueError("Database name too long (max 63 characters)")
        memset(self.c_url.dbname, 0, 64)
        memcpy(self.c_url.dbname, <char*>py_bytes, len(py_bytes))
        self.c_url.flags |= 0x8  # Set has_database flag

    @property
    def username(self):
        if self.c_url.flags & 0x1:  # Has user flag
            return self.c_url.user.decode('utf8')
        return None

    @username.setter
    def username(self, str value):
        if value is None:
            memset(self.c_url.user, 0, 32)
            self.c_url.flags &= ~0x1  # Clear has_user flag
            return
            
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 32:
            raise ValueError("Username too long (max 31 characters)")
        memset(self.c_url.user, 0, 32)
        memcpy(self.c_url.user, <char*>py_bytes, len(py_bytes))
        self.c_url.flags |= 0x1  # Set has_user flag

    @property
    def password(self):
        if self.c_url.flags & 0x2:  # Has password flag
            return self.c_url.pwd.decode('utf8')
        return None

    @password.setter
    def password(self, str value):
        if value is None:
            memset(self.c_url.pwd, 0, 32)
            self.c_url.flags &= ~0x2  # Clear has_password flag
            return
            
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 32:
            raise ValueError("Password too long (max 31 characters)")
        memset(self.c_url.pwd, 0, 32)
        memcpy(self.c_url.pwd, <char*>py_bytes, len(py_bytes))
        self.c_url.flags |= 0x2  # Set has_password flag

    @property
    def options(self):
        if self.c_url.flags & 0x10:  # Has options flag
            return self.c_url.opts.decode('utf8')
        return None

    @options.setter
    def options(self, str value):
        if value is None:
            memset(self.c_url.opts, 0, 128)
            self.c_url.flags &= ~0x10  # Clear has_options flag
            return
            
        py_bytes = value.encode('utf8')
        if len(py_bytes) >= 128:
            raise ValueError("Options string too long (max 127 characters)")
        memset(self.c_url.opts, 0, 128)
        memcpy(self.c_url.opts, <char*>py_bytes, len(py_bytes))
        self.c_url.flags |= 0x10  # Set has_options flag

    @property
    def ssl_enabled(self):
        return bool(self.c_url.flags & 0x20)  # SSL flag

    @ssl_enabled.setter
    def ssl_enabled(self, value):
        if value:
            self.c_url.flags |= 0x20  # Set SSL flag
        else:
            self.c_url.flags &= ~0x20  # Clear SSL flag