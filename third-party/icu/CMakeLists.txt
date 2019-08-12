find_library(LIBICU_UC NAMES libicuuc.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_I18N NAMES libicui18n.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_UC NAMES libicuuc.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_I18N NAMES libicui18n.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_IO NAMES libicuio.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_TU NAMES libicutu.a HINTS ${ICU_HINT}/lib)
find_library(LIBICU_DATA NAMES libicudata.a HINTS ${ICU_HINT}/lib)
find_path(LIBICU_INCLUDE unicode/utf8.h HINTS ${ICU_HINT}/include)
if(LIBICU_UC AND LIBICU_I18N AND LIBICU_UC AND LIBICU_I18N AND LIBICU_IO AND LIBICU_TU AND LIBICU_DATA AND LIBICU_INCLUDE)
  set(ICU4C_FOUND YES CACHE BOOL "" FORCE)
else()
  set(ICU4C_FOUND NO CACHE BOOL "" FORCE)
endif()

if(ICU4C_FOUND)
  message("Using built-in icu4c: ${LIBICU_INCLUDE} (${LIBICU_I18N})")

  add_custom_target(icu4c)

  add_library(icuuc INTERFACE)
  target_include_directories(icuuc INTERFACE ${LIBICU_INCLUDE})
  target_link_libraries(icuuc INTERFACE ${LIBICU_UC})

  add_library(icui18n INTERFACE)
  target_include_directories(icui18n INTERFACE ${LIBICU_INCLUDE})
  target_link_libraries(icui18n INTERFACE ${LIBICU_I18N})

  add_library(icuio INTERFACE)
  target_include_directories(icuio INTERFACE ${LIBICU_INCLUDE})
  target_link_libraries(icuio INTERFACE ${LIBICU_IO})

  add_library(icutu INTERFACE)
  target_include_directories(icutu INTERFACE ${LIBICU_INCLUDE})
  target_link_libraries(icutu INTERFACE ${LIBICU_TU})

  add_library(icudata INTERFACE)
  target_include_directories(icudata INTERFACE ${LIBICU_INCLUDE})
  target_link_libraries(icudata INTERFACE ${LIBICU_DATA})

  set(ICU4C_LIBRARIES
    ${LIBICU_UC}
    ${LIBICU_I18N}
    ${LIBICU_IO}
    ${LIBICU_TU}
    ${LIBICU_DATA}
    CACHE STRING "")
else()
  message(FATAL_ERROR "Dependency icu not found.")
endif()
