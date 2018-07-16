cmake_minimum_required(VERSION 3.5)

project(Podd VERSION 1.7.0 LANGUAGES CXX)
set(EXTVERS .3-devel)

#----------------------------------------------------------------------------
# Use our modules and utility functions
set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake/Modules
  ${CMAKE_MODULE_PATH}
  )
include(PoddCMakeEnv)

#----------------------------------------------------------------------------
# Build utility programs by default unless we are a submodule
if("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  option(${PROJECT_NAME_UC}_BUILD_UTILS "Build utility programs" ON)
else()
  option(${PROJECT_NAME_UC}_BUILD_UTILS "Build utility programs" OFF)
endif()

#----------------------------------------------------------------------------
# Install in GNU-style directory layout
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME_LC})

#----------------------------------------------------------------------------
# Find ROOT (using our custom FindROOT.cmake)
find_package(ROOT 5.10 REQUIRED)

#----------------------------------------------------------------------------
# Set up the compiler flags
set_compiler_flags("${ROOT_CXX_FLAGS}")
set_diagnostic_flags(WALL WEXTRA)
report_build_info()

#----------------------------------------------------------------------------
# Directories in which we build things
add_subdirectory(hana_decode)
add_subdirectory(src)
if(${PROJECT_NAME_UC}_BUILD_UTILS)
  add_subdirectory(utils)
endif()
add_subdirectory(cmake)

#----------------------------------------------------------------------------
# Install and export targets defined in the subdirectories
install(EXPORT ${PROJECT_NAME_LC}-exports
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${INSTALL_CONFIGDIR}
  )

#----------------------------------------------------------------------------
# Install additional components
install(DIRECTORY scripts/ DESTINATION scripts
  USE_SOURCE_PERMISSIONS
  PATTERN "*.C"
  PATTERN "*.sh"
  )
