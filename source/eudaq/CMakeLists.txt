option(ALTEL_BUILD_EUDAQ_PRODUCER "build Producer EUDAQ2?" ON)
if(NOT ALTEL_BUILD_EUDAQ_PRODUCER)
  return()
endif()

aux_source_directory(./ MODULE_SRC)

find_package(eudaq CONFIG
  COMPONENTS eudaq::core eudaq::lcio
  REQUIRED NO_DEFAULT_PATH)

include_directories(${EUDAQ_INCLUDE_DIRS})

list(FIND EUDAQ_LIBRARIES "eudaq_lcio" LCIO_FIND_POS)
if(${LCIO_FIND_POS} EQUAL -1)
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")
endif()

add_library(eudaq_module_altel SHARED ${MODULE_SRC})
set_property(TARGET eudaq_module_altel PROPERTY CXX_STANDARD 14)

target_link_libraries(eudaq_module_altel eudaq::core eudaq::lcio altel-rbcp mycommon)
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


add_subdirectory(legacy)
