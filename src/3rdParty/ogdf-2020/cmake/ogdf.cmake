# OGDF (library only) CMake configuration

# cache configuration
option(BUILD_SHARED_LIBS "Whether to build shared libraries instead of static ones." OFF)
set(OGDF_MEMORY_MANAGER "POOL_TS" CACHE STRING "Memory manager to be used.")
set_property(CACHE OGDF_MEMORY_MANAGER PROPERTY STRINGS POOL_TS POOL_NTS MALLOC_TS)
set(OGDF_DEBUG_MODE "REGULAR" CACHE STRING "Whether to use (heavy) OGDF assertions in debug mode.")
set_property(CACHE OGDF_DEBUG_MODE PROPERTY STRINGS NONE REGULAR HEAVY)
mark_as_advanced(OGDF_DEBUG_MODE)
option(OGDF_USE_ASSERT_EXCEPTIONS "Whether to throw an exception on failed assertions." OFF)
set(OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE "OFF" CACHE
    STRING "Which library (libdw, libbdf, libunwind) to use in case a stack trace should be written \
    to a failed assertion exceptions's what(). Library must be found by CMake to be able to use it.")
if(OGDF_USE_ASSERT_EXCEPTIONS)
  set_property(CACHE OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE PROPERTY STRINGS "OFF")
else()
  unset(OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE CACHE)
endif()
option(OGDF_WARNING_ERRORS "Whether to treat compiler warnings as errors; may break compilation!" OFF)
mark_as_advanced(OGDF_WARNING_ERRORS)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND OGDF_MEMORY_MANAGER STREQUAL MALLOC_TS)
  option(OGDF_LEAK_CHECK "Whether to use the address sanitizer for the MALLOC_TS memory manager." OFF)
else()
  unset(OGDF_LEAK_CHECK CACHE)
endif()

# set debug mode
if(OGDF_DEBUG_MODE STREQUAL HEAVY)
  set(OGDF_HEAVY_DEBUG ON)
endif()
if(NOT OGDF_DEBUG_MODE STREQUAL NONE AND (MULTICONFIG_BUILD OR CMAKE_BUILD_TYPE MATCHES Debug))
  set(OGDF_DEBUG ON)
endif()

# find available packages for stack traces
if(OGDF_USE_ASSERT_EXCEPTIONS)
  find_package(Libdw)
  if(LIBDW_FOUND)
    set_property(CACHE OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE APPEND PROPERTY STRINGS "ON_LIBDW")
  endif()
  find_package(Libbfd)
  if(LIBBFD_FOUND)
    set_property(CACHE OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE APPEND PROPERTY STRINGS "ON_LIBBFD")
  endif()
  find_package(Libunwind)
  if(LIBUNWIND_FOUND)
    set_property(CACHE OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE APPEND PROPERTY STRINGS "ON_LIBUNWIND")
  endif()
endif()
set(extra_flags_desc "Extra compiler flags for compiling OGDF, tests, and examples")
set(OGDF_EXTRA_CXX_FLAGS "${available_default_warning_flags}" CACHE
    STRING "${extra_flags_desc}.")
set(OGDF_EXTRA_CXX_FLAGS_DEBUG "${available_default_warning_flags_debug}" CACHE
    STRING "${extra_flags_desc} applied only when compiling in debug mode.")
set(OGDF_EXTRA_CXX_FLAGS_RELEASE "${available_default_warning_flags_release}" CACHE
    STRING "${extra_flags_desc} applied only when not compiling in debug mode.")
mark_as_advanced(OGDF_EXTRA_CXX_FLAGS)
mark_as_advanced(OGDF_EXTRA_CXX_FLAGS_DEBUG)
mark_as_advanced(OGDF_EXTRA_CXX_FLAGS_RELEASE)

# compilation
file(GLOB_RECURSE ogdf_headers include/ogdf/*.h)
file(GLOB_RECURSE ogdf_sources src/ogdf/*.cpp)
set(ogdf_sources "${ogdf_sources};${ogdf_headers}")
add_library(OGDF "${ogdf_sources}")
target_link_libraries(OGDF PUBLIC COIN)
group_files(ogdf_sources "ogdf")
target_compile_features(OGDF PUBLIC cxx_range_for)

target_include_directories(OGDF PUBLIC # for the autogen header
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(OGDF PUBLIC # for the general include files
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
if(COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES)
  target_include_directories(OGDF SYSTEM PUBLIC ${COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES})
endif()

function (add_ogdf_extra_flags TARGET_NAME)
  set(extra_flags ${OGDF_EXTRA_CXX_FLAGS})
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(extra_flags  "${extra_flags} ${OGDF_EXTRA_CXX_FLAGS_DEBUG}")
  else()
    set(extra_flags  "${extra_flags} ${OGDF_EXTRA_CXX_FLAGS_RELEASE}")
  endif()
  if(OGDF_WARNING_ERRORS)
    set(extra_flags "${warnings_as_errors_flag} ${extra_flags}")
  endif()
  if(OGDF_LEAK_CHECK)
    set(leak_flags "-fsanitize=address -fno-omit-frame-pointer")
    set(extra_flags "${extra_flags} ${leak_flags}")
    set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS " ${leak_flags} ")
  endif()
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " ${extra_flags} ")
endfunction()

add_ogdf_extra_flags(OGDF)

# set OGDF_INSTALL for shared libraries
if(BUILD_SHARED_LIBS)
  target_compile_definitions(OGDF PRIVATE OGDF_INSTALL)
endif()

# autogen header variables for debug mode
set(SHOW_STACKTRACE 0)
if(OGDF_DEBUG AND OGDF_USE_ASSERT_EXCEPTIONS)
  include(check-pretty-function)
  if(compiler_has_pretty_function)
    set(OGDF_FUNCTION_NAME "__PRETTY_FUNCTION__")
  else() # fallback to C++11 standard
    set(OGDF_FUNCTION_NAME "__func__")
  endif()
  if(OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE MATCHES LIBDW)
    set(SHOW_STACKTRACE 1)
    set(BACKWARD_HAS_DW 1)
  elseif(OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE MATCHES LIBBFD)
    set(SHOW_STACKTRACE 1)
    set(BACKWARD_HAS_BFD 1)
  elseif(OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE MATCHES LIBUNWIND)
    set(SHOW_STACKTRACE 1)
    set(BACKWARD_HAS_UNWIND 1)
  endif()
endif()

# autogen header variables if libs are shared
if(BUILD_SHARED_LIBS)
  set(OGDF_DLL 1)
endif()

# autogen header variables for SSE3
include(check-sse3)
if(has_sse3_intrin)
  set(OGDF_SSE3_EXTENSIONS <intrin.h>)
elseif(has_sse3_pmmintrin)
  set(OGDF_SSE3_EXTENSIONS <pmmintrin.h>)
else()
  message(STATUS "SSE3 could not be activated")
endif()

# autogen header variables for Linux-specific CPU_SET, etc.
include(check-cpu-macros)
if(has_linux_cpu_macros)
  set(OGDF_HAS_LINUX_CPU_MACROS 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_link_libraries(OGDF PUBLIC pthread)
endif()

# add stack trace settings
if(BACKWARD_HAS_DW)
  target_include_directories(OGDF SYSTEM PUBLIC ${LIBDW_INCLUDE_DIR})
elseif(BACKWARD_HAS_BFD)
  target_include_directories(OGDF SYSTEM PUBLIC ${LIBBFD_INCLUDE_DIR} ${LIBDL_INCLUDE_DIR})
elseif(BACKWARD_HAS_UNWIND)
  target_include_directories(OGDF SYSTEM PUBLIC ${LIBUNWIND_INCLUDE_DIR})
endif()
if(SHOW_STACKTRACE)
  if(BACKWARD_HAS_DW)
    target_link_libraries(OGDF PUBLIC ${LIBDW_LIBRARY})
  elseif(BACKWARD_HAS_BFD)
    target_link_libraries(OGDF PUBLIC ${LIBBFD_LIBRARY} ${LIBDL_LIBRARY})
  elseif(BACKWARD_HAS_UNWIND)
    target_link_libraries(OGDF PUBLIC ${LIBUNWIND_LIBRARY})
  endif()
endif()

# installation
set(OGDF_INSTALL_LIBRARY_DIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "Installation path of OGDF library")
set(OGDF_INSTALL_INCLUDE_DIR "include" CACHE PATH "Installation path of OGDF header files (creates subdirectory)")
set(OGDF_INSTALL_CMAKE_DIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/OGDF/" CACHE PATH "Installation path of OGDF files for CMake")
mark_as_advanced(OGDF_INSTALL_LIBRARY_DIR OGDF_INSTALL_INCLUDE_DIR OGDF_INSTALL_CMAKE_DIR)
configure_file(cmake/ogdf-config.cmake "${PROJECT_BINARY_DIR}/ogdf-config.cmake" @ONLY)
install(TARGETS OGDF
  EXPORT OgdfTargets
  LIBRARY DESTINATION "${OGDF_INSTALL_LIBRARY_DIR}"
  ARCHIVE DESTINATION "${OGDF_INSTALL_LIBRARY_DIR}"
  INCLUDES DESTINATION "${COIN_INSTALL_INCLUDE_DIR}"
  PUBLIC_HEADER DESTINATION "${OGDF_INSTALL_INCLUDE_DIR}")
install(DIRECTORY "${PROJECT_BINARY_DIR}/include/ogdf" include/ogdf
  DESTINATION "${OGDF_INSTALL_INCLUDE_DIR}"
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inc")
install(EXPORT OgdfTargets DESTINATION "${OGDF_INSTALL_CMAKE_DIR}")
install(FILES "${PROJECT_BINARY_DIR}/ogdf-config.cmake" DESTINATION "${OGDF_INSTALL_CMAKE_DIR}")
export(EXPORT OgdfTargets)
