cmake_minimum_required(VERSION 3.5)

project(Podd VERSION 1.7.0 LANGUAGES CXX)
set(EXTVERS .5-devel)

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
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/${MAIN_PROJECT_NAME_LC})

#----------------------------------------------------------------------------
# Find ROOT (using our custom FindROOT.cmake)
set(minimum_root_version 5.10)
find_package(ROOT ${minimum_root_version} REQUIRED)
# Register this dependency - it's in our public interface
config_add_dependency(ROOT ${minimum_root_version})

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
# Install additional components
install(DIRECTORY scripts/ DESTINATION scripts
  USE_SOURCE_PERMISSIONS
  PATTERN "*.C"
  PATTERN "*.sh"
  )
