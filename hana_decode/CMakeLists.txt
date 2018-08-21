cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items built here
set(LIBNAME dc)
set(ALTLIBNAME haDecode)
set(SUBPROJECT_NAME Decode)

#----------------------------------------------------------------------------
# Configuration options
option(ONLINE_ET "Enable support ET message system" OFF)

#----------------------------------------------------------------------------
# Required dependencies
find_package(EVIO CONFIG QUIET)
if(NOT EVIO_FOUND)
  find_package(EVIO MODULE)
  if(NOT EVIO_FOUND)
    include(FetchBuildEVIO)
  endif()
endif()

if(ONLINE_ET)
  find_package(ET)
endif()

#----------------------------------------------------------------------------
# Sources and headers
set(src
  Caen1190Module.cxx
  Caen775Module.cxx
  Caen792Module.cxx
  CodaDecoder.cxx
  F1TDCModule.cxx
  Fadc250Module.cxx
  FastbusModule.cxx
  GenScaler.cxx
  Lecroy1875Module.cxx
  Lecroy1877Module.cxx
  Lecroy1881Module.cxx
  Module.cxx
  PipeliningModule.cxx
  Scaler1151.cxx
  Scaler3800.cxx
  Scaler3801.cxx
  Scaler560.cxx
  SimDecoder.cxx
  THaCodaData.cxx
  THaCodaFile.cxx
  THaCrateMap.cxx
  THaEpics.cxx
  THaEvData.cxx
  THaSlotData.cxx
  THaUsrstrutils.cxx
  VmeModule.cxx
  )
if(ONLINE_ET)
  if(ET_FOUND)
  # no longer needed?
  #  find_library(IEEE_LIBRARY ieee)
  #  set(ONLIBS ${ET_LIBRARY} ${IEEE_LIBRARY})
  #  set(ONLIBS ${ET_LIBRARY})
    set(ONLIBS EVIO::ET)
    list(APPEND src THaEtClient.cxx)
  else()
    message(WARNING
      "Cannot find ET system. Check $CODA environment variable. Disabling ONLINE_ET")
    set(ONLINE_ET FALSE)
  endif()
endif()

string(REPLACE .cxx .h headers "${src}")
list(APPEND headers THaBenchmark.h)
set(allheaders ${headers} Decoder.h)

#----------------------------------------------------------------------------
# libdc
add_library(${LIBNAME} SHARED ${src} ${allheaders} lib${LIBNAME}Dict.cxx)
add_library(${PROJECT_NAME}::${SUBPROJECT_NAME} ALIAS ${LIBNAME})

target_compile_options(${LIBNAME}
  PUBLIC
    ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
if(WITH_DEBUG)
  target_compile_definitions(${LIBNAME} PUBLIC WITH_DEBUG)
endif()
if(ONLINE_ET)
  target_compile_definitions(${LIBNAME} PUBLIC ONLINE_ET)
endif()

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
  )
target_link_libraries(${LIBNAME}
  PUBLIC
    ROOT::Core
  PRIVATE
    EVIO::EVIO
    ${ONLIBS}
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
# libdcDict dictionary
build_root_dictionary(lib${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  PCMNAME ${LIBNAME}
  LINKDEF ${ALTLIBNAME}_LinkDef.h
  )
