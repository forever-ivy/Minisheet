#pragma once

#include "minisheet/m1_types.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct CellRecord {
  string biaoshi;
  string yuanshi;
  string xianshi;
  string cuowu;
  CellKind leixing = CellKind::Empty;
  bool you_shuzhi = false;
  double shuzhi = 0.0;
};

struct Workbook {
  unordered_map<string, CellRecord> danyuange_biao_;
  mutable CellRecord kong_danyuange_;
  int yuan_csv_hang_shu = 0;
  int yuan_csv_lie_shu = 0;
};

void clear(Workbook& gongzuobu);
void set_cell(Workbook& gongzuobu, const string& danyuange_id, const string& yuanshi);
const CellRecord& cell(const Workbook& gongzuobu, const string& danyuange_id);
bool has_cell(const Workbook& gongzuobu, const string& danyuange_id);
vector<string> ordered_cell_ids(const Workbook& gongzuobu);
const unordered_map<string, CellRecord>& cells(const Workbook& gongzuobu);
unordered_map<string, CellRecord>& mutable_cells(Workbook& gongzuobu);
void recalculate_all(Workbook& gongzuobu);
void recalculate_from(Workbook& gongzuobu, const string& danyuange_id);
