// ========================================
// API接口实现文件
// 把Workbook转换成JSON格式，供服务器返回给前端
// ========================================

#include "minisheet/m7_api.h"

#include "json.hpp"  // 第三方JSON库，nlohmann/json

using namespace std;

// 把CellKind枚举转成字符串
// 前端需要字符串类型的type字段
const char* cell_kind_text(CellKind leixing) {
  switch (leixing) {
    case CellKind::Empty:
      return "empty";     // 空单元格
    case CellKind::Integer:
      return "integer";   // 整数
    case CellKind::Float:
      return "float";     // 浮点数（小数）
    case CellKind::String:
      return "string";    // 字符串
    case CellKind::Formula:
      return "formula";   // 公式
  }
  return "empty";  // 默认值，理论上不会走到这里
}

// 生成工作簿的JSON快照
// 这个函数会被服务器调用来返回当前状态给前端
string workbook_snapshot_json(const Workbook& gongzuobu) {
  // 创建一个JSON对象存所有单元格
  nlohmann::json danyuange_json = nlohmann::json::object();

  // 遍历所有单元格（已经按顺序排好了）
  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);

    // 每个单元格是一个对象，包含以下字段：
    // id: 单元格ID，如"A1"
    // raw: 原始输入内容，如"=A1+1"或"123"
    // display: 显示内容，如"124"
    // type: 类型字符串，如"formula"
    // error: 错误信息，没有就为空字符串
    danyuange_json[danyuange_id] = {
        {"id", danyuange.biaoshi},
        {"raw", danyuange.yuanshi},
        {"display", danyuange.xianshi},
        {"type", cell_kind_text(danyuange.leixing)},
        {"error", danyuange.cuowu},
    };
  }

  // 组装最终的JSON结构
  nlohmann::json kuaizhao = {
      {"maxRows", kMaxRows},      // 最大行数
      {"maxCols", kMaxColumns},   // 最大列数
      {"cells", danyuange_json},  // 所有单元格
  };

  // 转成字符串返回
  return kuaizhao.dump();
}
