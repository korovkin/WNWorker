cmake_minimum_required(VERSION 3.2)
project (wnworker)

add_definitions(-std=c++14)
add_definitions(-Wall)
# add_definitions(-Wno-deprecated-declarations)

set(WNWORKER_SOURCES
  main.cpp
  WNWorker.cpp
)

set(WNWORKER_HEADERS
  WNWorker.h
)

add_executable(
  wnworker
  ${WNWORKER_SOURCES}
  ${WNWORKER_HEADERS}
)

target_include_directories(
  wnworker SYSTEM
  PUBLIC /usr/local/include
)


target_link_libraries(
  wnworker
  -lpthread
  -lglog
  -lgflags
)

