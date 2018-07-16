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
# Load Podd configuration, if needed. It finds its required dependencies
# in turn, including ROOT
if(NOT TARGET Podd::HallA)
  if(DEFINED ENV{ANALYZER})
    set(CMAKE_PREFIX_PATH "$ENV{ANALYZER}" "${CMAKE_PREFIX_PATH}")
  endif()

  find_package(Podd 1.7 REQUIRED)

endif()

set_diagnostic_flags(WALL WEXTRA)
#report_build_info()

#----------------------------------------------------------------------------
# Library
add_library(${LIBNAME} SHARED ${src} ${headers} ${LIBNAME}Dict.cxx)

target_compile_options(${LIBNAME} PRIVATE ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST})

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:include>
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
get_target_definitions(${LIBNAME} defines)
get_target_property(podd_includes Podd::HallA INTERFACE_INCLUDE_DIRECTORIES)
ROOT_GENERATE_DICTIONARY(${LIBNAME} ${defines} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${LIBNAME}_LinkDef.h
  )
set(PCM_FILE ${CMAKE_CURRENT_BINARY_DIR}/lib${LIBNAME}_rdict.pcm)

add_custom_target(${LIBNAME}_ROOTDICT
  DEPENDS ${src} ${headers} ${LIBNAME}_LinkDef.h ${LIBNAME}Dict.cxx)

install(FILES ${PCM_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
