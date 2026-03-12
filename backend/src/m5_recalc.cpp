#include "minisheet/m5_recalc.h"

#include "minisheet/m3_display.h"
#include "minisheet/m4_formula.h"

#include <vector>

using namespace std;

void recalculate_all_cells(Workbook& gongzuobu) {
  for (auto& xiang : mutable_cells(gongzuobu)) {
    refresh_literal_cell(xiang.second);
  }

  vector<string> fangwen_zhong;
  vector<string> yiwancheng;
  for (const auto& xiang : cells(gongzuobu)) {
    if (xiang.second.leixing == CellKind::Formula) {
      double shuzhi = 0.0;
      (void)evaluate_cell_numeric(gongzuobu, xiang.first, fangwen_zhong, yiwancheng, shuzhi);
    }
  }
}

void recalculate_impacted_cells(Workbook& gongzuobu, const string& danyuange_id) {
  (void)danyuange_id;
  recalculate_all_cells(gongzuobu);
}
