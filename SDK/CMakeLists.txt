cmake_minimum_required(VERSION 3.5)

project(PoddSDK LANGUAGES CXX)

#----------------------------------------------------------------------------
# Names of source files and target libraries.
# You do want to modify this section.

# The name of your package.
# The generated shared library will be named lib$(PACKAGE).so/dylib
set(PACKAGE User)

# Sources.
# List all your source files here. They will be put into a shared library
# that can be loaded from a script.
# List only the implementation files (*.cxx). For every implementation file
# there must be a corresponding header file (*.h).
set(src
  UserDetector.cxx
  UserModule.cxx
  UserApparatus.cxx
  UserScintillator.cxx
  UserEvtHandler.cxx
  SkeletonModule.cxx
  )

# Headers.
# Leave the following line untouched.
string(REPLACE .cxx .h headers "${src}")

# If you have any extra headers needed for building the ROOT dictionary,
# add them here. Otherwise leave this line commented out.
#list(APPEND headers UserClassHeader.h)

# If you have other headers NOT needed for the ROOT dictionary but needed for
# the library, add them here as shown in the line commented out below.
set(allheaders ${headers})
#set(allheaders ${headers} StandaloneHeader1.h StandaloneHeader2.h)

#------------------------------------------------------------------------------
# Do not change anything below here unless you know what you are doing

#----------------------------------------------------------------------------
# Find Podd, if necessary, and load its CMake module
if(NOT TARGET Podd::HallA)
  if(DEFINED ENV{ANALYZER})
    set(CMAKE_PREFIX_PATH "$ENV{ANALYZER}" "${CMAKE_PREFIX_PATH}")
  endif()

  find_package(Podd 1.7 REQUIRED)
endif()
include(PoddCMakeEnv)

set_diagnostic_flags(WALL WEXTRA)
#report_build_info()

#----------------------------------------------------------------------------
# Library
add_library(${PACKAGE} SHARED ${src} ${allheaders} ${PACKAGE}Dict.cxx)

target_compile_options(${PACKAGE} PRIVATE ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST})

target_include_directories(${PACKAGE}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
target_link_libraries(${PACKAGE} PUBLIC Podd::HallA)

include(GNUInstallDirs)

install(TARGETS ${PACKAGE}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES ${allheaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${PACKAGE} ${headers}
  TARGETS ${PACKAGE}
  LINKDEF ${PACKAGE}_LinkDef.h
  )
