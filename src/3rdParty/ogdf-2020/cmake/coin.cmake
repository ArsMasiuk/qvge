# COIN-related CMake configuration

# cache
set(COIN_SOLVER "CLP" CACHE STRING "Linear program solver to be used by COIN.")
set_property(CACHE COIN_SOLVER PROPERTY STRINGS CLP CPX GRB)
if(NOT COIN_SOLVER STREQUAL CLP)
  set(COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES "" CACHE PATH "Locations of required header files for the external LP solver.")
  set(COIN_EXTERNAL_SOLVER_LIBRARIES "" CACHE FILEPATH "Libraries for the external LP solver.")
else()
  unset(COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES CACHE)
  unset(COIN_EXTERNAL_SOLVER_LIBRARIES CACHE)
endif()

# compilation
file(GLOB_RECURSE COIN_SOURCES src/coin/*.cpp)
if(NOT COIN_SOLVER STREQUAL "GRB")
  list(REMOVE_ITEM COIN_SOURCES "${PROJECT_SOURCE_DIR}/src/coin/Osi/OsiGrbSolverInterface.cpp")
endif()
if(NOT COIN_SOLVER STREQUAL "CPX")
  list(REMOVE_ITEM COIN_SOURCES "${PROJECT_SOURCE_DIR}/src/coin/Osi/OsiCpxSolverInterface.cpp")
endif()
add_library(COIN ${COIN_LIBRARY_TYPE} ${COIN_SOURCES})
group_files(COIN_SOURCES "coin")
target_include_directories(COIN SYSTEM PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/coin>
  $<INSTALL_INTERFACE:include/coin>)
target_compile_definitions(COIN PRIVATE
    -DCLP_BUILD -DCOINUTILS_BUILD -DOSI_BUILD -D__OSI_CLP__
    -DCOMPILE_IN_CG -DCOMPILE_IN_CP -DCOMPILE_IN_LP -DCOMPILE_IN_TM
    -DHAVE_CONFIG_H -D_CRT_SECURE_NO_WARNINGS)

# external LP solver
if(COIN_EXTERNAL_SOLVER_LIBRARIES)
  target_link_libraries(COIN PUBLIC ${COIN_EXTERNAL_SOLVER_LIBRARIES})
  foreach(EXT_LIB ${COIN_EXTERNAL_SOLVER_LIBRARIES})
    if(NOT EXISTS ${EXT_LIB} OR IS_DIRECTORY ${EXT_LIB})
      message(SEND_ERROR "The provided library does not exist: ${EXT_LIB}")
    endif()
  endforeach()
endif()
if(COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES)
  foreach(EXT_DIR ${COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES})
    if(NOT IS_DIRECTORY ${EXT_DIR})
      message(SEND_ERROR "The provided directory is invalid: ${EXT_DIR}")
    endif()
  endforeach()
  target_include_directories(COIN SYSTEM PUBLIC ${COIN_EXTERNAL_SOLVER_INCLUDE_DIRECTORIES})
endif()

# autogen header variables
if(COIN_SOLVER STREQUAL "CLP")
  set(COIN_SOLVER_IS_EXTERNAL 0)
else()
  set(COIN_SOLVER_IS_EXTERNAL 1)
endif()

# installation
set(COIN_INSTALL_LIBRARY_DIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "Installation path of COIN library")
set(COIN_INSTALL_INCLUDE_DIR "include" CACHE PATH "Installation path of COIN header files (creates subdirectory)")
set(COIN_INSTALL_CMAKE_DIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/OGDF/" CACHE PATH "Installation path of COIN files for CMake")
mark_as_advanced(COIN_INSTALL_LIBRARY_DIR COIN_INSTALL_INCLUDE_DIR COIN_INSTALL_CMAKE_DIR)
install(TARGETS COIN
  EXPORT CoinTargets
  LIBRARY DESTINATION "${COIN_INSTALL_LIBRARY_DIR}"
  ARCHIVE DESTINATION "${COIN_INSTALL_LIBRARY_DIR}"
  INCLUDES DESTINATION "${COIN_INSTALL_INCLUDE_DIR}"
  PUBLIC_HEADER DESTINATION "${COIN_INSTALL_INCLUDE_DIR}")
install(DIRECTORY include/coin
  DESTINATION "${COIN_INSTALL_INCLUDE_DIR}"
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")
install(EXPORT CoinTargets DESTINATION "${COIN_INSTALL_CMAKE_DIR}")
export(EXPORT CoinTargets)
