cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Build utility programs by default
option(${PROJECT_NAME_UC}_BUILD_UTILS "Build utility programs" ON)

#----------------------------------------------------------------------------
# analyzer executable

set(ANALYZER analyzer)
add_executable(${ANALYZER} analyzer.cxx)

target_link_libraries(${ANALYZER}
  PRIVATE
    Podd::HallA
  )
target_compile_options(${ANALYZER}
  PUBLIC
  ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
  ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
if(CMAKE_SYSTEM_NAME MATCHES Linux)
  # Linux (at least with g++) requires -fPIC even for the main program,
  # as we found out the hard way (see commit 20cf746)
  target_compile_options(${ANALYZER} PUBLIC -fPIC)
endif()

install(TARGETS ${ANALYZER}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

#----------------------------------------------------------------------------
# dbconvert database conversion utility

if(${PROJECT_NAME_UC}_BUILD_UTILS)
  set(DBCONVERT dbconvert)
  add_executable(${DBCONVERT} dbconvert.cxx)

  target_link_libraries(${DBCONVERT}
    PRIVATE
      Podd::HallA
    )
  target_compile_options(${DBCONVERT}
    PRIVATE
      ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
    )

  install(TARGETS ${DBCONVERT}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
