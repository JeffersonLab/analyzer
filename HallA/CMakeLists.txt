cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(LIBNAME HallA)

#----------------------------------------------------------------------------
# Sources and headers
set(src
  THaADCHelicity.cxx         THaDecData.cxx              THaG0Helicity.cxx
  THaG0HelicityReader.cxx    THaHelicity.cxx             THaHRS.cxx
  THaQWEAKHelicity.cxx       THaQWEAKHelicityReader.cxx  THaS2CoincTime.cxx
  THaVDCAnalyticTTDConv.cxx  THaVDCChamber.cxx           THaVDCCluster.cxx
  THaVDC.cxx                 THaVDCHit.cxx               THaVDCPlane.cxx
  THaVDCPoint.cxx            THaVDCPointPair.cxx         THaVDCTimeToDistConv.cxx
  THaVDCTrackID.cxx          THaVDCWire.cxx              TrigBitLoc.cxx
  VDCeff.cxx
  )

string(REPLACE .cxx .h headers "${src}")

#----------------------------------------------------------------------------
# libHallA
add_library(${LIBNAME} SHARED ${src} ${headers} ${LIBNAME}Dict.cxx)
add_library(${PROJECT_NAME}::${LIBNAME} ALIAS ${LIBNAME})

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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
    ${PROJECT_NAME}::Podd
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
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# libHallADict
build_root_dictionary(${LIBNAME} ${headers}
  TARGETS ${LIBNAME}
  LINKDEF ${LIBNAME}_LinkDef.h
  )
