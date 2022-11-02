# Get metadata about the configuration and build process
# The actual work is done by a small helper script.

# If shell scripts are not supported on some platform, one could put logic here
# to select a platform-specific script/program
set(_compileinfo_cmd get_compileinfo.sh)

# Find git to get git revision info and commit date.
# If not found, quietly continue since the code can be built without git.
find_package(Git QUIET)
if( Git_FOUND )
  include(GetGitRevision)
  get_git_info(_git_hash _git_date)
  if(_git_date)
    set(${PROJECT_NAME_UC}_SOURCE_DATETIME "${_git_date}")
  endif()
endif()

set(_get_git_description_cmd get_git_description.sh)
find_program(GITDESCRIPTION ${_get_git_description_cmd}
  HINTS ${CMAKE_CURRENT_LIST_DIR}/..
  PATH_SUFFIXES scripts
  DOC "External helper program for retrieving git description"
  )
if(NOT GITDESCRIPTION)
  message(FATAL_ERROR
    "PoddCompileInfo: Cannot find ${_get_git_description_cmd}. Check source code integrity.")
endif()
if(_git_hash)
  set(_git "${GIT_EXECUTABLE}")
else()
  set(_git "nogit")
endif()
add_custom_target(gitrev_${PROJECT_NAME}
  COMMAND "${GITDESCRIPTION}"
    "${_git}"
    "${CMAKE_CURRENT_BINARY_DIR}/git_description_${PROJECT_NAME}.h"
    "${PROJECT_NAME}_GITREV"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  BYPRODUCTS git-description_${PROJECT_NAME}.h
  VERBATIM
  )

find_program(GET_COMPILEINFO ${_compileinfo_cmd}
  HINTS ${CMAKE_CURRENT_LIST_DIR}/..
  PATH_SUFFIXES scripts
  DOC "External helper program to report current build metadata"
  )
if(NOT GET_COMPILEINFO)
  message(FATAL_ERROR
    "PoddCompileInfo: Cannot find ${_compileinfo_cmd}. Check your Podd installation.")
endif()

execute_process(COMMAND "${GET_COMPILEINFO}" "${CMAKE_CXX_COMPILER}"
  OUTPUT_VARIABLE _compileinfo_out
  ERROR_VARIABLE _compileinfo_err
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

if(_compileinfo_err)
  message(FATAL_ERROR "Error running ${_compileinfo_cmd}:
${_compileinfo_err}")
endif()

string(REGEX REPLACE "\n" ";" _compileinfo "${_compileinfo_out}")
list(LENGTH _compileinfo _compileinfo_len)
if(NOT _compileinfo_len EQUAL 7)
  message(FATAL_ERROR
    "Need exactly 7 items from ${_compileinfo_cmd}, got ${_compileinfo_len}")
endif()
if(NOT _git_date)
  list(GET _compileinfo 1 ${PROJECT_NAME_UC}_SOURCE_DATETIME)
  set(${PROJECT_NAME_UC}_GIT_REVISION "")
endif()
list(GET _compileinfo 1 ${PROJECT_NAME_UC}_BUILD_TIME)
list(GET _compileinfo 2 ${PROJECT_NAME_UC}_OSVERS)
list(GET _compileinfo 3 ${PROJECT_NAME_UC}_PLATFORM)
list(GET _compileinfo 4 ${PROJECT_NAME_UC}_NODE)
list(GET _compileinfo 5 ${PROJECT_NAME_UC}_BUILD_USER)
list(GET _compileinfo 6 ${PROJECT_NAME_UC}_CXX_VERSION)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
  set(_cxx clang)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(_cxx gcc)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Intel)
  set(_cxx icc)
else()
  set(_cxx ${CMAKE_CXX_COMPILER_ID})
endif()
set(${PROJECT_NAME_UC}_CXX_SHORT_VERSION ${_cxx}-${CMAKE_CXX_COMPILER_VERSION})

unset(_get_git_description_cmd)
unset(_git)
unset(_git_hash)
unset(_git_date)
unset(_compileinfo_cmd)
unset(_compileinfo_out)
unset(_compileinfo_err)
unset(_compileinfo_len)
unset(_compileinfo)
unset(_cxx)
