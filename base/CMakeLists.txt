find_package(Boost REQUIRED COMPONENTS system date_time regex)

file(GLOB sources "*.cpp" "*.h")

if(WIN32)
  file(GLOB sources_win "win/*.cpp" "win/*.h")
  list(APPEND sources ${sources_win})
endif()

add_library(scada_base ${sources})

target_include_directories(scada_base PUBLIC ".." ${Boost_INCLUDE_DIRS})
target_link_libraries(scada_base PUBLIC base ${Boost_LIBRARIES})