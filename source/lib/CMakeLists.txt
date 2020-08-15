add_subdirectory(rbcp)

aux_source_directory(src LIB_SRC)
add_library(altel-rbcp SHARED ${LIB_SRC} )
target_link_libraries(altel-rbcp PRIVATE mycommon rbcp)

set(ALTEL_REG_JSON ${CMAKE_CURRENT_LIST_DIR}/misc/altel_reg_cmd_list.json)
set(ALTEL_REG_JSON_HH  ${CMAKE_BINARY_DIR}/altel_reg_cmd_list.hh)
if( (NOT EXISTS ${ALTEL_REG_JSON_HH}) OR (${ALTEL_REG_JSON} IS_NEWER_THAN ${ALTEL_REG_JSON_HH}) )
  set(HH_DELIM "~~~cpp~raw~~~")
  file(READ ${ALTEL_REG_JSON} ALTEL_REG_JSON_CONTENT)
  set(ALTEL_REG_JSON_CONTENT "R\"${HH_DELIM}(${ALTEL_REG_JSON_CONTENT})${HH_DELIM}\"")
  file(WRITE ${ALTEL_REG_JSON_HH} "${ALTEL_REG_JSON_CONTENT}")
endif()


target_include_directories(altel-rbcp
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  )

install(TARGETS altel-rbcp
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
