configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h")

include_directories(".")

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
add_subdirectory(remote)
add_subdirectory(vidicon)
