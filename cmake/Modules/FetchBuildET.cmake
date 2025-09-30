# Fetch ET archive from GitHub and prepare for building the C-library

message(STATUS "Will build local copy of ET")
set(ET_VERSION 16.5.0)
set(org JeffersonLab)
set(repo et)
set(release v${ET_VERSION})
set(tarfile ${release}.tar.gz)
set(ET_HASH 399629afd753ee05d3419408935e0baa)

set(ET_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/et/src)
set(ET_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/et/build)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ET_SOURCE_DIR} ${ET_BINARY_DIR})
set(ET_TARFILE ${ET_SOURCE_DIR}/${tarfile})
file(DOWNLOAD https://github.com/${org}/${repo}/archive/${tarfile}
  ${ET_TARFILE}
  EXPECTED_HASH MD5=${ET_HASH}
  STATUS _status
  )
list(GET _status 0 _errval)
if(_errval)
  list(GET _status 1 _errmsg)
  message(FATAL_ERROR "Failed to download ET archive: ${_errmsg}")
else()
  message(STATUS "Successfully downloaded ET archive version ${ET_VERSION}")
endif()
unset(_status)
unset(_errval)

# This command is system dependent. It requires GNU tar or macOS BSD tar in PATH.
# cmake -E tar does not support --strip-components and extracting a filename glob.
find_program(TARPROG NAMES tar gtar DOC "Unix tar archive utility")
if(NOT TARPROG)
  message(FATAL_ERROR "Need tar to unpack ET archive")
endif()
execute_process(COMMAND ${TARPROG} --version OUTPUT_VARIABLE _tarout)
STRING(FIND "${_tarout}" "GNU" _havegnu)
if(_havegnu GREATER -1)
  # GNU tar: some versions want an explicit --wildcards flag
  set(TAR_FLAGS "--wildcards")
endif()
execute_process(COMMAND ${TARPROG} -x --strip-components=3 -f ${ET_TARFILE} ${TAR_FLAGS} "*/libsrc"
  WORKING_DIRECTORY ${ET_SOURCE_DIR})
# Configure custom CMake files for building the ET C-library only
configure_file(et/CMakeLists.txt.in ${ET_SOURCE_DIR}/CMakeLists.txt @ONLY)
file(COPY et/ETConfig.cmake.in DESTINATION ${ET_SOURCE_DIR})

add_subdirectory(${ET_SOURCE_DIR} ${ET_BINARY_DIR})
