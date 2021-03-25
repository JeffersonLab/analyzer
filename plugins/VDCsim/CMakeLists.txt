cmake_minimum_required(VERSION 3.5)

project(VDCsim LANGUAGES CXX)

set(PACKAGE VDCsim)
set(src THaVDCSim.cxx THaVDCSimDecoder.cxx THaVDCSimRun.cxx)
set(app vdcsimgen)
string(REPLACE .cxx .h headers "${src}")

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
add_library(${PACKAGE} SHARED ${src} ${headers} ${PACKAGE}Dict.cxx)

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
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${PACKAGE} ${headers}
  TARGETS ${PACKAGE}
  LINKDEF ${PACKAGE}_LinkDef.h
  )

#----------------------------------------------------------------------------
# Application
add_executable(${app} ${app}.cxx)

target_link_libraries(${app} PRIVATE ${PACKAGE})
target_compile_options(${app} PRIVATE ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST})

install(TARGETS ${app} DESTINATION ${CMAKE_INSTALL_BINDIR})
