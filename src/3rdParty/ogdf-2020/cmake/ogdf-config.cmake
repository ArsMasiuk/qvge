# This file is to be included by user code using find_package()

include("${CMAKE_CURRENT_LIST_DIR}/CoinTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/OgdfTargets.cmake")

set(OGDF_INCLUDE_DIRS $<TARGET_PROPERTY:OGDF,INTERFACE_INCLUDE_DIRECTORIES>)
