option(ALTEL_BUILD_EUDAQ_JADE "build old jade raw converter?" ON)
if(NOT ALTEL_BUILD_EUDAQ_JADE)
  return()
endif()

aux_source_directory(src MODULE_SRC_LEGACY)

find_package(eudaq CONFIG
  COMPONENTS eudaq::core eudaq::lcio
  REQUIRED NO_DEFAULT_PATH)


# include_directories(${EUDAQ_INCLUDE_DIRS})

list(FIND EUDAQ_LIBRARIES "eudaq_lcio" LCIO_FIND_POS)
if(${LCIO_FIND_POS} EQUAL -1)
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")
endif()

add_library(eudaq_module_altel_legacy SHARED ${MODULE_SRC_LEGACY})
set_target_properties(eudaq_module_altel_legacy PROPERTIES CXX_STANDARD 14)
target_include_directories(eudaq_module_altel_legacy
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

target_link_libraries(eudaq_module_altel_legacy eudaq::core eudaq::lcio mycommon)

install(TARGETS
  eudaq_module_altel_legacy
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
