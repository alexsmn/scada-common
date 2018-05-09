if(NOT BOOST_FOUND)
  find_package(Boost REQUIRED COMPONENTS system date_time regex)
endif()

file(GLOB sources "*.cpp" "*.h")

if(WIN32)
  file(GLOB sources_win "win/*.cpp" "win/*.h")
  list(APPEND sources ${sources_win})
endif()

add_library(scada_base ${sources})

target_include_directories(scada_base PUBLIC "..")
target_link_libraries(scada_base PUBLIC base Boost::boost Boost::system)
set_property(TARGET scada_base PROPERTY FOLDER ${scada_folder})
