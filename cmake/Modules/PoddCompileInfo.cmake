# Get metadata about the configuration and build process
# The actual work is done by a small helper script.

# If shell scripts are not supported on some platform, one could put logic here
# to select a platform-specfic script/program
set(_compileinfo_cmd get_compileinfo.sh)

find_program(GET_COMPILEINFO ${_compileinfo_cmd}
  HINTS
    ${CMAKE_CURRENT_LIST_DIR}/../..
    ${CMAKE_CURRENT_LIST_DIR}/../../..
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
_compileinfo_err")
endif()

string(REGEX REPLACE "\n" ";" _compileinfo "${_compileinfo_out}")
list(LENGTH _compileinfo _compileinfo_len)
if(NOT _compileinfo_len EQUAL 7)
  message(FATAL_ERROR
    "Need exactly 7 items from ${_compileinfo_cmd}, got ${_compileinfo_len}")
endif()
list(GET _compileinfo 0 ${PROJECT_NAME_UC}_BUILD_DATE)
list(GET _compileinfo 1 ${PROJECT_NAME_UC}_BUILD_DATETIME)
list(GET _compileinfo 2 ${PROJECT_NAME_UC}_PLATFORM)
list(GET _compileinfo 3 ${PROJECT_NAME_UC}_NODE)
list(GET _compileinfo 4 ${PROJECT_NAME_UC}_BUILD_USER)
list(GET _compileinfo 5 ${PROJECT_NAME_UC}_GIT_REVISION)
list(GET _compileinfo 6 ${PROJECT_NAME_UC}_CXX_VERSION)

unset(_compileinfo_cmd)
unset(_compileinfo_out)
unset(_compileinfo_err)
unset(_compileinfo_len)
unset(_compileinfo)
