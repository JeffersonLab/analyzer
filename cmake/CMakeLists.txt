#----------------------------------------------------------------------------
# Install project-specific modules and scripts. The module directory will be
# appended to CMAKE_MODULE_PATH when loading this project's configuration file
#
install(DIRECTORY
  ${CMAKE_CURRENT_LIST_DIR}/Modules
  DESTINATION ${INSTALL_CONFIGDIR}
  PATTERN "PoddCreate*.cmake" EXCLUDE
  )

install(DIRECTORY
  ${CMAKE_CURRENT_LIST_DIR}/scripts
  DESTINATION ${INSTALL_CONFIGDIR}
  USE_SOURCE_PERMISSIONS
  )

install(DIRECTORY
  ${CMAKE_CURRENT_LIST_DIR}/templates
  DESTINATION ${INSTALL_CONFIGDIR}
  USE_SOURCE_PERMISSIONS
  FILES_MATCHING PATTERN "GrabGitRef.cmake.in"
  )

#----------------------------------------------------------------------------
# Create <Project>Config.cmake and <Project>ConfigVersion.cmake
# as well as setup scripts and modulefiles
#
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(WriteProjectConfig)
  include(PoddCreateSetup)
  include(PoddCreateModulesfiles)
endif()
