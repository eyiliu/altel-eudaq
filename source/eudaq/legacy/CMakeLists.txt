option(ALTEL_BUILD_EUDAQ_MODULE_LAGECY "build lagecy eudaq module (JadeProducer and JadeRaw Converters)" ON)
if(NOT ALTEL_BUILD_EUDAQ_MODULE_LAGECY)
  return()
endif()

message(STATUS "Module eudaq_module_altel_legacy is enabled")

aux_source_directory(src MODULE_SRC_LEGACY)

find_package(eudaq REQUIRED
  COMPONENTS eudaq::core
  OPTIONAL_COMPONENTS eudaq::lcio
  CONFIG
  PATHS ${CMAKE_INSTALL_PREFIX}
  NO_DEFAULT_PATH
  )

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel_legacy")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel_lecagy")
endif()

add_library(eudaq_module_altel_legacy SHARED ${MODULE_SRC_LEGACY})
set_target_properties(eudaq_module_altel_legacy PROPERTIES CXX_STANDARD 14)
target_include_directories(eudaq_module_altel_legacy
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

if(TARGET eudaq::lcio)
  target_link_libraries(eudaq_module_altel_legacy eudaq::core eudaq::lcio  mycommon)
else()
  target_link_libraries(eudaq_module_altel_legacy eudaq::core mycommon)
endif()

install(TARGETS
  eudaq_module_altel_legacy
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
