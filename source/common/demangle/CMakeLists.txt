set(LIB_SRC cxa_demangle.cc undname.cc)
add_library(demangle STATIC ${LIB_SRC})
target_include_directories(demangle
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

set(LIB_PUBLIC_HEADERS demangle.hh)
set_target_properties(demangle PROPERTIES PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}")
set_target_properties(demangle PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS demangle
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
