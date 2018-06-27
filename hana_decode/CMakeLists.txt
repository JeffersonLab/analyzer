cmake_minimum_required(VERSION 3.5)

set(haDecode_src 
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

set(headers
  Caen1190Module.h Caen775Module.h Caen792Module.h CodaDecoder.h Decoder.h
  et.h F1TDCModule.h Fadc250Module.h FastbusModule.h GenScaler.h Lecroy1875Module.h
  Lecroy1877Module.h Lecroy1881Module.h Module.h PipeliningModule.h Scaler1151.h Scaler3800.h
  Scaler3801.h Scaler560.h SimDecoder.h THaBenchmark.h THaCodaData.h THaCodaFile.h
  THaCrateMap.h THaEpics.h THaEtClient.h THaEvData.h THaGenDetTest.h
  THaSlotData.h THaUsrstrutils.h VmeModule.h
  )

ROOT_GENERATE_DICTIONARY(haDecodeDict ${headers} 
  INCLUDEDIRS include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src 
  LINKDEF haDecode_LinkDef.h  
  OPTIONS -p)
set(PCM_FILE  ${CMAKE_CURRENT_BINARY_DIR}/haDecodeDict_rdict.pcm)

ADD_CUSTOM_TARGET(haDecode_ROOTDICTS DEPENDS ${haDecode_src} ${headers} haDecode_LinkDef.h haDecodeDict.cxx)

add_library(dc STATIC
  ${haDecode_src}
  haDecodeDict.cxx
  )
add_library(PODD::Decode ALIAS dc)

#Set target properties
target_include_directories(dc
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)
#target_compile_features(dc PRIVATE cxx_auto_type)
#target_compile_options( dc PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

target_link_libraries(dc
  #PUBLIC ${EXPAT_LIBRARIES}
  #PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC ${ROOT_LIBRARIES} 
  PUBLIC EVIO::EVIO
  PRIVATE PODD::HallA
)
#include(GNUInstallDirs)
#set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/JSONUtils)

install(TARGETS dc 
  EXPORT podd-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
set_target_properties(dc PROPERTIES EXPORT_NAME Decode)

#This is required so that the exported target has the name JSONUtils and not jsonutils
#set_target_properties(HallA PROPERTIES EXPORT_NAME halla)

install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/podd)
install(FILES ${PCM_FILE} DESTINATION lib)

#Export the targets to a script
install(EXPORT podd-targets
  FILE
  PODDTargets.cmake
  NAMESPACE
  PODD::
  DESTINATION
    lib/podd
)
