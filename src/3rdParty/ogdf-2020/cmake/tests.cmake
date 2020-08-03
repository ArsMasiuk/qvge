# Tests-related CMake configuration

# cache configuration
option(OGDF_SEPARATE_TESTS "Whether to build separate test executables (used for continuous integration)" OFF)

function(make_test_target TARGET)
  target_include_directories(${TARGET} BEFORE PUBLIC test/include)
  make_user_target(${TARGET})
endfunction()

file(GLOB_RECURSE all_resources test/resources/*)
set(packres_sources "${PROJECT_SOURCE_DIR}/test/src/pack-resources.cpp")
set_source_files_properties("${packres_sources}" PROPERTIES OBJECT_DEPENDS "${all_resources}")
add_executable(pack-resources EXCLUDE_FROM_ALL "${packres_sources}")
set_property(TARGET pack-resources PROPERTY RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test")
make_some_target(pack-resources test/include)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/test/src")
set(generated_path "${PROJECT_BINARY_DIR}/test/src/generated.cpp")
add_custom_command(
  OUTPUT ${generated_path}
  COMMAND pack-resources "${generated_path}"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "Packing resources to compile them into the test binary"
  DEPENDS pack-resources)

file(GLOB_RECURSE TEST_SOURCES test/src/*.cpp)
list(REMOVE_ITEM TEST_SOURCES "${packres_sources}")
group_files(TEST_SOURCES "test")
if(OGDF_SEPARATE_TESTS)
  add_custom_target(tests)

  # omit compiling main.cpp for every separate test
  add_library(bandit-runner STATIC EXCLUDE_FROM_ALL "test/src/main.cpp" "test/src/resources.cpp" "${generated_path}")
  make_test_target(bandit-runner)

  # compile each test
  foreach(SOURCE_FILE ${TEST_SOURCES})
    get_filename_component(TARGET ${SOURCE_FILE} NAME_WE)
    if(NOT ${TARGET} STREQUAL "main" AND
       NOT ${TARGET} STREQUAL "resources")
      add_executable(test-${TARGET} EXCLUDE_FROM_ALL ${SOURCE_FILE})
      set_property(TARGET test-${TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test/bin")
      add_dependencies(tests test-${TARGET})
      target_link_libraries(test-${TARGET} bandit-runner)
      make_test_target(test-${TARGET})
    endif()
  endforeach()
else()
  add_executable(tests EXCLUDE_FROM_ALL ${TEST_SOURCES} "${generated_path}")
  make_test_target(tests)
endif()
