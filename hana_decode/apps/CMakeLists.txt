cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Decoder example/test executables
add_executable(epicsd epics_main.cxx)
add_executable(prfact prfact_main.cxx)
add_executable(tdecex tdecex_main.cxx THaGenDetTest.cxx)
add_executable(tdecpr tdecpr_main.cxx)
add_executable(tst1190 tst1190_main.cxx)
add_executable(tstf1tdc tstf1tdc_main.cxx)
add_executable(tstfadc tstfadc_main.cxx)
add_executable(tstfadcblk tstfadcblk_main.cxx)
add_executable(tstio tstio_main.cxx)
add_executable(tstoo tstoo_main.cxx)

set(allexe epicsd prfact tdecex tdecpr tst1190 tstf1tdc
  tstfadc tstfadcblk tstio tstoo
  )

if(ONLINE_ET)
  add_executable(etclient etclient_main.cxx)
  add_executable(tstcoda tstcoda_main.cxx)
  list(APPEND allexe etclient tstcoda)
endif()


foreach(exe IN LISTS allexe)
  target_link_libraries(${exe}
    PRIVATE
      Podd::Decode
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
