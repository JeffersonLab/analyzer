# Fetch EVIO archive (currently from our GitHub mirror) and prepare for building the C-library

message(STATUS "Will build local copy of EVIO")
set(EVIO_VERSION 5.2)
set(repo evio)
set(release v${EVIO_VERSION})
set(tarfile ${release}.tar.gz)
set(EVIO_HASH 41cf675cc38cf783831cb5368ce12700)

set(EVIO_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/evio/src)
set(EVIO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/evio/build)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${EVIO_SOURCE_DIR} ${EVIO_BINARY_DIR})
set(EVIO_TARFILE ${EVIO_SOURCE_DIR}/${tarfile})
file(DOWNLOAD https://github.com/JeffersonLab/${repo}/archive/${tarfile}
  ${EVIO_TARFILE}
  EXPECTED_HASH MD5=${EVIO_HASH}
  STATUS _status
  )
list(GET _status 0 _errval)
if(_errval)
  list(GET _status 1 _errmsg)
  message(FATAL_ERROR "Failed to download EVIO archive: ${_errmsg}")
else()
  message(STATUS "Successfully downloaded EVIO archive version ${EVIO_VERSION}")
endif()
unset(_status)
unset(_errval)

# This command is system dependent. It requires GNU tar or macOS BSD tar in PATH.
# cmake -E tar does not support --strip-components and extracting a filename glob.
find_program(TARPROG NAMES tar gtar DOC "Unix tar archive utility")
if(NOT TARPROG)
  message(FATAL_ERROR "Need tar to unpack EVIO archive")
endif()
execute_process(COMMAND ${TARPROG} --version OUTPUT_VARIABLE _tarout)
STRING(FIND "${_tarout}" "GNU" _havegnu)
if(_havegnu GREATER -1)
  # GNU tar: some versions want an explicit --wildcards flag
  set(TAR_FLAGS "--wildcards")
endif()
execute_process(COMMAND ${TARPROG} -x --strip-components=3 -f ${EVIO_TARFILE} ${TAR_FLAGS} "*/libsrc"
  WORKING_DIRECTORY ${EVIO_SOURCE_DIR})
# Patch for issues with official EVIO release
if(INT64_IS_LONG)
  find_program(PATCHPROG NAMES patch DOC "Unix patch utility")
  if(NOT PATCHPROG)
    message(FATAL_ERROR "Need patch to build EVIO")
  endif()
  # This is somehow backwards. The code should /not/ need patching for the
  # standard case int64_t = long int. But it assumes int64_t = long long int.
  execute_process(COMMAND ${PATCHPROG} --silent --strip 3 --input
    ${CMAKE_CURRENT_SOURCE_DIR}/evio/0001-revert-printf-long-long-format.patch
    WORKING_DIRECTORY ${EVIO_SOURCE_DIR})
endif()
# Configure custom CMake files for building the EVIO C-library only
configure_file(evio/CMakeLists.txt.in ${EVIO_SOURCE_DIR}/CMakeLists.txt @ONLY)
file(COPY evio/EVIOConfig.cmake.in DESTINATION ${EVIO_SOURCE_DIR})

add_subdirectory(${EVIO_SOURCE_DIR} ${EVIO_BINARY_DIR})
