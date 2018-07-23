# Find EVIO library of JLab DAQ group (C API)
#
# Ole Hansen 28-Jun-2018

set(evio_arch ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

if( DEFINED ENV{EVIO} )
  set(evio_arch_lib $ENV{EVIO}/${evio_arch}/lib $ENV{EVIO}/${CMAKE_SYSTEM_NAME}/lib)
  set(evio_arch_inc $ENV{EVIO}/${evio_arch}/include $ENV{EVIO}/common/include)
endif()
if( DEFINED ENV{CODA} )
  set(coda_arch_lib $ENV{CODA}/${evio_arch}/lib $ENV{CODA}/${CMAKE_SYSTEM_NAME}/lib)
  set(coda_arch_inc $ENV{CODA}/${evio_arch}/include $ENV{CODA}/common/include)
endif()

find_library(EVIO_LIBRARY evio
  PATHS ENV EVIO_LIBDIR ${evio_arch_lib} ${coda_arch_lib} NO_DEFAULT_PATHS
  DOC "EVIO C-API library"
  )
find_library(EVIO_LIBRARY evio
  DOC "EVIO C-API library"
  )
find_path(EVIO_INCLUDE_DIR
  NAMES evio.h
  PATHS ENV EVIO_INCDIR ${evio_arch_inc} ${coda_arch_inc} NO_DEFAULT_PATHS
  DOC "EVIO C-API header include directory"
  )
find_path(EVIO_INCLUDE_DIR
  NAMES evio.h
  DOC "EVIO C-API header include directory"
  )
if(EVIO_INCLUDE_DIR)
  set(VERSION_REGEX "#define[ \t]+EV_VERSION[ \t]+([^ ]+)")
  file(STRINGS "${EVIO_INCLUDE_DIR}/evio.h" VERSION_STRING REGEX "${VERSION_REGEX}")
  string(REGEX REPLACE "${VERSION_REGEX}" "\\1" EVIO_VERSION "${VERSION_STRING}")
endif()
if(NOT TARGET EVIO::EVIO AND EVIO_LIBRARY AND EVIO_INCLUDE_DIR)
  add_library(EVIO::EVIO SHARED IMPORTED)
  set_target_properties(EVIO::EVIO PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EVIO_INCLUDE_DIR}"
    IMPORTED_LOCATION "${EVIO_LIBRARY}"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EVIO
  REQUIRED_VARS EVIO_LIBRARY EVIO_INCLUDE_DIR
  VERSION_VAR EVIO_VERSION
  )
