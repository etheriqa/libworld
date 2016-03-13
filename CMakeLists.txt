cmake_minimum_required(VERSION 2.8)

project(libworld)

set(CMAKE_C_FLAGS "-std=c11 -Werror -Wpedantic -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g2")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g0 -DNDEBUG")

include(CheckIncludeFile)

find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
    set(WORLD_USE_PTHREAD ON)
endif()

CHECK_INCLUDE_FILE(poll.h WORLD_HAVE_POLL_H)
if(WORLD_HAVE_POLL_H)
    set(WORLD_USE_POLL ON)
endif()

CHECK_INCLUDE_FILE(sys/event.h WORLD_HAVE_SYS_EVENT_H)
if(WORLD_HAVE_SYS_EVENT_H)
    set(WORLD_USE_POLL OFF)
    set(WORLD_USE_KQUEUE ON)
endif()

CHECK_INCLUDE_FILE(sys/epoll.h WORLD_HAVE_SYS_EPOLL_H)
if(WORLD_HAVE_SYS_EPOLL_H)
    set(WORLD_USE_POLL OFF)
    set(WORLD_USE_EPOLL ON)
endif()

if(WORLD_USE_PTHREAD)
    add_definitions(-DWORLD_USE_PTHREAD)
endif()

if(WORLD_USE_POLL)
    add_definitions(-DWORLD_USE_POLL)
    list(APPEND SOURCES src/io_poll.c)
endif()

if(WORLD_USE_KQUEUE)
    add_definitions(-DWORLD_USE_KQUEUE)
    list(APPEND SOURCES src/io_kqueue.c)
endif()

if(WORLD_USE_EPOLL)
    add_definitions(-DWORLD_USE_EPOLL)
    list(APPEND SOURCES src/io_epoll.c)
endif()

list(APPEND HEADERS include/world.h)

list(APPEND SOURCES src/hash.c)
list(APPEND SOURCES src/hashtable.c)
list(APPEND SOURCES src/origin.c)
list(APPEND SOURCES src/origin_iohandler.c)
list(APPEND SOURCES src/origin_iothread.c)
list(APPEND SOURCES src/replica.c)
list(APPEND SOURCES src/replica_iohandler.c)
list(APPEND SOURCES src/replica_iothread.c)

include_directories(include)

add_library(world ${HEADERS} ${SOURCES})

enable_testing()

add_executable(unit_vector test/unit/vector.c)
target_link_libraries(unit_vector world)
add_test(NAME unit/vector COMMAND unit_vector)

add_executable(unit_circular test/unit/circular.c)
target_link_libraries(unit_circular world)
add_test(NAME unit/circular COMMAND unit_circular)

add_executable(unit_hash test/unit/hash.c)
target_link_libraries(unit_hash world)
add_test(NAME unit/hash COMMAND unit_hash)

add_executable(unit_hashtable test/unit/hashtable.c)
target_link_libraries(unit_hashtable world)
add_test(NAME unit/hashtable COMMAND unit_hashtable)

add_executable(e2e_world test/e2e/world.c)
target_link_libraries(e2e_world world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/world COMMAND e2e_world)
