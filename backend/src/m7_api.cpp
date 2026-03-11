/**
 * @file m7_api.cpp
 * @brief API JSON 序列化实现
 *
 * 本文件实现：
 * - 工作簿快照的 JSON 格式转换
 * - 用于 HTTP API 返回的数据格式化
 */

#include "minisheet/m7_api.h"

#include "json.hpp"

using namespace std;

namespace minisheet {
namespace {

// ----------------------------------------------------------------------------
// 将 CellKind 枚举转换为字符串
// ----------------------------------------------------------------------------
string cell_kind_to_string(CellKind leixing) {
  switch (leixing) {
    case CellKind::Empty:
      return "empty";
    case CellKind::Integer:
      return "integer";
    case CellKind::Float:
      return "float";
    case CellKind::String:
      return "string";
    case CellKind::Formula:
      return "formula";
  }
  return "empty";
}

}  // namespace

// ----------------------------------------------------------------------------
// 生成工作簿的 JSON 快照
// 返回结构：
// {
//   "maxRows": 100000,
//   "maxCols": 10000,
//   "cells": {
//     "A1": {
//       "id": "A1",
//       "raw": "=B1+C1",
//       "display": "100",
//       "type": "formula",
//       "error": ""
//     },
//     ...
//   }
// }
// ----------------------------------------------------------------------------
string workbook_snapshot_json(const Workbook& gongzuobu) {
  nlohmann::json danyuange_json = nlohmann::json::object();
  for (const string& danyuange_id : gongzuobu.ordered_cell_ids()) {
    const CellRecord& danyuange = gongzuobu.cell(danyuange_id);
    danyuange_json[danyuange_id] = {
        {"id", danyuange.biaoshi},
        {"raw", danyuange.yuanshi},
        {"display", danyuange.xianshi},
        {"type", cell_kind_to_string(danyuange.leixing)},
        {"error", danyuange.cuowu},
    };
  }

  return nlohmann::json({
      {"maxRows", kMaxRows},
      {"maxCols", kMaxColumns},
      {"cells", danyuange_json},
  }).dump();
}

}  // namespace minisheet
