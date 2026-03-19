// ========================================
// 重新计算实现文件
// 当单元格内容改变时，需要重新计算公式
// 这个文件的职责不是“解析公式”，而是“安排重算顺序”：
// 1. 先把所有普通单元格的显示和数值状态刷干净
// 2. 再让 m4 从每个公式单元格出发递归计算依赖
// ========================================

#include "minisheet/m5_recalc.h"

#include "minisheet/m3_display.h"  // 需要刷新普通单元格
#include "minisheet/m4_formula.h"  // 需要计算公式

#include <vector>

using namespace std;

// 重新计算所有单元格
// 调用时机：加载文件后、批量修改后
void recalculate_all_cells(Workbook& gongzuobu) {
  // 第一步：先刷新所有普通单元格
  // 普通单元格就是非公式类型的（空、数字、字符串）
  // 这些只需要根据原始内容重新确定显示值
  for (auto& xiang : mutable_cells(gongzuobu)) {
    refresh_literal_cell(xiang.second);
  }

  // 第二步：计算所有公式单元格
  // 这里自己不解析公式，而是逐个把入口交给 m4::evaluate_cell_numeric。
  // m4 内部会继续递归访问被引用的单元格。
  vector<string> yiwancheng;    // 已完成的单元格列表

  for (const auto& xiang : cells(gongzuobu)) {
    if (xiang.second.leixing == CellKind::Formula) {
      double shuzhi = 0.0;
      // 从“这个公式单元格”出发向下展开依赖图。
      // 如果它依赖 A1/B1，m4 会继续递归进去；m5 只负责把这个入口触发起来。
      (void)evaluate_cell_numeric(gongzuobu, xiang.first, yiwancheng, shuzhi);
      // 注意：这里即使计算失败也不中断，继续算其他的
      // 失败的单元格会在evaluate_cell_numeric里被标记为#NA
    }
  }
}

// 重新计算受影响的单元格
// 理想情况下，只应该重算依赖danyuange_id的单元格
// 但目前采用简单实现：重算所有单元格
void recalculate_impacted_cells(Workbook& gongzuobu, const string& danyuange_id) {
  // 参数暂时没用，但保留用于以后优化
  (void)danyuange_id;

  // 目前就是重算所有
  // 以后可以优化成只算依赖这个单元格的公式
  recalculate_all_cells(gongzuobu);
}
