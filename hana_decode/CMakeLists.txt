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
  Caen1190Module.C
  Caen775Module.C
  Caen792Module.C
  CodaDecoder.C
  F1TDCModule.C
  Fadc250Module.C
  FastbusModule.C
  GenScaler.C
  Lecroy1875Module.C
  Lecroy1877Module.C
  Lecroy1881Module.C
  Module.C
  PipeliningModule.C
  Scaler1151.C
  Scaler3800.C
  Scaler3801.C
  Scaler560.C
  SimDecoder.C
  THaCodaData.C
  THaCodaDecoder.C
  THaCodaFile.C
  THaCrateMap.C
  THaEpics.C
  THaEvData.C
  THaFastBusWord.C
  THaSlotData.C
  THaUsrstrutils.C
  VmeModule.C
  )
if(ONLINE_ET)
  if(ET_FOUND)
  # no longer needed?
  #  find_library(IEEE_LIBRARY ieee)
  #  set(ONLIBS ${ET_LIBRARY} ${IEEE_LIBRARY})
  #  set(ONLIBS ${ET_LIBRARY})
    set(ONLIBS EVIO::ET)
    list(APPEND src THaEtClient.C)
  else()
    message(WARNING
      "Cannot find ET system. Check $CODA environment variable. Disabling ONLINE_ET")
    set(ONLINE_ET FALSE)
  endif()
endif()

string(REPLACE .C .h headers "${src}")
list(APPEND headers THaBenchmark.h)
set(allheaders ${headers} Decoder.h)

#----------------------------------------------------------------------------
# libdc
add_library(${LIBNAME} SHARED ${src} ${allheaders} lib${LIBNAME}Dict.C)
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

#----------------------------------------------------------------------------
# Standalone test/example executables
if(STANDALONE)
  add_executable(epicsd epics_main.C)
  add_executable(prfact prfact_main.C)
  add_executable(tdecex tdecex_main.C THaGenDetTest.C)
  add_executable(tdecpr tdecpr_main.C)
  add_executable(tst1190 tst1190_main.C)
  add_executable(tstf1tdc tstf1tdc_main.C)
  add_executable(tstfadc tstfadc_main.C)
  add_executable(tstfadcblk tstfadcblk_main.C)
  add_executable(tstio tstio_main.C)
  add_executable(tstoo tstoo_main.C)

  set(allexe epicsd prfact tdecex tdecpr tst1190 tstf1tdc
    tstfadc tstfadcblk tstio tstoo
    )

  if(ONLINE_ET)
    add_executable(etclient etclient_main.C)
    add_executable(tstcoda tstcoda_main.C)
    list(APPEND allexe etclient tstcoda)
  endif()


  foreach(exe IN LISTS allexe)
    target_link_libraries(${exe}
      PRIVATE
      Podd::HallA
      Podd::Decode
      EVIO::EVIO
      ${ONLIBS}
      ROOT::Libraries
      )
    target_compile_options(${exe}
      PRIVATE
      ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
      )
  endforeach()

  install(TARGETS ${allexe}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
