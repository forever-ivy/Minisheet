#pragma once

// ========================================
// API接口模块
// 提供把Workbook转换成JSON格式的功能
// 主要用于服务器给前端返回数据
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
