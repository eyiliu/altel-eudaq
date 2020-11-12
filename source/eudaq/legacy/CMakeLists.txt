option(ALTEL_BUILD_EUDAQ_MODULE_LAGECY "build lagecy eudaq module (JadeProducer and JadeRaw Converters)" ON)
if(NOT ALTEL_BUILD_EUDAQ_MODULE_LAGECY)
  return()
endif()

message(STATUS "Module eudaq_module_altel_legacy is enabled")

find_package(eudaq REQUIRED
  COMPONENTS eudaq::core
  OPTIONAL_COMPONENTS eudaq::lcio
  CONFIG
  PATHS ${CMAKE_INSTALL_PREFIX}
  NO_DEFAULT_PATH
  )

aux_source_directory(src MODULE_SRC_LEGACY_NONE_LCIO)
list(FILTER MODULE_SRC_LEGACY_NONE_LCIO EXCLUDE REGEX "(.+)LCEventConverter(.+)")
add_library(eudaq_module_altel_legacy_none_lcio OBJECT ${MODULE_SRC_LEGACY_NONE_LCIO})
target_include_directories(eudaq_module_altel_legacy_none_lcio
  PRIVATE
  $<TARGET_PROPERTY:eudaq::core,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:altel-rbcp,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:mycommon,INTERFACE_INCLUDE_DIRECTORIES>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel_legacy")
  aux_source_directory(src MODULE_SRC_LEGACY_LCIO)
  list(FILTER MODULE_SRC_LEGACY_LCIO INCLUDE REGEX "(.+)LCEventConverter(.+)")
  # list(LENGTH   MODULE_SRC_LEGACY_LCIO    LEN_MODULE_SRC_LEGACY_LCIO)
  add_library(eudaq_module_altel_legacy_lcio OBJECT ${MODULE_SRC_LEGACY_LCIO})
  set_target_properties(eudaq_module_altel_legacy_lcio PROPERTIES CXX_STANDARD 14)
  target_include_directories(eudaq_module_altel_legacy_lcio
    PRIVATE
    $<TARGET_PROPERTY:eudaq::core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:eudaq::lcio,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:altel-rbcp,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:mycommon,INTERFACE_INCLUDE_DIRECTORIES>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
else()
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel_lecagy")
endif()

if(TARGET eudaq_module_altel_legacy_lcio)
  add_library(eudaq_module_altel_legacy SHARED
    $<TARGET_OBJECTS:eudaq_module_altel_legacy_none_lcio>
    $<TARGET_OBJECTS:eudaq_module_altel_legacy_lcio>)
  target_link_libraries(eudaq_module_altel_legacy eudaq::core eudaq::lcio altel-rbcp mycommon)
else()
  add_library(eudaq_module_altel_legacy SHARED
    $<TARGET_OBJECTS:eudaq_module_altel_legacy_none_lcio>)
  target_link_libraries(eudaq_module_altel_legacy eudaq::core altel-rbcp mycommon)
endif()


install(TARGETS
  eudaq_module_altel_legacy
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
