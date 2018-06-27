cmake_minimum_required(VERSION 3.5)

set(podd_src 
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

# This would be alot easier if there was an "include" directory
set(headers
BankData.h BdataLoc.h FileInclude.h FixedArrayVar.h Helper.h MethodVar.h
SeqCollectionMethodVar.h SeqCollectionVar.h THaADCHelicity.h THaAnalysisObject.h THaAnalyzer.h
THaApparatus.h THaArrayString.h THaAvgVertex.h THaBeamDet.h THaBeamEloss.h THaBeam.h THaBeamInfo.h THaBeamModule.h
THaBPM.h THaCherenkov.h THaCluster.h THaCodaRun.h THaCoincTime.h THaCut.h THaCutList.h THaDebugModule.h THaDecData.h
THaDetectorBase.h THaDetector.h THaDetMap.h THaElectronKine.h THaElossCorrection.h THaEpicsEbeam.h THaEpicsEvtHandler.h
THaEvent.h THaEvt125Handler.h THaEvtTypeHandler.h THaExtTarCor.h THaFilter.h THaFormula.h THaG0Helicity.h
THaG0HelicityReader.h THaGlobals.h THaGoldenTrack.h THaHelicityDet.h THaHelicity.h THaHRS.h THaIdealBeam.h
THaInterface.h THaNamedList.h THaNonTrackingDetector.h THaOnlRun.h THaOutput.h THaParticleInfo.h
THaPhotoReaction.h THaPhysicsModule.h THaPidDetector.h THaPIDinfo.h THaPostProcess.h THaPrimaryKine.h THaPrintOption.h
THaQWEAKHelicity.h THaQWEAKHelicityReader.h THaRasteredBeam.h THaRaster.h THaReacPointFoil.h THaReactionPoint.h
THaRTTI.h THaRunBase.h THaRun.h THaRunParameters.h THaS2CoincTime.h THaSAProtonEP.h
THaScalerEvtHandler.h THaScintillator.h THaSecondaryKine.h THaShower.h THaSpectrometerDetector.h THaSpectrometer.h THaString.h THaSubDetector.h
THaTextvars.h THaTotalShower.h THaTrackEloss.h THaTrack.h THaTrackID.h THaTrackInfo.h THaTrackingDetector.h THaTrackingModule.h THaTrackOut.h
THaTrackProj.h THaTriggerTime.h THaTwoarmVertex.h THaUnRasteredBeam.h THaVar.h THaVarList.h THaVDCAnalyticTTDConv.h THaVDCChamber.h
THaVDCCluster.h THaVDC.h THaVDCHit.h THaVDCPlane.h THaVDCPoint.h THaVDCPointPair.h THaVDCTimeToDistConv.h THaVDCTrackID.h
THaVDCWire.h THaVertexModule.h THaVform.h THaVhist.h VarDef.h VariableArrayVar.h Variable.h
VarType.h VDCeff.h VectorObjMethodVar.h VectorObjVar.h VectorVar.h
)


ROOT_GENERATE_DICTIONARY(HallADict ${headers} 
  INCLUDEDIRS include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../hana_decode 
  LINKDEF HallA_LinkDef.h  
  OPTIONS -p)
set(PCM_FILE  ${CMAKE_CURRENT_BINARY_DIR}/HallADict_rdict.pcm)

#SET(lib_HEADERS ${lib_HEADERS})
#SET(lib_SRCS ${lib_Fortran_SRCs} ${lib_SRCS} ${lib_DICTIONARY_SRC})
#SET(lib_VERSION "${${PROJECT_NAME}_VERSION}")
#SET(lib_MAJOR_VERSION "${${PROJECT_NAME}_MAJOR_VERSION}")
#SET(lib_LIBRARY_PROPERTIES 
#    VERSION   "${lib_VERSION}"
#    SOVERSION "${lib_MAJOR_VERSION}"
#    SUFFIX ".so")

include_directories(${PROJECT_SOURCE_DIR}/hana_decode)

ADD_CUSTOM_TARGET(HallA_ROOTDICTS DEPENDS ${podd_src} ${headers} HallA_LinkDef.h HallADict.cxx)

add_library(HallA STATIC
  ${podd_src} HallADict.cxx
  )

add_library(PODD::HallA ALIAS HallA)

#Set target properties
target_include_directories(HallA
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

#target_compile_features(HallA PRIVATE cxx_auto_type)

target_link_libraries(HallA
  #PUBLIC ${EXPAT_LIBRARIES}
  #PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC ${ROOT_LIBRARIES} 
  PRIVATE PODD::Decode 
)
#This is required so that the exported target has the name JSONUtils and not jsonutils
set_target_properties(HallA PROPERTIES EXPORT_NAME HallA)

add_executable(analyzer main.cxx)
target_include_directories(analyzer
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

#target_compile_features(HallA PRIVATE cxx_auto_type)
target_compile_options(analyzer PRIVATE $<$<CXX_COMPILER_ID:GNU>:-DHAS_SSTREAM=1>)
target_link_libraries(analyzer
  PRIVATE HallA
  PRIVATE PODD::Decode
  PUBLIC ${ROOT_LIBRARIES} 
)

install(TARGETS HallA analyzer
  EXPORT podd-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )


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
