cmake_minimum_required(VERSION 2.8)

project(libworld)

include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-Werror -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

CHECK_INCLUDE_FILE(poll.h WORLD_HAVE_POLL_H)
CHECK_INCLUDE_FILE(sys/epoll.h WORLD_HAVE_SYS_EPOLL_H)
CHECK_INCLUDE_FILE(sys/event.h WORLD_HAVE_SYS_EVENT_H)

if(WORLD_HAVE_SYS_EVENT_H)
    set(WORLD_USE_KQUEUE ON CACHE BOOL "Use kqueue")
elseif(WORLD_HAVE_SYS_EPOLL_H)
    set(WORLD_USE_EPOLL ON CACHE BOOL "Use epoll")
elseif(WORLD_HAVE_SYS_POLL_H)
    set(WORLD_USE_POLL ON CACHE BOOL "Use poll")
endif()

list(APPEND PUBLIC_HEADERS include/world/origin.h)
list(APPEND PUBLIC_HEADERS include/world/replica.h)
list(APPEND PUBLIC_HEADERS include/world/snapshot.h)

list(APPEND SOURCES world/bitwise.cc)
if(WORLD_USE_KQUEUE)
    list(APPEND SOURCES world/event_kqueue.cc)
    add_definitions(-DWORLD_USE_KQUEUE)
endif()
if(WORLD_USE_EPOLL)
    list(APPEND SOURCES world/event_epoll.cc)
    add_definitions(-DWORLD_USE_EPOLL)
endif()
if(WORLD_USE_POLL)
    list(APPEND SOURCES world/event_poll.cc)
    add_definitions(-DWORLD_USE_POLL)
endif()
list(APPEND SOURCES world/hash.cc)
list(APPEND SOURCES world/hashmap.cc)
list(APPEND SOURCES world/origin_internal.cc)
list(APPEND SOURCES world/origin_io_thread.cc)
list(APPEND SOURCES world/origin_writer.cc)
list(APPEND SOURCES world/replica_internal.cc)
list(APPEND SOURCES world/replica_io_thread.cc)
list(APPEND SOURCES world/snapshot_set.cc)

include_directories(include)
include_directories(.)

add_library(world ${PUBLIC_HEADERS} ${SOURCES})

enable_testing()

add_executable(bitwise_test world/bitwise_test.cc)
target_link_libraries(bitwise_test world)
add_test(NAME bitwise COMMAND bitwise_test)

add_executable(hashmap_test world/hashmap_test.cc)
target_link_libraries(hashmap_test world)
add_test(NAME hashmap COMMAND hashmap_test)

add_executable(world_test world/world_test.cc)
target_link_libraries(world_test world)
add_test(NAME world COMMAND world_test)
