# Tests-related CMake configuration

# cache configuration
option(OGDF_SEPARATE_TESTS "Whether to build separate test executables (used for continuous integration)" OFF)

function(make_test_target TARGET)
  target_include_directories(${TARGET} BEFORE PUBLIC test/include)
  make_user_target(${TARGET})
endfunction()

file(GLOB_RECURSE TEST_SOURCES test/src/*.cpp)
group_files(TEST_SOURCES "test")
if(OGDF_SEPARATE_TESTS)
  add_custom_target(tests)

  # omit compiling main.cpp for every separate test
  add_library(bandit-runner STATIC EXCLUDE_FROM_ALL "test/src/main.cpp")
  make_test_target(bandit-runner)

  # compile each test
  SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/test/bin")
  foreach(SOURCE_FILE ${TEST_SOURCES})
    get_filename_component(TARGET ${SOURCE_FILE} NAME_WE)
    if(NOT ${TARGET} STREQUAL "main")
      add_executable(test-${TARGET} EXCLUDE_FROM_ALL ${SOURCE_FILE})
      add_dependencies(tests test-${TARGET})
      target_link_libraries(test-${TARGET} bandit-runner)
      make_test_target(test-${TARGET})
    endif()
  endforeach()
else()
  add_executable(tests EXCLUDE_FROM_ALL ${TEST_SOURCES})
  make_test_target(tests)
endif()
