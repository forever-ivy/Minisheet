# 设置 CMake 最低版本要求
# 版本 3.16 支持了一些现代 CMake 特性，如 target_precompile_headers
cmake_minimum_required(VERSION 3.16)

# 定义项目名称及使用的编程语言
# minisheet 是这个项目的名称，使用 C++ 和 C 两种语言
project(minisheet LANGUAGES CXX C)

# ==================== 编译器标准设置 ====================

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用 C++17，如果编译器不支持则报错
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器特定的扩展（如 GNU 扩展），确保代码具有更好的可移植性
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 C 语言标准为 C99
set(CMAKE_C_STANDARD 99)
# 强制要求使用 C99
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== 核心库定义 ====================

# 创建一个静态库目标 minisheet_core
# 该库包含 Minisheet 的所有核心功能模块
add_library(minisheet_core
  src/m1_types.cpp      # 基础数据类型定义（单元格、坐标等）
  src/m2_workbook.cpp   # 工作簿管理（多个工作表）
  src/m3_display.cpp    # 显示相关功能（格式化、渲染）
  src/m4_formula.cpp    # 公式解析器
  src/m5_recalc.cpp     # 公式重新计算引擎
  src/m6_storage.cpp    # 文件存储/序列化（保存/加载）
  src/m7_api.cpp        # 对外提供的 API 接口
  third_party/tinyexpr/tinyexpr.c  # 第三方表达式求值库
)

# 设置 minisheet_core 库的头文件搜索路径
# PUBLIC: 使用这个库的目标也能看到这些头文件路径
# PRIVATE: 只有编译这个库本身时使用这些路径
target_include_directories(minisheet_core
  PUBLIC include                    # 对外公开的 API 头文件
  PRIVATE third_party/tinyexpr ../vendor  # 仅内部使用的第三方库头文件
)

# ==================== 可执行程序定义 ====================

# 创建命令行版本的可执行程序
add_executable(minisheet_cli
  app/minisheet_cli.cpp   # CLI 应用程序入口
)

# 将核心库链接到 CLI 程序
# PRIVATE 表示这个依赖关系不会传递给其他链接 minisheet_cli 的目标
target_link_libraries(minisheet_cli PRIVATE minisheet_core)

# 创建服务器版本的可执行程序
add_executable(minisheet_server
  app/minisheet_server.cpp  # 服务器应用程序入口（可能用于与前端通信）
)

# 为服务器程序添加额外的头文件搜索路径
# 需要访问 vendor 目录下的库（如 WebSocket、HTTP 库等）
target_include_directories(minisheet_server PRIVATE ../vendor)

# 将核心库链接到服务器程序
target_link_libraries(minisheet_server PRIVATE minisheet_core)

if (WIN32)
  target_link_libraries(minisheet_server PRIVATE ws2_32)
endif()
