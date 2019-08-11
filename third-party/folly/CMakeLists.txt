if(NOT DEFINED FOLLY_FOUND)
  find_path(FOLLY_INCLUDE NAMES folly/folly-config.h)
  find_library(FOLLY_LIBRARY NAMES libfolly.a libfolly.dylib libfolly.so)
  if(FOLLY_INCLUDE AND FOLLY_LIBRARY)
    set(FOLLY_FOUND YES CACHE BOOL "" FORCE)
  else()
    set(FOLLY_FOUND NO CACHE BOOL "" FORCE)
  endif()
endif()

add_library(folly INTERFACE)

if(FOLLY_FOUND)
  message("Using built-in folly: ${FOLLY_INCLUDE} ${FOLLY_LIBRARY}")
  add_dependencies(folly folly_sub)
  target_include_directories(folly INTERFACE ${FOLLY_INCLUDE})
  target_link_libraries(folly INTERFACE ${FOLLY_LIBRARY} double-conversion gflags
    ${LIBGLOG_LIBRARY} jemalloc ${CMAKE_DL_LIBS} ${LIBEVENT_LIBRARY}
    boost ${LIBUNWIND_LIBRARIES} ${OPENSSL_LIBRARIES}
    ${EXTRA_INTERFACE_LIBS} pthread)
  target_compile_options(folly INTERFACE -Wno-vla)
else()
  message(FATAL_ERROR "Dependency folly not found.")
endif()
