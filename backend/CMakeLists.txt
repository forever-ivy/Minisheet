cmake_minimum_required(VERSION 3.16)

project(minisheet LANGUAGES CXX C)

# 所有目标统一使用固定的语言标准。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows 下用 MSVC 编译时，按 UTF-8 读取源码，避免中文注释触发编码报错。
if (MSVC)
  add_compile_options(/utf-8)
endif()

# 后端核心库，只保留 OJ 和服务端都会用到的计算逻辑。
add_library(minisheet_core
  src/m1_types.cpp
  src/m2_workbook.cpp
  src/m3_display.cpp
  src/m4_formula.cpp
  src/m5_recalc.cpp
  third_party/tinyexpr/tinyexpr.c
)

target_include_directories(minisheet_core
  PUBLIC include
  PRIVATE third_party/tinyexpr
)

# OJ 验收程序，入口文件就是 oj_cli.cpp，Windows 下的 Release 产物就是 myxls.exe。
add_executable(myxls
  app/oj_cli.cpp
)

target_link_libraries(myxls PRIVATE minisheet_core)

# 桌面端使用的本地后端服务。
add_executable(minisheet_server
  app/minisheet_server.cpp
  src/m6_storage.cpp
  src/m7_api.cpp
)

target_include_directories(minisheet_server PRIVATE vendor)
target_link_libraries(minisheet_server PRIVATE minisheet_core)

if (WIN32)
  target_link_libraries(minisheet_server PRIVATE ws2_32)
endif()
