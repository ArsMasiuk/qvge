# Documentation-related CMake configuration

find_package(Doxygen)

if(DOXYGEN_FOUND)
  if(DOXYGEN_VERSION VERSION_LESS 1.8.6)
    message(WARNING "Doxygen version >= 1.8.6 is necessary to build the documentation correctly.")
  endif()

  set(DOC_DIR "${PROJECT_SOURCE_DIR}/doc")
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${DOC_DIR}/ogdf-doxygen.cfg WORKING_DIRECTORY ${DOC_DIR})
else()
  message(WARNING "Doxygen not found. Documentation cannot be built.")
endif()
