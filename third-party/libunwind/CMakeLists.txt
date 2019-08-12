if(APPLE)
  # libunwind doesn't work on OSX
  return()
endif()

find_path(LIBUNWIND_INCLUDE NAMES libunwind.h)
find_library(LIBUNWIND_LIB NAMES libunwind.a libunwind.dylib libunwind.so)
if(LIBUNWIND_INCLUDE AND LIBUNWIND_LIB)
  set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIB} CACHE STRING "" FORCE)
  set(LIBUNWIND_FOUND YES CACHE BOOL "" FORCE)
else()
  set(LIBUNWIND_FOUND NO CACHE BOOL "" FORCE)
endif()

add_library(libunwind INTERFACE)

if(LIBUNWIND_FOUND)
  message("Using built-in libunwind: ${LIBUNWIND_INCLUDE} ${LIBUNWIND_LIBRARIES}")
  target_include_directories(libunwind INTERFACE ${LIBUNWIND_INCLUDE})
  target_link_libraries(libunwind INTERFACE ${LIBUNWIND_LIBRARIES})
else()
  message(FATAL_ERROR "Dependency libunwind not found.")
endif()
