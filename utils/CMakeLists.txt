cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of the main items we build here
set(EXENAME dbconvert)

#----------------------------------------------------------------------------
# Sources and headers
set(src dbconvert.cxx)

#----------------------------------------------------------------------------
# Executables
add_executable(${EXENAME} ${src})

target_link_libraries(${EXENAME}
  PRIVATE
    Podd::HallA
  )
target_compile_options(${EXENAME}
  PRIVATE
    ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )

install(TARGETS ${EXENAME}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
