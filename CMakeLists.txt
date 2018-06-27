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
set(INSTALL_CONFIGDIR lib/podd)

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

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/src/ha_compiledata.h
  DESTINATION include/podd
  )

##############################################
# Installation instructions

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)

# Install and export targets
install(EXPORT podd-targets
  FILE PODDTargets.cmake
  NAMESPACE PODD::
  DESTINATION lib/podd
  )

set(TARGETS_INSTALL_PATH lib/podd/PODDTargets.cmake)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/PODDConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PODDConfig.cmake
    INSTALL_DESTINATION lib/podd
    PATH_VARS TARGETS_INSTALL_PATH
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
  )


#Install the config, configversion and custom find modules
install(FILES
  #    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PODDConfigVersion.cmake
  DESTINATION lib/podd
)

##############################################
## Exporting from the build tree
#configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
#    ${CMAKE_CURRENT_BINARY_DIR}/FindRapidJSON.cmake
#    COPYONLY)

export(EXPORT podd-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/PODDTargets.cmake NAMESPACE PODD::)

#Register package in user's package registry
export(PACKAGE PODD) 
