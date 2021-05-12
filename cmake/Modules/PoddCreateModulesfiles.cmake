# Create the project environment module files

set(MODULES_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/modulefiles)

foreach(mod root analyzer)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/../templates/${mod}.in
    ${MODULES_INSTALL_DIR}/${mod}
    @ONLY
    )
endforeach()

install(
  DIRECTORY ${MODULES_INSTALL_DIR}
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}"
  )
