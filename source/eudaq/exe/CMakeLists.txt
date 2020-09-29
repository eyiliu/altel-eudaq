add_executable(ConverterEudaqAltel2Json ConverterEudaqAltel2Json.cc)
list(APPEND EXE_TARGET_LIST ConverterEudaqAltel2Json)
target_link_libraries(ConverterEudaqAltel2Json PRIVATE
  altel-rbcp
  rapidjson
  eudaq::core
  mycommon
  )

add_executable(ConverterEudaqJade2Json ConverterEudaqJade2Json.cc)
list(APPEND EXE_TARGET_LIST ConverterEudaqJade2Json)
target_link_libraries(ConverterEudaqJade2Json PRIVATE
  altel-rbcp
  rapidjson
  eudaq::core
  mycommon
  )

add_executable(oldEudaqRawDump oldEudaqRawDump.cxx)
list(APPEND EXE_TARGET_LIST oldEudaqRawDump)
target_link_libraries(oldEudaqRawDump PRIVATE eudaq::core mycommon)

# install-tree
install(TARGETS ${EXE_TARGET_LIST}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
