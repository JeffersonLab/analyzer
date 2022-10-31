###### Hall A Software database library SConscript Build File #####

from podd_util import build_library
Import('baseenv')

libname = 'PoddDB'
altname = 'Database'

src = """
Database.cxx
Textvars.cxx
VarType.cxx
"""

dbenv = baseenv.Clone()
dbenv.Replace(LIBS = [], RPATH=[])

# Database library
dblib = build_library(dbenv, libname, src,
                      extrahdrs = ['VarDef.h','Helper.h'],
                      dictname = altname,
                      versioned = True
                      )
