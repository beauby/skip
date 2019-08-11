if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

option(USE_JEMALLOC "Use jemalloc" ON)

# Quiet make output
set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

option(INCLUDE_JS_SELF "Include JS self hosting tests" OFF)
option(INCLUDE_FAILING "Include failing tests" OFF)

option(EXTERN_LKG "Location of an external LKG. If set should be the path to another skip repo's build tree to use as the lkg." OFF)

option(CONFIGURE_TESTS "Configure including tests" ON)
