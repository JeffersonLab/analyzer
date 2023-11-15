# Create the project shell environment setup script

if(APPLE)
  set(DY "DY")
endif()

foreach(scrpt setup.sh setup.csh setup_inbuild.sh setup_inbuild.csh)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/../templates/${scrpt}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${scrpt}
    @ONLY
  )
endforeach()

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.sh ${CMAKE_CURRENT_BINARY_DIR}/setup.csh
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
