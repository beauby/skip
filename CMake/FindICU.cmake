find_library(ICU_UC NAMES libicuuc.a HINTS ${ICU_HINT}/lib)
find_library(ICU_I18N NAMES libicui18n.a HINTS ${ICU_HINT}/lib)
find_library(ICU_UC NAMES libicuuc.a HINTS ${ICU_HINT}/lib)
find_library(ICU_I18N NAMES libicui18n.a HINTS ${ICU_HINT}/lib)
find_library(ICU_IO NAMES libicuio.a HINTS ${ICU_HINT}/lib)
find_library(ICU_TU NAMES libicutu.a HINTS ${ICU_HINT}/lib)
find_library(ICU_DATA NAMES libicudata.a HINTS ${ICU_HINT}/lib)
find_path(ICU_INCLUDE_DIR unicode/utf8.h HINTS ${ICU_HINT}/include)

if(ICU_UC AND ICU_I18N AND ICU_UC AND ICU_I18N AND ICU_IO AND ICU_TU
    AND ICU_DATA AND ICU_INCLUDE_DIR)
  set(ICU_FOUND YES CACHE BOOL "" FORCE)

  add_custom_target(icu)

  add_library(icuuc INTERFACE)
  target_include_directories(icuuc INTERFACE ${ICU_INCLUDE_DIR})
  target_link_libraries(icuuc INTERFACE ${ICU_UC})

  add_library(icui18n INTERFACE)
  target_include_directories(icui18n INTERFACE ${ICU_INCLUDE_DIR})
  target_link_libraries(icui18n INTERFACE ${ICU_I18N})

  add_library(icuio INTERFACE)
  target_include_directories(icuio INTERFACE ${ICU_INCLUDE_DIR})
  target_link_libraries(icuio INTERFACE ${ICU_IO})

  add_library(icutu INTERFACE)
  target_include_directories(icutu INTERFACE ${ICU_INCLUDE_DIR})
  target_link_libraries(icutu INTERFACE ${ICU_TU})

  add_library(icudata INTERFACE)
  target_include_directories(icudata INTERFACE ${ICU_INCLUDE_DIR})
  target_link_libraries(icudata INTERFACE ${ICU_DATA})

  set(ICU_LIBRARIES
    ${ICU_UC}
    ${ICU_I18N}
    ${ICU_IO}
    ${ICU_TU}
    ${ICU_DATA})

  message(STATUS "Found icu: ${ICU_INCLUDE_DIR}")
else()
  if(ICU_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find icu")
  else()
    message(STATUS "Could not find icu")
  endif()
endif()
