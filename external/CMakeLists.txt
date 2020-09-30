include(ExternalProject)

externalproject_add(
  external_aidatlu_dev
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/aidatlu
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
  )
set_target_properties(external_aidatlu_dev PROPERTIES EXCLUDE_FROM_ALL TRUE)
message(STATUS "To build aidatlu package (dev), use commands:\n> git submodule update --init --recursive \n> make external_aidatlu_dev")

externalproject_add(
  external_eudaq_dev
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/eudaq
  CMAKE_ARGS -DEUDAQ_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DEUDAQ_BUILD_GUI=OFF -DEUDAQ_BUILD_STDEVENT_MONITOR=OFF -DEUDAQ_LIBRARY_BUILD_LCIO=ON -DUSER_EUDET_BUILD=ON -DUSER_TLU_BUILD=ON -DUSER_TLU_BUILD_EUDET=OFF
  )
set_target_properties(external_eudaq_dev PROPERTIES EXCLUDE_FROM_ALL TRUE)
message(STATUS "To build EUDAQ package (dev), use commands:\n> git submodule update --init --recursive \n> make external_eudaq_dev")

externalproject_add(
  external_aidatlu
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/stable/aidatlu
  GIT_REPOSITORY https://github.com/eyiliu/aidatlu.git
  GIT_PROGRESS ON
  GIT_CONFIG advice.detachedHead=false
  GIT_TAG v1.2-2020-August
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
  )
set_target_properties(external_aidatlu PROPERTIES EXCLUDE_FROM_ALL TRUE)
message("To build aidatlu package, use command:\n>  make external_aidatlu")

externalproject_add(
  external_eudaq
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/stable/eudaq
  GIT_REPOSITORY https://github.com/eudaq/eudaq.git
  GIT_PROGRESS ON
  GIT_CONFIG advice.detachedHead=false
  GIT_TAG fc77bf805509e910091917bfa4072ced3f5b3480 # 2.4.4 + 1 commit
  CMAKE_ARGS -DEUDAQ_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DEUDAQ_BUILD_GUI=OFF -DEUDAQ_BUILD_STDEVENT_MONITOR=OFF -DEUDAQ_LIBRARY_BUILD_LCIO=ON -DUSER_EUDET_BUILD=ON -DUSER_TLU_BUILD=ON -DUSER_TLU_BUILD_EUDET=OFF
  )
set_target_properties(external_eudaq PROPERTIES EXCLUDE_FROM_ALL TRUE)
message("To build EUDAQ package, use command:\n>  make external_eudaq")
