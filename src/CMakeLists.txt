cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME HallA)
set(EXENAME analyzer)

#----------------------------------------------------------------------------
# Sources and headers
set(src
  THaADCHelicity.cxx          THaCoincTime.cxx            THaG0Helicity.cxx
  THaPhotoReaction.cxx        THaRunParameters.cxx        THaTrackID.cxx
  THaVDCTrackID.cxx           THaAnalysisObject.cxx       THaCut.cxx
  THaG0HelicityReader.cxx     THaPhysicsModule.cxx        THaS2CoincTime.cxx
  THaTrackInfo.cxx            THaAnalyzer.cxx             THaCutList.cxx
  THaGoldenTrack.cxx          THaPidDetector.cxx          THaSAProtonEP.cxx
  THaTrackOut.cxx             THaVDCPointPair.cxx         THaApparatus.cxx
  THaDebugModule.cxx          THaHRS.cxx                  THaVDCPoint.cxx
  THaPostProcess.cxx          THaTrackProj.cxx            THaEpicsEvtHandler.cxx
  THaVDCChamber.cxx           THaArrayString.cxx          THaDecData.cxx
  BdataLoc.cxx                THaHelicity.cxx             THaPrimaryKine.cxx
  THaScintillator.cxx         THaTrackingDetector.cxx     THaVDCWire.cxx
  THaAvgVertex.cxx            THaDetMap.cxx               THaHelicityDet.cxx
  THaPrintOption.cxx          THaSecondaryKine.cxx        THaTrackingModule.cxx
  THaVar.cxx                  THaBPM.cxx                  THaDetector.cxx
  THaIdealBeam.cxx            THaQWEAKHelicity.cxx        THaShower.cxx
  THaTriggerTime.cxx          THaVarList.cxx              THaBeam.cxx
  THaDetectorBase.cxx         THaInterface.cxx
  THaQWEAKHelicityReader.cxx
  THaSpectrometer.cxx         THaTwoarmVertex.cxx         THaVertexModule.cxx
  THaBeamDet.cxx              THaElectronKine.cxx         THaNamedList.cxx
  THaRTTI.cxx                 THaSpectrometerDetector.cxx THaUnRasteredBeam.cxx
  THaVform.cxx                THaBeamEloss.cxx            THaElossCorrection.cxx
  THaNonTrackingDetector.cxx  THaRaster.cxx               THaString.cxx
  THaVDC.cxx                  THaVhist.cxx                THaBeamInfo.cxx
  THaEpicsEbeam.cxx           THaRasteredBeam.cxx         THaSubDetector.cxx
  THaVDCAnalyticTTDConv.cxx   VDCeff.cxx                  THaBeamModule.cxx
  THaEvent.cxx                FileInclude.cxx             THaReacPointFoil.cxx
  THaTextvars.cxx             THaVDCCluster.cxx           THaCherenkov.cxx
  THaExtTarCor.cxx            THaOutput.cxx               THaReactionPoint.cxx
  THaTotalShower.cxx          THaVDCHit.cxx               THaCluster.cxx
  THaFilter.cxx               THaPIDinfo.cxx              THaRun.cxx
  THaTrack.cxx                THaVDCPlane.cxx             THaCodaRun.cxx
  THaFormula.cxx              THaParticleInfo.cxx         THaRunBase.cxx
  THaTrackEloss.cxx           THaVDCTimeToDistConv.cxx    THaEvtTypeHandler.cxx
  THaScalerEvtHandler.cxx     THaEvt125Handler.cxx        Variable.cxx
  VariableArrayVar.cxx        FixedArrayVar.cxx           VectorVar.cxx
  MethodVar.cxx               SeqCollectionVar.cxx
  SeqCollectionMethodVar.cxx  VectorObjVar.cxx            VectorObjMethodVar.cxx
  BankData.cxx
  )

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
# libHallA
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

#----------------------------------------------------------------------------
# analyzer executable

# Only build this if this is not a subproject
if("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")

  add_executable(${EXENAME} main.cxx)

  target_link_libraries(${EXENAME}
    PRIVATE
      ${LIBNAME}
    )
  target_compile_options(${EXENAME}
    PUBLIC
      ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
    PRIVATE
      ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
    )
  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    # Linux (at least with g++) requires -fPIC even for main, as we found
    # out the hard way (see commit 20cf746)
    target_compile_options(${EXENAME} PUBLIC -fPIC)
  endif()

  install(TARGETS ${EXENAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()
