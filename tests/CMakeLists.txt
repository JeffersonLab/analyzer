# Configure Podd test suite

find_package(Catch2 CONFIG REQUIRED)
# find_package always fails if a package's major versions differ, even
# though both Catch2 versions 2 and 3 can be made to work here just fine
if(Catch2_VERSION VERSION_LESS 2)
  message(FATAL_ERROR "Tests require at least Catch2 version 2")
endif()

# Sources and headers
set(SRC ArrayRTTI_t.cxx Formula_t.cxx Textvars_t.cxx TestsSetup_t.cxx
  ArrayRTTI.cxx UnitTest.cxx)
# string(REPLACE .cxx .h HDR "${SRC}")
set(HDR ArrayRTTI.h UnitTest.h)

# All tests live in one large executable for now
add_executable(Catch2Tests_t ${SRC} ${HDR} Catch2TestsDict.cxx)
target_link_libraries(Catch2Tests_t PRIVATE Podd::HallA)
target_include_directories(Catch2Tests_t PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Build/link executable as necessary for the Catch version we have
if(Catch2_VERSION VERSION_GREATER_EQUAL 3)
  target_link_libraries(Catch2Tests_t PRIVATE Catch2::Catch2WithMain)
  target_compile_definitions(Catch2Tests_t PRIVATE "HAVE_CATCH3")
else()
  target_link_libraries(Catch2Tests_t PRIVATE Catch2::Catch2)
endif()

# ROOT dictionary
build_root_dictionary(Catch2Tests FOR_EXE ${HDR}
  TARGETS Catch2Tests_t
  LINKDEF Tests_LinkDef.h
)

# Add tests to CTest
include(Catch)
catch_discover_tests(Catch2Tests_t)
