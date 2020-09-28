add_library(factory INTERFACE)
target_include_directories(factory
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

set(LIB_PUBLIC_HEADERS Factory.hh)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15.0") 
  set_target_properties(factory PROPERTIES PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}")  
else()
  message(STATUS "Unable to export PUBLIC_HEADER from interface target by CMake < 3.15, using CMake INSTALL FILES alternatively")
  install(FILES ${LIB_PUBLIC_HEADERS} DESTINATION include)
endif()

install(TARGETS factory
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
