cmake_minimum_required(VERSION 3.10)
project(a26_card_harness C)

set(CMAKE_C_STANDARD 11)
set(BUILD_QT OFF CACHE BOOL "" FORCE)
set(BUILD_SDL OFF CACHE BOOL "" FORCE)
set(BUILD_GL OFF CACHE BOOL "" FORCE)
set(BUILD_GLES2 OFF CACHE BOOL "" FORCE)
set(BUILD_GLES3 OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(USE_FFMPEG OFF CACHE BOOL "" FORCE)
set(USE_LUA OFF CACHE BOOL "" FORCE)
set(USE_LIBZIP OFF CACHE BOOL "" FORCE)
set(USE_SQLITE3 OFF CACHE BOOL "" FORCE)
set(USE_MINIZIP OFF CACHE BOOL "" FORCE)
set(USE_DEBUGGERS OFF CACHE BOOL "" FORCE)
set(USE_GDB_STUB OFF CACHE BOOL "" FORCE)
set(USE_ELF OFF CACHE BOOL "" FORCE)
set(USE_EPOXY OFF CACHE BOOL "" FORCE)
set(M_CORE_GB OFF CACHE BOOL "" FORCE)
set(M_CORE_GBA ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/mgba EXCLUDE_FROM_ALL)

add_library(supercard_model STATIC src/supercard_model.c)
target_include_directories(supercard_model PUBLIC include)

add_executable(supercard-model-test tests/supercard_model_test.c)
target_link_libraries(supercard-model-test PRIVATE supercard_model)
target_compile_options(supercard-model-test PRIVATE -UNDEBUG)

add_library(ezflash_sd_model STATIC src/ezflash_sd_model.c)
target_include_directories(ezflash_sd_model PUBLIC include)
add_executable(ezflash-sd-model-test tests/ezflash_sd_model_test.c)
target_link_libraries(ezflash-sd-model-test PRIVATE ezflash_sd_model)
target_compile_options(ezflash-sd-model-test PRIVATE -UNDEBUG)

if(TARGET mgba)
  add_library(supercard_mgba_bus STATIC src/supercard_mgba_bus.c)
  target_include_directories(supercard_mgba_bus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(supercard_mgba_bus PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(supercard_mgba_bus PUBLIC supercard_model mgba)

  add_executable(mgba-core-smoke src/mgba_core_smoke.c)
  target_include_directories(mgba-core-smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(mgba-core-smoke PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(mgba-core-smoke PRIVATE supercard_mgba_bus)

  add_library(ezflash_mgba_trace STATIC src/ezflash_mgba_trace.c)
  target_include_directories(ezflash_mgba_trace PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(ezflash_mgba_trace PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(ezflash_mgba_trace PUBLIC mgba)

  add_library(ezflash_mgba_sd STATIC src/ezflash_mgba_sd.c)
  target_include_directories(ezflash_mgba_sd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(ezflash_mgba_sd PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(ezflash_mgba_sd PUBLIC ezflash_sd_model mgba)

  add_executable(ezflash-mgba-sd-smoke src/ezflash_mgba_sd_smoke.c)
  target_include_directories(ezflash-mgba-sd-smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(ezflash-mgba-sd-smoke PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(ezflash-mgba-sd-smoke PRIVATE ezflash_mgba_sd)

  add_executable(ezflash-mgba-runner src/ezflash_mgba_runner.c)
  target_include_directories(ezflash-mgba-runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(ezflash-mgba-runner PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(ezflash-mgba-runner PRIVATE ezflash_mgba_sd)

  add_executable(ezflash-mgba-trace-smoke src/ezflash_mgba_trace_smoke.c)
  target_include_directories(ezflash-mgba-trace-smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(ezflash-mgba-trace-smoke PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(ezflash-mgba-trace-smoke PRIVATE ezflash_mgba_trace)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/mgba_supercard_runner.c")
  add_executable(supercard-mgba-runner src/mgba_supercard_runner.c)
  target_include_directories(supercard-mgba-runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
    ${CMAKE_BINARY_DIR}/third_party/mgba/include)
  target_compile_definitions(supercard-mgba-runner PRIVATE
    $<TARGET_PROPERTY:mgba,COMPILE_DEFINITIONS>)
  target_link_libraries(supercard-mgba-runner PRIVATE supercard_mgba_bus)
else()
  message(STATUS "mGBA SuperCard runner is not implemented; model and core-smoke targets remain available")
endif()
