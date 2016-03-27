cmake_minimum_required(VERSION 2.8)

project(libworld)

set(CMAKE_C_FLAGS "-std=c11 -Werror -Wpedantic -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g2")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g0 -DNDEBUG")

if(NOT WORLD_DISABLE_LTO)
    if(CMAKE_C_COMPILER_ID STREQUAL Clang)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL GNU)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
    endif()
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)

find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-DWORLD_USE_POSIX_THREAD)
endif()

include(CheckIncludeFile)

CHECK_INCLUDE_FILE(poll.h WORLD_HAVE_POLL_H)
CHECK_INCLUDE_FILE(sys/epoll.h WORLD_HAVE_SYS_EPOLL_H)
CHECK_INCLUDE_FILE(sys/event.h WORLD_HAVE_SYS_EVENT_H)
CHECK_INCLUDE_FILE(sys/select.h WORLD_HAVE_SYS_SELECT_H)

if(NOT WORLD_IO_MULTIPLEX_METHOD)
    if(WORLD_HAVE_SYS_EPOLL_H)
        set(WORLD_IO_MULTIPLEX_METHOD epoll)
    elseif(WORLD_HAVE_SYS_EVENT_H)
        set(WORLD_IO_MULTIPLEX_METHOD kqueue)
    elseif(WORLD_HAVE_POLL_H)
        set(WORLD_IO_MULTIPLEX_METHOD poll)
    elseif(WORDL_HAVE_SYS_SELECT_H)
        set(WORLD_IO_MULTIPLEX_METHOD select)
    else()
        message(FATAL_ERROR "*** No I/O multiplex methods are found ***")
    endif()
endif()

if(WORLD_IO_MULTIPLEX_METHOD STREQUAL select)
    add_definitions(-DWORLD_USE_SELECT)
    list(APPEND SOURCES src/world_io_select.c)
elseif(WORLD_IO_MULTIPLEX_METHOD STREQUAL poll)
    add_definitions(-DWORLD_USE_POLL)
    list(APPEND SOURCES src/world_io_poll.c)
elseif(WORLD_IO_MULTIPLEX_METHOD STREQUAL kqueue)
    add_definitions(-DWORLD_USE_KQUEUE)
    list(APPEND SOURCES src/world_io_kqueue.c)
elseif(WORLD_IO_MULTIPLEX_METHOD STREQUAL epoll)
    add_definitions(-DWORLD_USE_EPOLL)
    list(APPEND SOURCES src/world_io_epoll.c)
else()
    message(FATAL_ERROR "*** Unknown I/O multiplex method: ${WORLD_IO_MULTIPLEX_METHOD} ***")
endif()

list(APPEND HEADERS include/world.h)

list(APPEND SOURCES src/world_allocator.c)
list(APPEND SOURCES src/world_circular.c)
list(APPEND SOURCES src/world_hash.c)
list(APPEND SOURCES src/world_hashtable.c)
list(APPEND SOURCES src/world_hashtable_bucket.c)
list(APPEND SOURCES src/world_hashtable_entry.c)
list(APPEND SOURCES src/world_hashtable_log.c)
list(APPEND SOURCES src/world_origin.c)
list(APPEND SOURCES src/world_origin_iohandler.c)
list(APPEND SOURCES src/world_origin_iothread.c)
list(APPEND SOURCES src/world_replica.c)
list(APPEND SOURCES src/world_replica_iohandler.c)
list(APPEND SOURCES src/world_replica_iothread.c)
list(APPEND SOURCES src/world_system.c)
list(APPEND SOURCES src/world_vector.c)

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

add_executable(unit_io test/unit/io.c)
target_link_libraries(unit_io world)
add_test(NAME unit/io COMMAND unit_io)

add_executable(unit_hashtable test/unit/hashtable.c)
target_link_libraries(unit_hashtable world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME unit/hashtable COMMAND unit_hashtable)

add_executable(e2e_protocol_origin test/e2e/protocol_origin.c)
target_link_libraries(e2e_protocol_origin world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/protocol_origin COMMAND e2e_protocol_origin)

add_executable(e2e_protocol_replica test/e2e/protocol_replica.c)
target_link_libraries(e2e_protocol_replica world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/protocol_replica COMMAND e2e_protocol_replica)

add_executable(e2e_world test/e2e/world.c)
target_link_libraries(e2e_world world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/world COMMAND e2e_world)

add_executable(e2e_c1k test/e2e/c1k.c)
target_link_libraries(e2e_c1k world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/c1k COMMAND e2e_c1k)

add_executable(bench_log test/bench/log.c)
target_link_libraries(bench_log world ${CMAKE_THREAD_LIBS_INIT})
