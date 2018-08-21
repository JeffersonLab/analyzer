cmake_minimum_required(VERSION 3.5)

project(PoddSDK LANGUAGES CXX)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME User)

#----------------------------------------------------------------------------
# Sources and headers
set(src
  UserDetector.cxx
  UserModule.cxx
  UserApparatus.cxx
  UserScintillator.cxx
  UserEvtHandler.cxx
  SkeletonModule.cxx
  )

string(REPLACE .cxx .h headers "${src}")

#----------------------------------------------------------------------------
# Load Podd configuration, if needed
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
add_library(${LIBNAME} SHARED ${src} ${headers} ${LIBNAME}Dict.cxx)

target_compile_options(${LIBNAME} PRIVATE ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST})

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
target_link_libraries(${LIBNAME} PUBLIC Podd::HallA)

include(GNUInstallDirs)

install(TARGETS ${LIBNAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${LIBNAME}_LinkDef.h
  )
