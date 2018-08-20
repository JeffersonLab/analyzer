cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME Podd)

#----------------------------------------------------------------------------
# Sources and headers (ls -w 96 -x *.cxx; macOS: COLUMNS=96 ls -x *.cxx)
set(src
  BankData.cxx            BdataLoc.cxx                 CodaRawDecoder.cxx
  DecData.cxx             FileInclude.cxx              FixedArrayVar.cxx
  MethodVar.cxx           SeqCollectionMethodVar.cxx   SeqCollectionVar.cxx
  SimDecoder.cxx          THaAnalysisObject.cxx        THaAnalyzer.cxx
  THaApparatus.cxx        THaArrayString.cxx           THaAvgVertex.cxx
  THaBeam.cxx             THaBeamDet.cxx               THaBeamEloss.cxx
  THaBeamInfo.cxx         THaBeamModule.cxx            THaBPM.cxx
  THaCherenkov.cxx        THaCluster.cxx               THaCodaRun.cxx
  THaCoincTime.cxx        THaCut.cxx                   THaCutList.cxx
  THaDebugModule.cxx      THaDetectorBase.cxx          THaDetector.cxx
  THaDetMap.cxx           THaElectronKine.cxx          THaElossCorrection.cxx
  THaEpicsEbeam.cxx       THaEpicsEvtHandler.cxx       THaEvent.cxx
  THaEvt125Handler.cxx    THaEvtTypeHandler.cxx        THaExtTarCor.cxx
  THaFilter.cxx           THaFormula.cxx               THaGoldenTrack.cxx
  THaHelicityDet.cxx      THaIdealBeam.cxx             THaInterface.cxx
  THaNamedList.cxx        THaNonTrackingDetector.cxx   THaOutput.cxx
  THaParticleInfo.cxx     THaPhotoReaction.cxx         THaPhysicsModule.cxx
  THaPidDetector.cxx      THaPIDinfo.cxx               THaPostProcess.cxx
  THaPrimaryKine.cxx      THaPrintOption.cxx           THaRaster.cxx
  THaRasteredBeam.cxx     THaReacPointFoil.cxx         THaReactionPoint.cxx
  THaRTTI.cxx             THaRunBase.cxx               THaRun.cxx
  THaRunParameters.cxx    THaSAProtonEP.cxx            THaScalerEvtHandler.cxx
  THaScintillator.cxx     THaSecondaryKine.cxx         THaShower.cxx
  THaSpectrometer.cxx     THaSpectrometerDetector.cxx  THaString.cxx
  THaSubDetector.cxx      THaTextvars.cxx              THaTotalShower.cxx
  THaTrack.cxx            THaTrackEloss.cxx            THaTrackID.cxx
  THaTrackInfo.cxx        THaTrackingDetector.cxx      THaTrackingModule.cxx
  THaTrackOut.cxx         THaTrackProj.cxx             THaTriggerTime.cxx
  THaTwoarmVertex.cxx     THaUnRasteredBeam.cxx        THaVar.cxx
  THaVarList.cxx          THaVertexModule.cxx          THaVform.cxx
  THaVhist.cxx            VariableArrayVar.cxx         Variable.cxx
  VectorObjMethodVar.cxx  VectorObjVar.cxx             VectorVar.cxx
  )
if(ONLINE_ET)
  list(APPEND src THaOnlRun.cxx)
endif()

string(REPLACE .cxx .h headers "${src}")
list(APPEND headers THaGlobals.h)
set(allheaders ${headers} Helper.h VarDef.h VarType.h)

#----------------------------------------------------------------------------
# Generate ha_compiledata.h

# Be sure there's no old ha_compiledata.h hanging around from a non-CMake build
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
    ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
if(WITH_DEBUG)
  target_compile_definitions(${LIBNAME} PUBLIC WITH_DEBUG)
endif()

target_link_libraries(${LIBNAME}
  PUBLIC
    ${PROJECT_NAME}::Decode
    ROOT::Libraries
  )
set_target_properties(${LIBNAME} PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME ${LIBNAME}
  )

install(TARGETS ${LIBNAME}
  EXPORT ${MAIN_PROJECT_NAME_LC}-exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES ${allheaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# libHallADict
build_root_dictionary(${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${LIBNAME}_LinkDef.h
  )
