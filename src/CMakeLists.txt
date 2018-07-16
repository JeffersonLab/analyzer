cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME HallA)
set(EXENAME analyzer)

#----------------------------------------------------------------------------
# Sources and headers
set(src
  THaADCHelicity.C          THaCoincTime.C            THaG0Helicity.C
  THaPhotoReaction.C        THaRunParameters.C        THaTrackID.C
  THaVDCTrackID.C           THaAnalysisObject.C       THaCut.C
  THaG0HelicityReader.C     THaPhysicsModule.C        THaS2CoincTime.C
  THaTrackInfo.C            THaAnalyzer.C             THaCutList.C
  THaGoldenTrack.C          THaPidDetector.C          THaSAProtonEP.C
  THaTrackOut.C             THaVDCPointPair.C         THaApparatus.C
  THaDebugModule.C          THaHRS.C                  THaVDCPoint.C
  THaPostProcess.C          THaTrackProj.C            THaEpicsEvtHandler.C
  THaVDCChamber.C           THaArrayString.C          THaDecData.C
  BdataLoc.C                THaHelicity.C             THaPrimaryKine.C
  THaScintillator.C         THaTrackingDetector.C     THaVDCWire.C
  THaAvgVertex.C            THaDetMap.C               THaHelicityDet.C
  THaPrintOption.C          THaSecondaryKine.C        THaTrackingModule.C
  THaVar.C                  THaBPM.C                  THaDetector.C
  THaIdealBeam.C            THaQWEAKHelicity.C        THaShower.C
  THaTriggerTime.C          THaVarList.C              THaBeam.C
  THaDetectorBase.C         THaInterface.C
  THaQWEAKHelicityReader.C
  THaSpectrometer.C         THaTwoarmVertex.C         THaVertexModule.C
  THaBeamDet.C              THaElectronKine.C         THaNamedList.C
  THaRTTI.C                 THaSpectrometerDetector.C THaUnRasteredBeam.C
  THaVform.C                THaBeamEloss.C            THaElossCorrection.C
  THaNonTrackingDetector.C  THaRaster.C               THaString.C
  THaVDC.C                  THaVhist.C                THaBeamInfo.C
  THaEpicsEbeam.C           THaRasteredBeam.C         THaSubDetector.C
  THaVDCAnalyticTTDConv.C   VDCeff.C                  THaBeamModule.C
  THaEvent.C                FileInclude.C             THaReacPointFoil.C
  THaTextvars.C             THaVDCCluster.C           THaCherenkov.C
  THaExtTarCor.C            THaOutput.C               THaReactionPoint.C
  THaTotalShower.C          THaVDCHit.C               THaCluster.C
  THaFilter.C               THaPIDinfo.C              THaRun.C
  THaTrack.C                THaVDCPlane.C             THaCodaRun.C
  THaFormula.C              THaParticleInfo.C         THaRunBase.C
  THaTrackEloss.C           THaVDCTimeToDistConv.C    THaEvtTypeHandler.C
  THaScalerEvtHandler.C     THaEvt125Handler.C        Variable.C
  VariableArrayVar.C        FixedArrayVar.C           VectorVar.C
  MethodVar.C               SeqCollectionVar.C
  SeqCollectionMethodVar.C  VectorObjVar.C            VectorObjMethodVar.C
  ExtraData.C
  )

string(REPLACE .C .h headers "${src}")
list(APPEND headers THaGlobals.h)
set(allheaders ${headers} VarDef.h VarType.h)

#----------------------------------------------------------------------------
# Generate ha_compiledata.h

# Be sure there's no old ha_compiledata.h hanging around from a non-CMake build
file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/ha_compiledata.h")

include(PoddCompileInfo)
configure_file(ha_compiledata.h.in ha_compiledata.h)
list(APPEND allheaders "${CMAKE_CURRENT_BINARY_DIR}/ha_compiledata.h")

#----------------------------------------------------------------------------
# libHallA
add_library(${LIBNAME} SHARED ${src} ${allheaders} ${LIBNAME}Dict.C)
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

  add_executable(${EXENAME} main.C)

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
