include(ExternalProject)

externalproject_add(
  external_aidatlu
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/aidatlu
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
  )
set_target_properties(external_aidatlu PROPERTIES EXCLUDE_FROM_ALL TRUE)
message("To build aidatlu package, use command:  make external_aidatlu")

externalproject_add(
  external_eudaq
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/eudaq
  CMAKE_ARGS -DEUDAQ_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DEUDAQ_BUILD_GUI=OFF -DEUDAQ_BUILD_STDEVENT_MONITOR=OFF -DEUDAQ_LIBRARY_BUILD_LCIO=ON -DUSER_EUDET_BUILD=ON -DUSER_TLU_BUILD=ON -DUSER_TLU_BUILD_EUDET=OFF
  )
set_target_properties(external_aidatlu PROPERTIES EXCLUDE_FROM_ALL TRUE)
message("To build EUDAQ package, use command:  make external_eudaq")
