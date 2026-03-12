// ========================================
// 重新计算实现文件
// 当单元格内容改变时，需要重新计算公式
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
  // 公式单元格需要递归计算，可能涉及依赖的其他单元格
  vector<string> fangwen_zhong;  // 访问栈，用于循环引用检测
  vector<string> yiwancheng;    // 已完成的单元格列表

  for (const auto& xiang : cells(gongzuobu)) {
    if (xiang.second.leixing == CellKind::Formula) {
      double shuzhi = 0.0;
      // 调用公式计算函数，第三个参数false表示计算总和不是平均值
      (void)evaluate_cell_numeric(gongzuobu, xiang.first, fangwen_zhong, yiwancheng, shuzhi);
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
