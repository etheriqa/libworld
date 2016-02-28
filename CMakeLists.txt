cmake_minimum_required(VERSION 2.8)

project(libworld CXX)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-Werror -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -flto")

include_directories(include)
include_directories(.)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_EVENT REQUIRED libevent)
include_directories(${PC_EVENT_INCLUDE_DIRS})
find_library(EVENT_LIBRARY
    NAMES event_core
    PATHS ${PC_EVENT_LIBRARY_DIR})

add_library(world
    include/world/origin.h
    include/world/replica.h
    include/world/snapshot.h
    world/bitwise.cc
    world/hash.cc
    world/hashmap.cc
    world/origin_internal.cc
    world/replica_internal.cc
    world/snapshot_set.cc
    )

enable_testing()

add_executable(bitwise_test world/bitwise_test.cc)
target_link_libraries(bitwise_test world)
add_test(NAME bitwise COMMAND bitwise_test)

add_executable(hashmap_test world/hashmap_test.cc)
target_link_libraries(hashmap_test world)
add_test(NAME hashmap COMMAND hashmap_test)

add_executable(origin_test world/origin_test.cc)
target_link_libraries(origin_test world ${EVENT_LIBRARY})
add_test(NAME origin COMMAND origin_test)

add_executable(replica_test world/replica_test.cc)
target_link_libraries(replica_test world ${EVENT_LIBRARY})
add_test(NAME replica COMMAND replica_test)
