cmake_minimum_required(VERSION 3.16)

project(minisheet LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(minisheet_core
  src/m1_types.cpp
  src/m2_workbook.cpp
  src/m3_display.cpp
  src/m4_formula.cpp
  src/m5_recalc.cpp
  src/m6_storage.cpp
  src/m7_api.cpp
  third_party/tinyexpr/tinyexpr.c
)

target_include_directories(minisheet_core
  PUBLIC include
  PRIVATE third_party/tinyexpr ../vendor
)

add_executable(minisheet_cli
  app/minisheet_cli.cpp
)

target_link_libraries(minisheet_cli PRIVATE minisheet_core)

add_executable(minisheet_server
  app/minisheet_server.cpp
)

target_include_directories(minisheet_server PRIVATE ../vendor)
target_link_libraries(minisheet_server PRIVATE minisheet_core)

add_executable(minisheet_storage_roundtrip_test
  tests/storage_roundtrip.cpp
)

target_link_libraries(minisheet_storage_roundtrip_test PRIVATE minisheet_core)

if (WIN32)
  target_link_libraries(minisheet_server PRIVATE ws2_32)
endif()
