include_directories(".")

# OPC-UA
set(OPENSSL_ROOT_DIR "${deps}/openssl")
set(OPCUA_ROOT_DIR "${deps}/UA-AnsiC")
set(OPCUA_DIR "${deps}/opcuapp")
set(OPCUAPP_DIR "${deps}/opcuapp")

add_subdirectory("${deps}/googletest" gtest)
add_subdirectory("${deps}/icu" icu)

set(GTEST_INCLUDE_DIR "${deps}/googletest/googletest/include")
set(GMOCK_INCLUDE_DIR "${deps}/googletest/googlemock/include")

set(CHROME_BASE_ROOT "${deps}/chromebase")
add_subdirectory(${CHROME_BASE_ROOT} chromebase)

add_subdirectory("${deps}/express" express)
add_subdirectory("${deps}/net" net)

add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(address_space)
add_subdirectory(opcua)
add_subdirectory(remote)

if(WIN32)
  add_subdirectory(vidicon)
endif()

if(WIN32)
  file(GLOB VC_REDIST_FILES "${VC_REDIST}/*")
  install(FILES ${VC_REDIST_FILES}
    DESTINATION bin
    COMPONENT Common)

  set(BOOST_COMPONENTS thread log log_setup filesystem date_time regex program_options)
  find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

  set(BOOST_DEBUG_SUFFIX "vc142-mt-gd-x32-${Boost_LIB_VERSION}")
  set(BOOST_RELEASE_SUFFIX "vc142-mt-x32-${Boost_LIB_VERSION}")

  foreach(BOOST_COMPONENT ${BOOST_COMPONENTS})
    install(FILES
        "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_${BOOST_COMPONENT}-${BOOST_DEBUG_SUFFIX}.dll>"
        "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_${BOOST_COMPONENT}-${BOOST_RELEASE_SUFFIX}.dll>"
      DESTINATION bin
      COMPONENT Common)
  endforeach()

  install(FILES
      "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/libprotobufd.dll>"
      "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/libprotobuf.dll>"
      "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/OPCUA.dll>"
      "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/OPCUA.dll>"
      "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/libeay32.dll>"
      "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/libeay32.dll>"
      "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/ssleay32.dll>"
      "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/ssleay32.dll>"
    DESTINATION bin
    COMPONENT Common)

endif(WIN32)