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
- `vendor/`: 第三方头文件

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

## CLI 命令

```bash
./backend/build/minisheet_cli import examples/basic-demo.csv
./backend/build/minisheet_cli save examples/basic-demo.csv /tmp/minisheet.dat
./backend/build/minisheet_cli load /tmp/minisheet.dat
./backend/build/minisheet_cli pack examples/basic-demo.csv /tmp/minisheet.dat
./backend/build/minisheet_cli unpack /tmp/minisheet.dat /tmp/minisheet.csv
```

## 老师验收建议流程

1. 用 CLI 导入测试 CSV
2. 保存 DAT 并重新加载 DAT
3. 再打开前端页面做表格编辑和公式演示

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

- `make:mac` 需要在 macOS 上执行
- `make:win` 现在默认打 `win32/x64`
- 如果你在 macOS 上交叉打 Windows 包，需要先安装 `mingw-w64`：

```bash
brew install mingw-w64
```

- 当前产物是未签名的 zip 便携包，首次打开时系统可能会提示未知来源
