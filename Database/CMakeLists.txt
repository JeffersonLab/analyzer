cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items built here
set(LIBNAME PoddDB)
set(SUBPROJECT_NAME Database)

#----------------------------------------------------------------------------
# Sources and headers
set(src Database.cxx Textvars.cxx VarType.cxx)

string(REPLACE .cxx .h headers "${src}")
set(allheaders ${headers} VarDef.h Helper.h)

#----------------------------------------------------------------------------
# libPoddDB
add_library(${LIBNAME} SHARED ${src} ${allheaders} ${LIBNAME}Dict.cxx)
add_library(${PROJECT_NAME}::${SUBPROJECT_NAME} ALIAS ${LIBNAME})

target_compile_options(${LIBNAME}
  PUBLIC
    ${${MAIN_PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${MAIN_PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
if(WITH_DEBUG)
  target_compile_definitions(${LIBNAME} PUBLIC WITH_DEBUG)
endif()

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
target_link_libraries(${LIBNAME}
  PUBLIC
    ROOT::Core
  )
set_target_properties(${LIBNAME} PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME ${SUBPROJECT_NAME}
  )

install(TARGETS ${LIBNAME}
  EXPORT ${MAIN_PROJECT_NAME_LC}-exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
install(FILES ${allheaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${SUBPROJECT_NAME}_LinkDef.h
  )

#----------------------------------------------------------------------------
