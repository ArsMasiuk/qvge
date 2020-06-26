# Compilation of examples

file(GLOB_RECURSE example_sources doc/examples/*.cpp)
group_files(example_sources "examples")
add_custom_target(examples)
foreach(source ${example_sources})
  get_filename_component(target ${source} NAME_WE)
  get_filename_component(targetdir ${source} DIRECTORY)
  add_executable(ex-${target} EXCLUDE_FROM_ALL ${source})
  add_dependencies(examples ex-${target})
  set_property(TARGET ex-${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${targetdir}")
  make_user_target(ex-${target})
endforeach()
