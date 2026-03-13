#pragma once

// ========================================
// API接口模块
// 提供把Workbook转换成JSON格式的功能
// 主要用于服务器给前端返回数据
// 这是最靠近前端的一层：
// - 上游：HTTP server 在返回 /api/snapshot、/api/cell 等结果时会调用它
// - 下游：只依赖 m2_workbook 读取 Workbook，不参与计算和存储
// 所以 m7 只负责“展示给前端的数据长什么样”
// ========================================

#include "minisheet/m2_workbook.h"

#include <string>

using namespace std;

// 把整个工作簿转换成JSON字符串
// 返回的JSON格式大概是这样：
// {
//   "maxRows": 32767,
//   "maxCols": 256,
//   "cells": {
//     "A1": {"id": "A1", "raw": "123", "display": "123", "type": "integer", "error": ""},
//     "B1": {"id": "B1", "raw": "=A1+1", "display": "124", "type": "formula", "error": ""}
//   }
// }
string workbook_snapshot_json(const Workbook& gongzuobu);
