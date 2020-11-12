add_subdirectory(legacy)

option(ALTEL_BUILD_EUDAQ_MODULE "build lagecy eudaq module (AltelProducer and Altel Converters)" ON)
if(NOT ALTEL_BUILD_EUDAQ_MODULE)
  return()
endif()
message(STATUS "Module eudaq_module_altel is enabled")

aux_source_directory(src MODULE_SRC)

find_package(eudaq REQUIRED
  COMPONENTS eudaq::core
  OPTIONAL_COMPONENTS eudaq::lcio
  CONFIG
  PATHS ${CMAKE_INSTALL_PREFIX}
  NO_DEFAULT_PATH
  )

aux_source_directory(src MODULE_SRC_NONE_LCIO)
list(FILTER MODULE_SRC_NONE_LCIO EXCLUDE REGEX "(.+)LCEventConverter(.+)")
add_library(eudaq_module_altel_none_lcio OBJECT ${MODULE_SRC_NONE_LCIO})
target_include_directories(eudaq_module_altel_none_lcio
  PRIVATE
  $<TARGET_PROPERTY:eudaq::core,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:altel-rbcp,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:mycommon,INTERFACE_INCLUDE_DIRECTORIES>
)

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel")
  aux_source_directory(src MODULE_SRC_LCIO)
  list(FILTER MODULE_SRC_LCIO INCLUDE REGEX "(.+)LCEventConverter(.+)")
  # list(LENGTH   MODULE_SRC_LCIO    LEN_MODULE_SRC_LCIO)
  add_library(eudaq_module_altel_lcio OBJECT ${MODULE_SRC_LCIO})
  set_target_properties(eudaq_module_altel_lcio PROPERTIES CXX_STANDARD 14)
  target_include_directories(eudaq_module_altel_lcio
    PRIVATE
    $<TARGET_PROPERTY:eudaq::core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:eudaq::lcio,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:altel-rbcp,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:mycommon,INTERFACE_INCLUDE_DIRECTORIES>
    )
else()
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel")
endif()

if(TARGET eudaq_module_altel_lcio)
  add_library(eudaq_module_altel SHARED
    $<TARGET_OBJECTS:eudaq_module_altel_none_lcio>
    $<TARGET_OBJECTS:eudaq_module_altel_lcio>)
  target_link_libraries(eudaq_module_altel eudaq::core eudaq::lcio altel-rbcp mycommon)
else()
  add_library(eudaq_module_altel SHARED
    $<TARGET_OBJECTS:eudaq_module_altel_none_lcio>)
  target_link_libraries(eudaq_module_altel eudaq::core altel-rbcp mycommon)
endif()

# set_property(TARGET eudaq_module_altel APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})

install(TARGETS
  eudaq_module_altel
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


add_subdirectory(exe)
