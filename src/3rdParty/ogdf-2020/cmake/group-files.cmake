# groups source files according to the directory structure
function(group_files SOURCES)
  foreach(SOURCE_FILE ${${SOURCES}})
    get_filename_component(GROUP "${SOURCE_FILE}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" GROUP "${GROUP}")
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    set(GROUP "${GROUP}\\")
    foreach(REPL ${ARGN})
      string(REPLACE "\\${REPL}\\" "\\" GROUP "${GROUP}")
    endforeach()

    source_group("${GROUP}" FILES "${SOURCE_FILE}")
  endforeach()
endfunction()
