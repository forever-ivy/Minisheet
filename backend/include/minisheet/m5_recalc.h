#pragma once

/**
 * @file m5_recalc.h
 * @brief 公式重计算引擎接口
 *
 * 本文件提供公式重计算的入口函数。
 * 重计算引擎支持：
 * - 递归求值公式单元格
 * - 循环依赖检测
 * - 计算结果缓存
 */

#include "minisheet/m2_workbook.h"

namespace minisheet {

/**
 * @brief 重新计算所有公式单元格
 *
 * 遍历工作簿中的所有公式单元格，递归求值并更新显示。
 * 自动处理单元格之间的依赖关系。
 *
 * @param gongzuobu 要重计算的工作簿
 */
void recalculate_all_cells(Workbook& gongzuobu);

/**
 * @brief 从指定单元格开始重计算
 *
 * 当前实现为全量重计算，后续可优化为只计算受影响的单元格。
 *
 * @param gongzuobu 要重计算的工作簿
 * @param danyuange_id 起始单元格ID（用于确定影响范围）
 */
void recalculate_impacted_cells(Workbook& gongzuobu, const std::string& danyuange_id);

}  // namespace minisheet
