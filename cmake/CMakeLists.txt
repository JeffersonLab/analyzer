# Create <Project>Config.cmake and <Project>ConfigVersion.cmake

include(CMakePackageConfigHelpers)

#----------------------------------------------------------------------------
# Convert the ;-list of find_dependency() commands to a sequence of lines
#
get_property(commands GLOBAL PROPERTY PODD_FIND_DEPENDENCY_COMMANDS)
set(FIND_DEPENDENCY_COMMANDS)
foreach(cmd IN LISTS commands)
  if(NOT FIND_DEPENDENCY_COMMANDS)
    set( FIND_DEPENDENCY_COMMANDS "${cmd}")
  else()
    set( FIND_DEPENDENCY_COMMANDS
      "${FIND_DEPENDENCY_COMMANDS}
${cmd}")
  endif()
endforeach()

#----------------------------------------------------------------------------
# Configure and install the project configuration and version files
#
set(TARGETS_FILE ${INSTALL_CONFIGDIR}/${PROJECT_NAME}Targets.cmake)
configure_package_config_file(
  ${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
  PATH_VARS
    INSTALL_CONFIGDIR
    CMAKE_INSTALL_PREFIX
    CMAKE_INSTALL_INCLUDEDIR
    TARGETS_FILE
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${INSTALL_CONFIGDIR}
  )

#----------------------------------------------------------------------------
# Install project-specific modules. The module directory will be appended to
# CMAKE_MODULE_PATH when loading this project's configuration file
#
install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/Modules
  DESTINATION ${INSTALL_CONFIGDIR}
  FILES_MATCHING PATTERN "*.cmake"
  )
