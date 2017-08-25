FILE(GLOB_RECURSE net_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cc"
  "*.cpp"
  "*.h"
)

FILE(GLOB_RECURSE net_unittests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*_unittest*"
)

FILE(GLOB_RECURSE net_sources_win RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*_win*"
  "winsock*"
)

FILE(GLOB net_sources_win_dir RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "win/*.*"
)

LIST(APPEND net_sources_win
  ${net_sources_win_dir}
)

FILE(GLOB_RECURSE net_sources_lin RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*_posix*"
  "*_linux*"
)

LIST(REMOVE_ITEM net_sources
  ${net_sources_win}
  ${net_sources_lin}
  ${net_unittests}
)

IF(WIN32)
  LIST(APPEND net_sources ${net_sources_win})
ELSE()
  LIST(APPEND net_sources ${net_sources_lin})
ENDIF()

ADD_DEFINITIONS(-DNET_IMPLEMENTATION)

ADD_LIBRARY(net ${LIBRARY_LINKAGE} ${net_sources})
TARGET_LINK_LIBRARIES(net scada_base)
