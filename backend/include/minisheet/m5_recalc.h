#pragma once

// ========================================
// 重新计算模块
// 当单元格内容改变时，需要重新计算相关的公式
// 它是 m2_workbook 和 m4_formula 之间的调度层：
// - 上游：m2::recalculate_all / recalculate_from
// - 下游：先调 m3 刷新普通单元格，再调 m4 递归计算公式
// 所以 m5 不保存数据，主要负责“按什么顺序重算”
// ========================================

#include "minisheet/m2_workbook.h"

// 重新计算所有单元格
// 遍历工作簿中所有单元格：
// 1. 先刷新所有普通单元格（数字、文字）的显示
// 2. 再计算所有公式单元格的值
// 这个函数在加载文件或批量修改后会用到
void recalculate_all_cells(Workbook& gongzuobu);

// 重新计算受影响的单元格
// 参数danyuange_id是刚刚被修改的单元格
// 应该重新计算所有依赖这个单元格的公式
// 注意：目前的简化实现是直接重算所有单元格
void recalculate_impacted_cells(Workbook& gongzuobu, const string& danyuange_id);
