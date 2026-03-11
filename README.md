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
- `examples/`: 演示用 CSV 样例
- `scripts/`: 启动服务、接口 smoke test
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
```

## 老师验收建议流程

1. 用 CLI 导入测试 CSV
2. 保存 DAT 并重新加载 DAT
3. 再打开前端页面做表格编辑和公式演示

## 当前验证命令

```bash
bash scripts/test_server_api.sh
```
