# Create <Project>Config.cmake and <Project>ConfigVersion.cmake

include(CMakePackageConfigHelpers)

#----------------------------------------------------------------------------
# Generate project configuration and targets files. Do this only for the
# main project, though, not for submodules since their targets are included
# in the main configuration file
#
#----------------------------------------------------------------------------
# Convert the ;-list of find_dependency() commands to a sequence of lines
#
get_property(commands GLOBAL PROPERTY ${PROJECT_NAME_UC}_FIND_DEPENDENCY_COMMANDS)
set(FIND_DEPENDENCY_COMMANDS)
foreach(cmd IN LISTS commands)
  if(NOT FIND_DEPENDENCY_COMMANDS)
    set( FIND_DEPENDENCY_COMMANDS "${cmd}")
  else()
    set( FIND_DEPENDENCY_COMMANDS
      "${FIND_DEPENDENCY_COMMANDS}\n${cmd}")
  endif()
endforeach()

#----------------------------------------------------------------------------
# Configure and install the project configuration and version files
#
set(TARGETS_FILE ${INSTALL_CONFIGDIR}/${PROJECT_NAME}Targets.cmake)
find_file(${PROJECT_NAME_UC}_CONFIG_IN ${PROJECT_NAME}Config.cmake.in
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/templates ${CMAKE_CURRENT_SOURCE_DIR}
  NO_DEFAULT_PATH
  )
configure_package_config_file(
  ${${PROJECT_NAME_UC}_CONFIG_IN}
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
# Install and export targets defined by this project
#
install(EXPORT ${PROJECT_NAME_LC}-exports
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${INSTALL_CONFIGDIR}
  )
