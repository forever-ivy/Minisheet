# Minisheet

一个面向课程复刻任务的简化版电子表格项目。

## 技术栈

- 后端：C++17
- 前端：React + TypeScript
- 表达式内核：`tinyexpr`
- HTTP：`cpp-httplib`
- JSON：`nlohmann/json`

## 功能范围

- 最多 `32767` 行、`256` 列
- 支持整数、浮点数、字符串、公式
- 公式支持 `+ - * / %`、括号、一元正负号、单元格引用
- 支持 `sqrt`、`abs`、`sum(A1:B3)`、`avg(A1:B3)`
- 字符串参与数值运算、非法引用、循环引用统一返回 `#NA`
- 支持 CSV 导入、自定义 DAT 保存和 DAT 回读

## 目录结构

- `backend/`: `M1-M7` 后端核心、CLI、HTTP 服务
- `frontend/`: `M8-M10` 前端展示、编辑和状态
- `electron/`: Electron 桌面壳、打包脚本与运行时桥接
- `examples/`: 演示用 CSV 样例
- `scripts/`: 启动和演示脚本
- `backend/vendor/`: 后端使用的第三方头文件

## 启动后端

```bash
cmake -S backend -B backend/build
cmake --build backend/build
./backend/build/minisheet_server
```

或者直接运行：

```bash
bash scripts/start_demo.sh
```

## OJ 验收 CLI

```bash
cmake -S backend -B backend/build
cmake --build backend/build --target myxls
./backend/build/myxls < input.txt
```

这个入口专门用于 PTA `7-4 SimpleGrid复杂公式` 验收：

- 从标准输入读取 `m n` 与后续 `m` 行逗号分隔单元格
- 支持数字、空白、`=公式`、`+ - * /`、括号、`sin/cos/sqrt`、`A1-Z100` 引用
- 输出固定两位小数，每个单元格后保留一个空格
- 错误公式、越界引用、循环引用统一输出 `0.00`

Windows 验收机上的目标产物是 `myxls.exe`。构建后把它放到 `D:\testxls\myxls.exe`，再运行老师提供的 `TestGrid.exe` 即可。

## Electron 桌面版

先安装 Electron 依赖：

```bash
npm --prefix electron install
```

开发模式会同时启动 React dev server、构建后端并拉起 Electron：

```bash
npm --prefix electron run dev
```

打包前会自动构建前端和当前平台的后端二进制：

```bash
npm --prefix electron run make:mac
npm --prefix electron run make:win
```

说明：

- `make:mac` 在 macOS 上打 Apple Silicon `darwin/arm64` 包
- `make:win` 保持现有 Forge 流程，目标为 `win32/x64`，会生成 Windows 安装器产物（含 `Setup.exe`）以及 zip 包
- 如果你在 macOS 上交叉打 Windows 包，需要先安装 `mingw-w64`：

```bash
brew install mingw-w64
```

- 由于 Electron Forge 的 Squirrel.Windows maker 不支持在 macOS 主机上构建 Windows 安装器，`make:win` 的 `Setup.exe` 需要在 Windows 主机，或安装了 wine/mono 的 Linux 环境中执行
- 当前产物是未签名包，首次打开或安装时系统可能会提示未知来源
