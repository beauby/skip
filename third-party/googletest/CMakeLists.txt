if(NOT DEFINED GTEST_FOUND)
  find_path(GTEST_INCLUDE NAMES gtest/gtest.h HINTS ${GTEST_HINT}/include)
  find_path(GTEST_SRC NAMES src/gtest-all.cc HINTS ${GTEST_HINT} PATH_SUFFIXES src/googletest)
  find_library(GTEST_LIBRARY NAMES libgtest.dylib libgtest.so)
  if(GTEST_INCLUDE AND GTEST_LIBRARY AND GTEST_SRC)
    set(GTEST_FOUND YES CACHE BOOL "" FORCE)
  else()
    set(GTEST_FOUND NO CACHE BOOL "" FORCE)
  endif()
endif()

if(GTEST_FOUND)
  message("Using built-in gtest: ${GTEST_INCLUDE} ${GTEST_LIBRARY} ${GTEST_SRC}")

  add_library(gtest_main STATIC
    "${GTEST_SRC}/src/gtest-all.cc"
    "${GTEST_SRC}/src/gtest_main.cc"
    )
  target_include_directories(gtest_main PRIVATE ${GTEST_SRC})
  target_include_directories(gtest_main PUBLIC ${GTEST_INCLUDE})
else()
  message(FATAL_ERROR "Dependency gtest not found.")
endif()
