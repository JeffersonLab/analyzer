cmake_minimum_required(VERSION 3.5)
project(PODD VERSION 1.6.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH
   ${CMAKE_MODULE_PATH}
   ${PROJECT_SOURCE_DIR}/cmake
   ${PROJECT_SOURCE_DIR}/cmake/modules)

## Use C++17 features 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## OS Specific settings
include(cmake/os.cmake)
# provides variables for header ha_compiledata.h
include(cmake/modules/CompileTimeInfo.cmake)

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/PODD)

find_package(EXPAT REQUIRED COMPONENTS EXPAT )
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

find_package(EVIO REQUIRED)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS}/etc/cmake)
find_package(ROOT REQUIRED COMPONENTS Tree)
include(${ROOT_USE_FILE})
# extra root settings
include_directories(${ROOT_INCLUDE_DIRS})
link_directories(${ROOT_LIBRARY_DIR})
include(cmake/root.cmake)

# this helps with the circular headers
include_directories(src)


configure_file(ha_compiledata.h.in 
  src/ha_compiledata.h)

add_subdirectory(hana_decode)
add_subdirectory(src)
#add_subdirectory(src/libsrc++)
#add_subdirectory(src/execsrc)
#src/examples
#src/test

##############################################
# Installation instructions

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/PODDConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PODDConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config, configversion and custom find modules
install(FILES
  #    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfigVersion.cmake
  DESTINATION ${INSTALL_CONFIGDIR}
)

##############################################
## Exporting from the build tree
#configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
#    ${CMAKE_CURRENT_BINARY_DIR}/FindRapidJSON.cmake
#    COPYONLY)

export(EXPORT PODD-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/PODDTargets.cmake NAMESPACE PODD::)

#Register package in user's package registry
export(PACKAGE PODD) 
