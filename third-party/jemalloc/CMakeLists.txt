find_path(JEMALLOC_INCLUDE jemalloc/jemalloc.h)
find_library(JEMALLOC_LIBRARY jemalloc)

if (JEMALLOC_INCLUDE AND JEMALLOC_LIBRARY)
  set(JEMALLOC_FOUND YES CACHE BOOL "" FORCE)
else()
  set(JEMALLOC_FOUND NO CACHE BOOL "" FORCE)
endif()

if(JEMALLOC_FOUND)
  message("Using built-in jemalloc: ${JEMALLOC_INCLUDE} ${JEMALLOC_LIBRARY}")

  # We have a locally installed one
  add_library(jemalloc INTERFACE)
  target_include_directories(jemalloc INTERFACE ${JEMALLOC_INCLUDE})
  target_link_libraries(jemalloc INTERFACE ${JEMALLOC_LIBRARY})
else()
  message(FATAL_ERROR "Dependency jemalloc not found.")
endif()
