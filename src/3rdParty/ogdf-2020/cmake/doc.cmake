# Documentation-related CMake configuration

set(doc_dir "${PROJECT_SOURCE_DIR}/doc")
set(DOC_INSTALL_DIR "share/doc/libogdf" CACHE PATH "Installation path of OGDF documentation and examples")
mark_as_advanced(DOC_INSTALL_DIR)

# installation of examples as documentation
file(GLOB_RECURSE example_files "${doc_dir}/*.txt" "${doc_dir}/*.bench" "${doc_dir}/*.gml")
install(DIRECTORY doc/examples
  DESTINATION ${DOC_INSTALL_DIR}
  FILES_MATCHING
    PATTERN "*.cpp"
    PATTERN "*.txt"
    PATTERN "*.gml"
    PATTERN "*.bench"
    PATTERN "output-*" EXCLUDE)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  if(DOXYGEN_VERSION VERSION_LESS 1.8.6)
    message(WARNING "Doxygen version >= 1.8.6 is necessary to build the documentation correctly.")
  endif()

  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${doc_dir}/ogdf-doxygen.cfg WORKING_DIRECTORY ${doc_dir})
  option(DOC_INSTALL "Whether we want to install documentation. Makes doc part of the default target." OFF)

  if(DOC_INSTALL)
    set(doxystamp "${PROJECT_BINARY_DIR}/doxygen.stamp")
    file(GLOB_RECURSE doc_files ${doc_dir}/*.md ${doc_dir}/*.dox)
    add_custom_command(OUTPUT "${doxystamp}"
      DEPENDS ${doc_files} ${ogdf_headers}
              ${doc_dir}/ogdf-doxygen.cfg ${doc_dir}/ogdf-footer.html ${doc_dir}/ogdf-header.html ${doc_dir}/ogdf-layout.xml
      COMMAND cmake --build . --target doc
      COMMAND cmake -E touch "${doxystamp}"
      COMMENT "Generating documentation if necessary"
      VERBATIM)
    add_custom_target(doc-check ALL DEPENDS "${doxystamp}")

    install(DIRECTORY doc/html DESTINATION "${DOC_INSTALL_DIR}")
    install(FILES LICENSE.txt LICENSE_GPL_v2.txt LICENSE_GPL_v3.txt DESTINATION "${DOC_INSTALL_DIR}")
  endif()
else()
  message(WARNING "Doxygen not found. Documentation cannot be built.")
endif()
