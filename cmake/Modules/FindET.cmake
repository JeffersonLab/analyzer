# Find ET library of JLab DAQ group
#
# Ole Hansen 4-Jul-2018

set(arch ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(et_dbg et-dbg)
endif()
if( DEFINED ENV{CODA} )
  set(coda_arch_lib $ENV{CODA}/${arch}/lib $ENV{CODA}/${CMAKE_SYSTEM_NAME}/lib)
  set(coda_arch_inc $ENV{CODA}/${arch}/include $ENV{CODA}/common/include)
endif()
# $ENV{CODA} path is searched first, then all other locations
find_library(ET_LIBRARY ${et_dbg} et
  PATHS ENV ET_LIBDIR ${coda_arch_lib} NO_DEFAULT_PATH
  DOC "Event Transport (ET) library"
)
find_library(ET_LIBRARY ${et_dbg} et
  DOC "Event Transport (ET) library"
)
find_path(ET_INCLUDE_DIR
    NAMES et.h
    PATHS ENV ET_INCDIR ${coda_arch_inc} NO_DEFAULT_PATH
    DOC "Event Transport (ET) header include directory"
)
find_path(ET_INCLUDE_DIR
    NAMES et.h
    DOC "Event Transport (ET) header include directory"
)
if(ET_INCLUDE_DIR)
  set(VERSION_REGEX "#define[ \t]+ET_VERSION[ \t]+([^ ]+)")
  file(STRINGS "${ET_INCLUDE_DIR}/et_private.h" VERSION_STRING REGEX "${VERSION_REGEX}")
  string(REGEX REPLACE "${VERSION_REGEX}.*" "\\1" ET_VERSION_MAJOR "${VERSION_STRING}")
  set(VERSION_REGEX "#define[ \t]+ET_VERSION_MINOR[ \t]+([^ ]+)")
  file(STRINGS "${ET_INCLUDE_DIR}/et_private.h" VERSION_STRING REGEX "${VERSION_REGEX}")
  if(VERSION_STRING)
    string(REGEX REPLACE "${VERSION_REGEX}.*" "\\1" ET_VERSION_MINOR "${VERSION_STRING}")
  else()
    set(ET_VERSION_MINOR "0")
  endif()
  set(ET_VERSION "${ET_VERSION_MAJOR}.${ET_VERSION_MINOR}")
endif()

if(NOT TARGET ET::ET AND ET_LIBRARY AND ET_INCLUDE_DIR)
  add_library(ET::ET SHARED IMPORTED)
  set_target_properties(ET::ET PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${ET_INCLUDE_DIR}"
    IMPORTED_LOCATION "${ET_LIBRARY}"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ET
  REQUIRED_VARS ET_LIBRARY ET_INCLUDE_DIR
  VERSION_VAR ET_VERSION
)
