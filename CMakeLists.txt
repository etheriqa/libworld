cmake_minimum_required(VERSION 2.8)

project(libworld)

set(CMAKE_C_FLAGS "-std=c11 -Werror -Wpedantic -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g2")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g0 -DNDEBUG")

add_definitions(-D_POSIX_C_SOURCE=200809L)

if(WORLD_LTO)
  if(CMAKE_C_COMPILER_ID STREQUAL Clang)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL GNU)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
  endif()
endif()

find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
  add_definitions(-DWORLD_USE_POSIX_THREAD)
endif()

include(CheckIncludeFile)

CHECK_INCLUDE_FILE(poll.h WORLD_HAVE_POLL_H)
CHECK_INCLUDE_FILE(sys/epoll.h WORLD_HAVE_SYS_EPOLL_H)
CHECK_INCLUDE_FILE(sys/event.h WORLD_HAVE_SYS_EVENT_H)
CHECK_INCLUDE_FILE(sys/select.h WORLD_HAVE_SYS_SELECT_H)

if(NOT WORLD_IO_MULTIPLEXER)
  if(WORLD_HAVE_SYS_EPOLL_H)
    set(WORLD_IO_MULTIPLEXER epoll)
  elseif(WORLD_HAVE_SYS_EVENT_H)
    set(WORLD_IO_MULTIPLEXER kqueue)
  elseif(WORLD_HAVE_POLL_H)
    set(WORLD_IO_MULTIPLEXER poll)
  elseif(WORDL_HAVE_SYS_SELECT_H)
    set(WORLD_IO_MULTIPLEXER select)
  else()
    message(FATAL_ERROR "*** No I/O multiplexers are found ***")
  endif()
endif()

if(WORLD_IO_MULTIPLEXER STREQUAL select)
  add_definitions(-DWORLD_USE_SELECT)
  list(APPEND SOURCES src/world_io_select.c)
elseif(WORLD_IO_MULTIPLEXER STREQUAL poll)
  add_definitions(-DWORLD_USE_POLL)
  list(APPEND SOURCES src/world_io_poll.c)
elseif(WORLD_IO_MULTIPLEXER STREQUAL kqueue)
  add_definitions(-DWORLD_USE_KQUEUE)
  list(APPEND SOURCES src/world_io_kqueue.c)
elseif(WORLD_IO_MULTIPLEXER STREQUAL epoll)
  add_definitions(-DWORLD_USE_EPOLL)
  list(APPEND SOURCES src/world_io_epoll.c)
else()
  message(FATAL_ERROR "*** Unknown I/O multiplexer: ${WORLD_IO_MULTIPLEXER} ***")
endif()

list(APPEND HEADERS include/world.h)
list(APPEND HEADERS include/worldaux.h)

list(APPEND SOURCES src/world_allocator.c)
list(APPEND SOURCES src/world_circular.c)
list(APPEND SOURCES src/world_hash.c)
list(APPEND SOURCES src/world_io.c)
list(APPEND SOURCES src/world_hashtable.c)
list(APPEND SOURCES src/world_hashtable_bucket.c)
list(APPEND SOURCES src/world_hashtable_entry.c)
list(APPEND SOURCES src/world_hashtable_log.c)
list(APPEND SOURCES src/world_origin.c)
list(APPEND SOURCES src/world_origin_handler.c)
list(APPEND SOURCES src/world_origin_thread.c)
list(APPEND SOURCES src/world_replica.c)
list(APPEND SOURCES src/world_replica_handler.c)
list(APPEND SOURCES src/world_replica_thread.c)
list(APPEND SOURCES src/world_system.c)
list(APPEND SOURCES src/world_vector.c)
list(APPEND SOURCES src/worldaux_client.c)
list(APPEND SOURCES src/worldaux_server.c)

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

add_executable(e2e_worldaux test/e2e/worldaux.c)
target_link_libraries(e2e_worldaux world ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME e2e/worldaux COMMAND e2e_worldaux)

add_executable(bench_server test/bench/server.c)
target_link_libraries(bench_server world ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_client test/bench/client.c)
target_link_libraries(bench_client world ${CMAKE_THREAD_LIBS_INIT})

add_executable(example_server example/server.c)
target_link_libraries(example_server world ${CMAKE_THREAD_LIBS_INIT})

add_executable(example_client example/client.c)
target_link_libraries(example_client world ${CMAKE_THREAD_LIBS_INIT})
