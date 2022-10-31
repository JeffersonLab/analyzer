cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME Podd)

#----------------------------------------------------------------------------
# Sources and headers (ls -w 96 -x *.cxx; macOS: COLUMNS=96 ls -x *.cxx)
set(src
  BankData.cxx                 BdataLoc.cxx                 CodaRawDecoder.cxx
  DecData.cxx                  DetectorData.cxx             FileInclude.cxx
  FixedArrayVar.cxx            InterStageModule.cxx         MethodVar.cxx
  MultiFileRun.cxx             SeqCollectionMethodVar.cxx   SeqCollectionVar.cxx
  SimDecoder.cxx               THaAnalysisObject.cxx        THaAnalyzer.cxx
  THaApparatus.cxx             THaArrayString.cxx           THaAvgVertex.cxx
  THaBPM.cxx                   THaBeam.cxx                  THaBeamDet.cxx
  THaBeamEloss.cxx             THaBeamInfo.cxx              THaBeamModule.cxx
  THaCherenkov.cxx             THaCluster.cxx               THaCodaRun.cxx
  THaCoincTime.cxx             THaCut.cxx                   THaCutList.cxx
  THaDebugModule.cxx           THaDetMap.cxx                THaDetector.cxx
  THaDetectorBase.cxx          THaElectronKine.cxx          THaElossCorrection.cxx
  THaEpicsEbeam.cxx            THaEpicsEvtHandler.cxx       THaEvent.cxx
  THaEvt125Handler.cxx         THaEvtTypeHandler.cxx        THaExtTarCor.cxx
  THaFilter.cxx                THaFormula.cxx               THaGoldenTrack.cxx
  THaHelicityDet.cxx           THaIdealBeam.cxx             THaInterface.cxx
  THaNamedList.cxx             THaNonTrackingDetector.cxx   THaOutput.cxx
  THaPIDinfo.cxx               THaParticleInfo.cxx          THaPhotoReaction.cxx
  THaPhysicsModule.cxx         THaPidDetector.cxx           THaPostProcess.cxx
  THaPrimaryKine.cxx           THaPrintOption.cxx           THaRTTI.cxx
  THaRaster.cxx                THaRasteredBeam.cxx          THaReacPointFoil.cxx
  THaReactionPoint.cxx         THaRun.cxx                   THaRunBase.cxx
  THaRunParameters.cxx         THaSAProtonEP.cxx            THaScalerEvtHandler.cxx
  THaScintillator.cxx          THaSecondaryKine.cxx         THaShower.cxx
  THaSpectrometer.cxx          THaSpectrometerDetector.cxx  THaString.cxx
  THaSubDetector.cxx           THaTotalShower.cxx           THaTrack.cxx
  THaTrackEloss.cxx            THaTrackID.cxx               THaTrackInfo.cxx
  THaTrackOut.cxx              THaTrackProj.cxx             THaTrackingDetector.cxx
  THaTrackingModule.cxx        THaTriggerTime.cxx           THaTwoarmVertex.cxx
  THaUnRasteredBeam.cxx        THaVar.cxx                   THaVarList.cxx
  THaVertexModule.cxx          THaVform.cxx                 THaVhist.cxx
  TimeCorrectionModule.cxx     Variable.cxx                 VariableArrayVar.cxx
  VectorObjMethodVar.cxx       VectorObjVar.cxx             VectorVar.cxx
  )
if(ONLINE_ET)
  list(APPEND src THaOnlRun.cxx)
endif()

string(REPLACE .cxx .h headers "${src}")
list(APPEND headers THaGlobals.h)
set(allheaders ${headers} DataType.h OptionalType.h)
if(CMAKE_CXX_STANDARD LESS 17)
  list(APPEND allheaders optional.hpp)
endif()

#----------------------------------------------------------------------------
# Generate ha_compiledata.h

# Be sure there are no old metadata headers hanging around from a non-CMake build
file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/ha_compiledata.h")

include(PoddCompileInfo)
configure_file(ha_compiledata.h.in ha_compiledata.h)
list(APPEND allheaders "${CMAKE_CURRENT_BINARY_DIR}/ha_compiledata.h")

#----------------------------------------------------------------------------
# libPodd
add_library(${LIBNAME} SHARED ${src} ${allheaders} ${LIBNAME}Dict.cxx)
add_library(${PROJECT_NAME}::${LIBNAME} ALIAS ${LIBNAME})

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>  # for ha_compiledata.h
  )

target_compile_options(${LIBNAME}
  PUBLIC
    ${${MAIN_PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${MAIN_PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
if(WITH_DEBUG)
  target_compile_definitions(${LIBNAME} PUBLIC WITH_DEBUG)
endif()

target_link_libraries(${LIBNAME}
  PUBLIC
    ${PROJECT_NAME}::Decode
    Podd::Database
    ROOT::Libraries
  )
set_target_properties(${LIBNAME} PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME ${LIBNAME}
  )
add_dependencies(${LIBNAME} gitrev_${PROJECT_NAME})

install(TARGETS ${LIBNAME}
  EXPORT ${MAIN_PROJECT_NAME_LC}-exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES
  ${allheaders} "${CMAKE_CURRENT_BINARY_DIR}/git_description_${PROJECT_NAME}.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${LIBNAME}_LinkDef.h
  )
