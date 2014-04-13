from distutils.core import setup, Extension
setup(name="allocstats", version="0.1",
      ext_modules=[Extension("allocstats", ["allocstatsmodule.c"])])

