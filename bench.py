# benchmark.py
import sys
import timeit
from typing import Optional


# Pure Python implementation with __slots__
class PythonURL:
    __slots__ = (
        "host",
        "port",
        "database",
        "username",
        "password",
        "options",
        "ssl_enabled",
    )

    def __init__(self):
        self.host: str = ""
        self.port: str = "5432"
        self.database: Optional[str] = None
        self.username: Optional[str] = None
        self.password: Optional[str] = None
        self.options: Optional[str] = None
        self.ssl_enabled: bool = False

    @staticmethod
    def parse(url: str) -> "PythonURL":
        url_obj = PythonURL()

        # Skip protocol
        if url.startswith("postgresql://"):
            url = url[13:]
        elif url.startswith("postgres://"):
            url = url[11:]

        # Split auth, host, and path
        remaining = url
        path_part = ""

        if "@" in remaining:
            auth, remaining = remaining.split("@", 1)
            if ":" in auth:
                url_obj.username, url_obj.password = auth.split(":", 1)
            else:
                url_obj.username = auth

        if "/" in remaining:
            remaining, path_part = remaining.split("/", 1)

        # Parse host/port
        if ":" in remaining:
            url_obj.host, url_obj.port = remaining.split(":", 1)
        else:
            url_obj.host = remaining

        # Parse path and options
        if path_part:
            if "?" in path_part:
                db, url_obj.options = path_part.split("?", 1)
                url_obj.database = db
                if "sslmode=require" in url_obj.options:
                    url_obj.ssl_enabled = True
            else:
                url_obj.database = path_part

        return url_obj

    def to_string(self) -> str:
        parts = ["postgresql://"]

        # Add authentication
        if self.username:
            parts.append(self.username)
            if self.password:
                parts.extend([":", self.password])
            parts.append("@")

        # Add host and port
        parts.append(self.host)
        if self.port != "5432":
            parts.extend([":", self.port])

        # Add database and options
        if self.database:
            parts.extend(["/", self.database])
        if self.options:
            parts.extend(["?", self.options])

        return "".join(parts)


# Import Cython version
from postgres_url import URL as CythonURL

# Benchmark data
URLS = [
    "postgresql://localhost",
    "postgresql://user:pass@localhost:5432/mydb",
    "postgresql://complex:pass@host.domain.com:5433/bigdb?sslmode=require&timeout=30",
]


def get_class_size(cls):
    """Get memory size of a class in bytes"""
    return sys.getsizeof(cls)


def benchmark_python_create(url):
    return PythonURL.parse(url)


def benchmark_cython_create(url):
    return CythonURL.parse(url)


def benchmark_python_tostring(obj):
    return obj.to_string()


def benchmark_cython_tostring(obj):
    return obj.to_string()


# Run benchmarks
def run_benchmark():
    print("Running benchmarks...")

    # Get class sizes
    py_class_size = get_class_size(PythonURL)
    cy_class_size = get_class_size(CythonURL)

    print("\nClass Memory Sizes:")
    print(f"PythonURL class: {py_class_size} bytes")
    print(f"CythonURL class: {cy_class_size} bytes")

    for url in URLS:
        print(f"\nURL: {url}")

        # Create objects for benchmarks
        py_obj = PythonURL.parse(url)
        cy_obj = CythonURL.parse(url)

        # Get instance sizes
        py_instance_size = sys.getsizeof(py_obj)
        cy_instance_size = sys.getsizeof(cy_obj)

        print(f"\nInstance Memory Sizes:")
        print(f"Python URL instance: {py_instance_size} bytes")
        print(f"Cython URL instance: {cy_instance_size} bytes")

        print("\nURL parsing:")
        # Python create benchmark
        python_create_time = timeit.timeit(
            lambda: benchmark_python_create(url), number=100000
        )

        # Cython create benchmark
        cython_create_time = timeit.timeit(
            lambda: benchmark_cython_create(url), number=100000
        )

        print(f"Python create time: {python_create_time:.4f}s")
        print(f"Cython create time: {cython_create_time:.4f}s")
        print(f"Create speedup: {python_create_time/cython_create_time:.2f}x")

        print("\nURL string generation:")
        # Python to_string benchmark
        python_tostring_time = timeit.timeit(
            lambda: benchmark_python_tostring(py_obj), number=100000
        )

        # Cython to_string benchmark
        cython_tostring_time = timeit.timeit(
            lambda: benchmark_cython_tostring(cy_obj), number=100000
        )

        print(f"Python to_string time: {python_tostring_time:.4f}s")
        print(f"Cython to_string time: {cython_tostring_time:.4f}s")
        print(
            f"To string speedup: {python_tostring_time/cython_tostring_time:.2f}x"
        )

        print("\nHost access:")
        # Python host property benchmark
        python_host_time = timeit.timeit(lambda: py_obj.host, number=100000)

        # Cython host property benchmark
        cython_host_time = timeit.timeit(lambda: cy_obj.host, number=100000)

        print(f"Python host access time: {python_host_time:.4f}s")
        print(f"Cython host access time: {cython_host_time:.4f}s")
        print(f"Host access speedup: {python_host_time/cython_host_time:.2f}x")

        print("\nHost modification:")
        # Python host setter benchmark
        python_host_set_time = timeit.timeit(
            lambda: setattr(py_obj, "host", "newhost.com"), number=100000
        )

        # Cython host setter benchmark
        cython_host_set_time = timeit.timeit(
            lambda: setattr(cy_obj, "host", "newhost.com"), number=100000
        )

        print(f"Python host set time: {python_host_set_time:.4f}s")
        print(f"Cython host set time: {cython_host_set_time:.4f}s")
        print(
            f"Host set speedup: {python_host_set_time/cython_host_set_time:.2f}x"
        )

        print("\nDatabase access:")
        # Python database property benchmark
        python_db_time = timeit.timeit(lambda: py_obj.database, number=100000)

        # Cython database property benchmark
        cython_db_time = timeit.timeit(lambda: cy_obj.database, number=100000)

        print(f"Python database access time: {python_db_time:.4f}s")
        print(f"Cython database access time: {cython_db_time:.4f}s")
        print(f"Database access speedup: {python_db_time/cython_db_time:.2f}x")


if __name__ == "__main__":
    run_benchmark()
