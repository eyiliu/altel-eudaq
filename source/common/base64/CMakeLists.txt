set(LIB_SRC base64.cc)
add_library(base64 STATIC ${LIB_SRC})
target_include_directories(base64
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

set(LIB_PUBLIC_HEADERS base64.hh)
set_target_properties(base64 PROPERTIES PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}")
set_target_properties(base64 PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS base64
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
