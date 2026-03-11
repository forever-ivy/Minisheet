/**
 * @file m2_workbook.cpp
 * @brief 工作簿核心类实现
 *
 * 本文件实现 Workbook 类，负责：
 * - 单元格数据的存储和管理
 * - 单元格内容的增删改查
 * - 排序后的单元格列表获取
 * - 触发重计算
 */

#include "minisheet/m2_workbook.h"

#include "minisheet/m3_display.h"
#include "minisheet/m5_recalc.h"

#include <algorithm>

using namespace std;

namespace minisheet {
namespace {

// ----------------------------------------------------------------------------
// 将单元格ID规范化（解析后再转回，确保格式统一）
// 例如："a1" -> "A1", "  A1  " -> "A1"
// ----------------------------------------------------------------------------
string normalize_cell_id(const string& danyuange_id) {
  return to_cell_id(parse_cell_id(danyuange_id));
}

}  // namespace

// ----------------------------------------------------------------------------
// 清空工作簿，删除所有单元格数据
// ----------------------------------------------------------------------------
void Workbook::clear() {
  danyuange_biao_.clear();
}

// ----------------------------------------------------------------------------
// 设置单元格内容
// 如果 raw 为空字符串，则删除该单元格
// 否则更新或创建单元格，并刷新其字面量显示
// ----------------------------------------------------------------------------
void Workbook::set_cell(const std::string& danyuange_id, const std::string& yuanshi) {
  const string guifanhou_id = normalize_cell_id(danyuange_id);
  CellCoord zuobiao = parse_cell_id(guifanhou_id);
  (void)zuobiao;

  // 空内容则删除单元格
  if (yuanshi.empty()) {
    danyuange_biao_.erase(guifanhou_id);
    return;
  }

  // 更新或创建单元格记录
  CellRecord& danyuange_jilu = danyuange_biao_[guifanhou_id];
  danyuange_jilu.biaoshi = guifanhou_id;
  danyuange_jilu.yuanshi = yuanshi;
  refresh_literal_cell(danyuange_jilu);
}

// ----------------------------------------------------------------------------
// 获取单元格记录（只读）
// 如果单元格不存在，返回一个空的临时 CellRecord
// ----------------------------------------------------------------------------
const CellRecord& Workbook::cell(const std::string& danyuange_id) const {
  const string guifanhou_id = normalize_cell_id(danyuange_id);
  auto dieqi = danyuange_biao_.find(guifanhou_id);
  if (dieqi == danyuange_biao_.end()) {
    // 返回空单元格（mutable 允许在 const 方法中修改）
    kong_danyuange_ = {};
    kong_danyuange_.biaoshi = guifanhou_id;
    kong_danyuange_.leixing = CellKind::Empty;
    kong_danyuange_.xianshi.clear();
    kong_danyuange_.yuanshi.clear();
    kong_danyuange_.cuowu.clear();
    kong_danyuange_.you_shuzhi = false;
    kong_danyuange_.shuzhi = 0.0;
    return kong_danyuange_;
  }
  return dieqi->second;
}

// ----------------------------------------------------------------------------
// 检查指定单元格是否存在
// ----------------------------------------------------------------------------
bool Workbook::has_cell(const std::string& danyuange_id) const {
  const string guifanhou_id = normalize_cell_id(danyuange_id);
  return danyuange_biao_.find(guifanhou_id) != danyuange_biao_.end();
}

// ----------------------------------------------------------------------------
// 获取按行列排序的所有单元格ID列表
// 排序规则：先按行号升序，同行再按列号升序
// ----------------------------------------------------------------------------
vector<string> Workbook::ordered_cell_ids() const {
  vector<string> id_men;
  id_men.reserve(danyuange_biao_.size());
  for (const auto& xiang : danyuange_biao_) {
    id_men.push_back(xiang.first);
  }

  sort(id_men.begin(), id_men.end(), [](const string& zuo_id, const string& you_id) {
    CellCoord zuo_zuobiao = parse_cell_id(zuo_id);
    CellCoord you_zuobiao = parse_cell_id(you_id);
    if (zuo_zuobiao.hang != you_zuobiao.hang) {
      return zuo_zuobiao.hang < you_zuobiao.hang;
    }
    return zuo_zuobiao.lie < you_zuobiao.lie;
  });
  return id_men;
}

// ----------------------------------------------------------------------------
// 获取所有单元格的只读引用
// ----------------------------------------------------------------------------
const std::unordered_map<std::string, CellRecord>& Workbook::cells() const {
  return danyuange_biao_;
}

// ----------------------------------------------------------------------------
// 获取所有单元格的可写引用（供内部重计算使用）
// ----------------------------------------------------------------------------
std::unordered_map<std::string, CellRecord>& Workbook::mutable_cells() {
  return danyuange_biao_;
}

// ----------------------------------------------------------------------------
// 重新计算所有公式单元格
// ----------------------------------------------------------------------------
void Workbook::recalculate_all() {
  recalculate_all_cells(*this);
}

// ----------------------------------------------------------------------------
// 从指定单元格开始重新计算（目前实现为全量重算）
// ----------------------------------------------------------------------------
void Workbook::recalculate_from(const std::string& danyuange_id) {
  recalculate_impacted_cells(*this, danyuange_id);
}

}  // namespace minisheet
