include(CMakeToolsHelpers OPTIONAL)

# Boost
# SET(BOOST_ROOT "${deps}/boost-1.63.0")
# SET(BOOST_LIBRARYDIR "${deps}/boost-1.63.0/stage")
SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

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

# ProtoBuf
ADD_SUBDIRECTORY("${deps}/protobuf" "protobuf-build")

# OPC (Windows-only)
IF(WIN32)
  ADD_SUBDIRECTORY("${deps}/OPCFoundation" "OPCFoundation-build")
ENDIF()

ADD_SUBDIRECTORY("${deps}/chromebase" chromebase)
ADD_SUBDIRECTORY("${deps}/express" "express")
ADD_SUBDIRECTORY(memdb)
ADD_SUBDIRECTORY(net)

ADD_DEFINITIONS(
  -DPROTOBUF_USE_EXCEPTIONS=0)

ADD_SUBDIRECTORY(base)

ADD_SUBDIRECTORY(opcua)
set(opcua_common_sources ${opcua_common_sources} PARENT_SCOPE)

ADD_SUBDIRECTORY(common)

ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(remote)
