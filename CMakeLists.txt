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

install(FILES
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_system-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_system-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_thread-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_thread-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_log-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_log-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_log_setup-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_log_setup-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_filesystem-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_filesystem-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_date_time-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_date_time-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_regex-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_regex-vc141-mt-x32-1_68.dll>"
    "$<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug/boost_program_options-vc141-mt-gd-x32-1_68.dll>"
    "$<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo/boost_program_options-vc141-mt-x32-1_68.dll>"
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
