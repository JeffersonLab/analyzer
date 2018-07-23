# Find ET library of JLab DAQ group
#
# Ole Hansen 4-Jul-2018

set(arch ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

if( DEFINED ENV{CODA} )
  find_library(ET_LIBRARY et
    PATHS $ENV{CODA}/${arch}/lib
	  $ENV{CODA}/${CMAKE_SYSTEM_NAME}/lib
    DOC "Event Transport (ET) library"
    )
  find_path(ET_INCLUDE_DIR
    NAMES et.h
    PATHS $ENV{CODA}/common/include
    DOC "Event Transport (ET) header include directory"
    )
endif()

if(NOT TARGET EVIO::ET AND ET_LIBRARY AND ET_INCLUDE_DIR)
  add_library(EVIO::ET SHARED IMPORTED)
  set_target_properties(EVIO::ET PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${ET_INCLUDE_DIR}"
    IMPORTED_LOCATION "${ET_LIBRARY}"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ET ET_INCLUDE_DIR ET_LIBRARY)
