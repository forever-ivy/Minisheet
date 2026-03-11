/**
 * @file m5_recalc.cpp
 * @brief 公式重计算引擎
 *
 * 本文件实现：
 * - 递归求值公式单元格
 * - 范围计算（SUM/AVG）
 * - 循环依赖检测
 * - 全量重计算和增量重计算
 */

#include "minisheet/m5_recalc.h"

#include "minisheet/m3_display.h"
#include "minisheet/m4_formula.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace minisheet {
namespace {

// ----------------------------------------------------------------------------
// 检查 ID 是否在列表中
// ----------------------------------------------------------------------------
bool contains_id(const vector<string>& id_men, const string& danyuange_id) {
  return find(id_men.begin(), id_men.end(), danyuange_id) != id_men.end();
}

// ----------------------------------------------------------------------------
// 从列表中移除指定 ID
// ----------------------------------------------------------------------------
void forget_id(vector<string>& id_men, const string& danyuange_id) {
  id_men.erase(remove(id_men.begin(), id_men.end(), danyuange_id), id_men.end());
}

// ----------------------------------------------------------------------------
// 设置公式错误状态
// 当公式求值失败时调用
// ----------------------------------------------------------------------------
void set_formula_error(CellRecord& danyuange) {
  danyuange.cuowu = "#NA";
  danyuange.xianshi = "#NA";
  danyuange.you_shuzhi = false;
  danyuange.shuzhi = 0.0;
}

// 前向声明：求值单元格数值
bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,
                           vector<string>& yiwancheng,
                           double& shuzhi);

// ----------------------------------------------------------------------------
// 求值范围内的数值（用于 SUM/AVG 函数）
// 遍历范围内的所有单元格，累加数值型单元格的值
// 参数：
//   shi_pingjun - true 表示计算平均值，false 表示计算总和
// ----------------------------------------------------------------------------
bool evaluate_range_numeric(Workbook& gongzuobu,
                            const CellRange& fanwei,
                            bool shi_pingjun,
                            vector<string>& fangwen_zhong,
                            vector<string>& yiwancheng,
                            double& shuzhi) {
  int zuixiao_hang = min(fanwei.qishi.hang, fanwei.jieshu.hang);
  int zuida_hang = max(fanwei.qishi.hang, fanwei.jieshu.hang);
  int zuixiao_lie = min(fanwei.qishi.lie, fanwei.jieshu.lie);
  int zuida_lie = max(fanwei.qishi.lie, fanwei.jieshu.lie);

  double zonghe = 0.0;
  int shuliang = 0;

  for (int hang = zuixiao_hang; hang <= zuida_hang; ++hang) {
    for (int lie = zuixiao_lie; lie <= zuida_lie; ++lie) {
      const string dangqian_id = to_cell_id({hang, lie});
      if (!gongzuobu.has_cell(dangqian_id)) {
        continue;
      }

      const CellRecord& dangqian_danyuange = gongzuobu.cell(dangqian_id);

      // 跳过空单元格和字符串单元格
      if (dangqian_danyuange.leixing == CellKind::Empty ||
          dangqian_danyuange.leixing == CellKind::String) {
        continue;
      }

      double danyuange_shuzhi = 0.0;
      if (!evaluate_cell_numeric(gongzuobu, dangqian_id, fangwen_zhong, yiwancheng,
                                 danyuange_shuzhi)) {
        return false;
      }

      zonghe += danyuange_shuzhi;
      shuliang += 1;
    }
  }

  if (shi_pingjun) {
    if (shuliang == 0) {
      return false;
    }
    shuzhi = zonghe / static_cast<double>(shuliang);
    return true;
  }

  shuzhi = zonghe;
  return true;
}

// ----------------------------------------------------------------------------
// 递归求值单元格的数值
// 处理四种情况：
// - Empty：返回 0
// - Integer/Float：直接返回值
// - String：返回失败
// - Formula：递归求值公式
//
// 循环依赖检测：
// - fangwen_zhong：当前正在访问的单元格（用于检测循环）
// - yiwancheng：已经计算完成的单元格（用于缓存）
// ----------------------------------------------------------------------------
bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,
                           vector<string>& yiwancheng,
                           double& shuzhi) {
  if (!is_valid_cell_id(danyuange_id)) {
    return false;
  }

  if (!gongzuobu.has_cell(danyuange_id)) {
    shuzhi = 0.0;
    return true;
  }

  CellRecord& danyuange = gongzuobu.mutable_cells().at(danyuange_id);
  switch (danyuange.leixing) {
    case CellKind::Empty:
      shuzhi = 0.0;
      return true;
    case CellKind::Integer:
    case CellKind::Float:
      shuzhi = danyuange.shuzhi;
      return true;
    case CellKind::String:
      return false;
    case CellKind::Formula:
      break;
  }

  // 已缓存结果，直接返回
  if (contains_id(yiwancheng, danyuange_id)) {
    if (!danyuange.you_shuzhi) {
      return false;
    }
    shuzhi = danyuange.shuzhi;
    return true;
  }

  // 检测到循环依赖
  if (contains_id(fangwen_zhong, danyuange_id)) {
    set_formula_error(danyuange);
    return false;
  }

  // 开始求值此公式单元格
  fangwen_zhong.push_back(danyuange_id);
  FormulaEvalResult jisuan_jieguo = evaluate_formula(
      danyuange.yuanshi,
      [&](const string& yilai_id, double& yilai_shuzhi) -> bool {
        return evaluate_cell_numeric(gongzuobu, yilai_id, fangwen_zhong, yiwancheng, yilai_shuzhi);
      },
      [&](const CellRange& fanwei, bool shi_pingjun, double& fanwei_shuzhi) -> bool {
        return evaluate_range_numeric(gongzuobu, fanwei, shi_pingjun, fangwen_zhong, yiwancheng,
                                      fanwei_shuzhi);
      });

  // 求值失败
  if (!jisuan_jieguo.chenggong || !std::isfinite(jisuan_jieguo.shuzhi)) {
    set_formula_error(danyuange);
    forget_id(fangwen_zhong, danyuange_id);
    yiwancheng.push_back(danyuange_id);
    return false;
  }

  // 求值成功，保存结果
  danyuange.cuowu.clear();
  danyuange.you_shuzhi = true;
  danyuange.shuzhi = jisuan_jieguo.shuzhi;
  danyuange.xianshi = format_number(jisuan_jieguo.shuzhi);
  shuzhi = jisuan_jieguo.shuzhi;
  forget_id(fangwen_zhong, danyuange_id);
  yiwancheng.push_back(danyuange_id);
  return true;
}

// ----------------------------------------------------------------------------
// 刷新所有字面量单元格的类型和显示
// 在公式求值前调用，确保非公式单元格的状态正确
// ----------------------------------------------------------------------------
void refresh_all_literal_cells(Workbook& gongzuobu) {
  for (auto& xiang : gongzuobu.mutable_cells()) {
    refresh_literal_cell(xiang.second);
  }
}

}  // namespace

// ----------------------------------------------------------------------------
// 重新计算所有公式单元格
// 流程：
// 1. 刷新所有字面量单元格
// 2. 遍历所有公式单元格并递归求值
// ----------------------------------------------------------------------------
void recalculate_all_cells(Workbook& gongzuobu) {
  refresh_all_literal_cells(gongzuobu);
  vector<string> fangwen_zhong;
  vector<string> yiwancheng;

  for (const auto& xiang : gongzuobu.cells()) {
    if (xiang.second.leixing == CellKind::Formula) {
      double shuzhi = 0.0;
      (void)evaluate_cell_numeric(gongzuobu, xiang.first, fangwen_zhong, yiwancheng, shuzhi);
    }
  }
}

// ----------------------------------------------------------------------------
// 增量重计算（从指定单元格开始）
// 当前实现为全量重计算，后续可优化为只计算受影响的单元格
// ----------------------------------------------------------------------------
void recalculate_impacted_cells(Workbook& gongzuobu, const std::string& danyuange_id) {
  (void)danyuange_id;
  recalculate_all_cells(gongzuobu);
}

}  // namespace minisheet
