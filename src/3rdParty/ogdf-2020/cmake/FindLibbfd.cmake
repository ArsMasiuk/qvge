find_path(LIBBFD_INCLUDE_DIR bfd.h)
find_path(LIBDL_INCLUDE_DIR dlfcn.h)
find_library(LIBBFD_LIBRARY bfd)
find_library(LIBDL_LIBRARY dl)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBBFD DEFAULT_MSG
  LIBBFD_LIBRARY LIBBFD_INCLUDE_DIR
  LIBDL_LIBRARY LIBDL_INCLUDE_DIR)
mark_as_advanced(LIBBFD_INCLUDE_DIR LIBDL_INCLUDE_DIR LIBBFD_LIBRARY LIBDL_LIBRARY)
