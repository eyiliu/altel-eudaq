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

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel")
endif()

add_library(eudaq_module_altel SHARED ${MODULE_SRC})
if(TARGET eudaq::lcio)
  target_link_libraries(eudaq_module_altel eudaq::core eudaq::lcio altel-rbcp mycommon)
else()
  target_link_libraries(eudaq_module_altel eudaq::core altel-rbcp mycommon)
endif()

set_target_properties(eudaq_module_altel PROPERTIES CXX_STANDARD 14)
# set_property(TARGET eudaq_module_altel APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})

install(TARGETS
  eudaq_module_altel
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

add_executable(oldEudaqRawDump exe/oldEudaqRawDump.cxx)
target_link_libraries(oldEudaqRawDump PRIVATE eudaq::core mycommon)
install(TARGETS
  oldEudaqRawDump
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
