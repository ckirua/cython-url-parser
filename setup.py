# setup.py
from Cython.Build import cythonize
from setuptools import Extension, setup

ext_modules = [
    Extension(
        "postgres_url",
        sources=["postgres_url.pyx", "fast_purl.c"],
        extra_compile_args=["-O3", "-march=native", "-mavx2"],
        extra_link_args=["-O3"],
    )
]

setup(
    name="postgres_url",
    ext_modules=cythonize(
        ext_modules,
        compiler_directives={
            "language_level": 3,
            "boundscheck": False,
            "wraparound": False,
            "initializedcheck": False,
            "nonecheck": False,
            "cdivision": True,
        },
        build_dir="src",
    ),
)
