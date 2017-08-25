FILE(GLOB memdb_sources
	"*.cpp"
	"*.h")

FILE(GLOB memdb_unittests
	"*_unittest.cpp"
	"*_unittest.h")

LIST(REMOVE_ITEM memdb_sources ${memdb_unittests})

ADD_LIBRARY(memdb STATIC ${memdb_sources})

TARGET_LINK_LIBRARIES(memdb scada_base)
