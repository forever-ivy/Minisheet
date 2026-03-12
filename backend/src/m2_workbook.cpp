#include "minisheet/m2_workbook.h"

#include "minisheet/m3_display.h"
#include "minisheet/m5_recalc.h"

#include <algorithm>

using namespace std;

bool compare_cell_id(const string& zuo_id, const string& you_id) {
  CellCoord zuo_zuobiao = parse_cell_id(zuo_id);
  CellCoord you_zuobiao = parse_cell_id(you_id);
  if (zuo_zuobiao.hang != you_zuobiao.hang) {
    return zuo_zuobiao.hang < you_zuobiao.hang;
  }
  return zuo_zuobiao.lie < you_zuobiao.lie;
}

void clear(Workbook& gongzuobu) {
  gongzuobu.danyuange_biao_.clear();
  gongzuobu.yuan_csv_hang_shu = 0;
  gongzuobu.yuan_csv_lie_shu = 0;
}

void set_cell(Workbook& gongzuobu, const string& danyuange_id, const string& yuanshi) {
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));
  if (yuanshi.empty()) {
    gongzuobu.danyuange_biao_.erase(guifanhou_id);
    return;
  }

  CellRecord& danyuange_jilu = gongzuobu.danyuange_biao_[guifanhou_id];
  danyuange_jilu.biaoshi = guifanhou_id;
  danyuange_jilu.yuanshi = yuanshi;
  refresh_literal_cell(danyuange_jilu);
}

const CellRecord& cell(const Workbook& gongzuobu, const string& danyuange_id) {
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));
  auto dieqi = gongzuobu.danyuange_biao_.find(guifanhou_id);
  if (dieqi != gongzuobu.danyuange_biao_.end()) {
    return dieqi->second;
  }

  gongzuobu.kong_danyuange_ = {};
  gongzuobu.kong_danyuange_.biaoshi = guifanhou_id;
  gongzuobu.kong_danyuange_.leixing = CellKind::Empty;
  return gongzuobu.kong_danyuange_;
}

bool has_cell(const Workbook& gongzuobu, const string& danyuange_id) {
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));
  return gongzuobu.danyuange_biao_.find(guifanhou_id) != gongzuobu.danyuange_biao_.end();
}

vector<string> ordered_cell_ids(const Workbook& gongzuobu) {
  vector<string> id_men;
  id_men.reserve(gongzuobu.danyuange_biao_.size());
  for (const auto& xiang : gongzuobu.danyuange_biao_) {
    id_men.push_back(xiang.first);
  }

  sort(id_men.begin(), id_men.end(), compare_cell_id);
  return id_men;
}

const unordered_map<string, CellRecord>& cells(const Workbook& gongzuobu) {
  return gongzuobu.danyuange_biao_;
}

unordered_map<string, CellRecord>& mutable_cells(Workbook& gongzuobu) {
  return gongzuobu.danyuange_biao_;
}

void recalculate_all(Workbook& gongzuobu) {
  recalculate_all_cells(gongzuobu);
}

void recalculate_from(Workbook& gongzuobu, const string& danyuange_id) {
  recalculate_impacted_cells(gongzuobu, danyuange_id);
}
