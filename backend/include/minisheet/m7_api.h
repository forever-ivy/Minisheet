#pragma once

/**
 * @file m7_api.h
 * @brief HTTP API JSON 序列化模块
 *
 * 本文件提供工作簿到 JSON 格式的转换功能，
 * 用于 HTTP API 的数据交换。
 */

#include "minisheet/m2_workbook.h"

#include <string>

namespace minisheet {

/**
 * @brief 生成工作簿的 JSON 快照
 *
 * 返回包含完整工作簿信息的 JSON 字符串：
 * {
 *   "maxRows": 100000,
 *   "maxCols": 10000,
 *   "cells": {
 *     "A1": {
 *       "id": "A1",
 *       "raw": "=B1+C1",
 *       "display": "100",
 *       "type": "formula",
 *       "error": ""
 *     },
 *     ...
 *   }
 * }
 *
 * @param gongzuobu 要序列化的工作簿
 * @return JSON 字符串
 */
std::string workbook_snapshot_json(const Workbook& gongzuobu);

}  // namespace minisheet
