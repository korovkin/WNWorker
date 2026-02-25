cmake_minimum_required(VERSION 4.2)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
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

include(FetchContent)

FetchContent_Declare(
  gflags
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG        v2.2.2
)

FetchContent_Declare(
  glog
  GIT_REPOSITORY https://github.com/google/glog.git
  GIT_TAG        v0.7.0
)

FetchContent_MakeAvailable(gflags glog)

add_executable(
  wnworker
  ${WNWORKER_SOURCES}
  ${WNWORKER_HEADERS}
)

target_link_libraries(
  wnworker
  glog::glog
  gflags
  -lpthread
)

