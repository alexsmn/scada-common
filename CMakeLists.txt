include(CMakeToolsHelpers OPTIONAL)

SET(Boost_USE_STATIC_LIBS ON)

# OPC-UA
set(OPENSSL_ROOT_DIR "${deps}/openssl")
set(OPCUA_ROOT_DIR "${deps}/UA-AnsiC/Stack")
set(OPCUA_DIR "${deps}/opcuapp")
set(OPCUAPP_DIR "${deps}/opcuapp")

# This is not working:
# SET(CMAKE_CXX_STANDARD 11)

# SET(PRECOMPILE "${PROJECT_SOURCE_DIR}/src/build/precompile.cc")
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Yuprecompile.h /FIprecompile.h")
# SET_SOURCE_FILES_PROPERTIES(${PRECOMPILE} PROPERTIES COMPILE_FLAGS "/Ycprecompile.h")

# OPC (Windows-only)
IF(WIN32)
  add_subdirectory("${deps}/OPCFoundation" "OPCFoundation-build")
ENDIF()

add_subdirectory("${deps}/chromebase" chromebase)
add_subdirectory("${deps}/express" "express")
add_subdirectory(memdb)
add_subdirectory(net)

ADD_DEFINITIONS(-DPROTOBUF_USE_EXCEPTIONS=0)

add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(opcua)
add_subdirectory(remote)
add_subdirectory(vidicon)

set_property(TARGET scada_base PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_common PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_common_unittests PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_unittests PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_opcua PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_remote PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_vidicon PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_remote PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_core_remote_unittests PROPERTY FOLDER ${scada_folder})
