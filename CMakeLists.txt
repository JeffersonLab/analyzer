cmake_minimum_required(VERSION 3.5)

project(Podd VERSION 1.7.6 LANGUAGES CXX)
set(EXTVERS)

#----------------------------------------------------------------------------
# Use our modules and utility functions
list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake/Modules")
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
include(PoddCMakeEnv)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  #--------------------------------------------------------------------------
  # Install in GNU-style directory layout
  include(GNUInstallDirs)
  set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/${MAIN_PROJECT_NAME_LC}")
  if(PODD_SET_RPATH)
    set_install_rpath()
  endif()

  #--------------------------------------------------------------------------
  # Find ROOT (using our custom FindROOT.cmake)
  set(minimum_root_version 6.00)
  find_package(ROOT ${minimum_root_version} REQUIRED)
  # Register this dependency - it's in our public interface
  config_add_dependency(ROOT ${minimum_root_version})

  #--------------------------------------------------------------------------
  # Set up the compiler flags
  set_compiler_flags("${ROOT_CXX_FLAGS}")
  set_diagnostic_flags(WALL WEXTRA)
  report_build_info()
endif()

#----------------------------------------------------------------------------
# Directories in which we build things
add_subdirectory(Database)
add_subdirectory(hana_decode)
add_subdirectory(Podd)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_subdirectory(HallA)
  add_subdirectory(apps)
endif()
add_subdirectory(cmake)

#----------------------------------------------------------------------------
# Install support files
# install(DIRECTORY scripts/ DESTINATION scripts
#   USE_SOURCE_PERMISSIONS
#   PATTERN "*.C"
#   PATTERN "*.sh"
#   )
