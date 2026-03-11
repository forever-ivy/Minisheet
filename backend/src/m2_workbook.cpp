#include "minisheet/m2_workbook.h"

#include "minisheet/m3_display.h"
#include "minisheet/m5_recalc.h"

#include <algorithm>

using namespace std;

namespace minisheet {
namespace {

string normalize_cell_id(const string& cell_id) {
  return to_cell_id(parse_cell_id(cell_id));
}

}  // namespace

void Workbook::clear() {
  cells_.clear();
  last_compute_ms_ = 0.0;
}

void Workbook::set_cell(const std::string& cell_id, const std::string& raw) {
  const string normalized_id = normalize_cell_id(cell_id);
  CellCoord coord = parse_cell_id(normalized_id);
  (void)coord;

  if (raw.empty()) {
    cells_.erase(normalized_id);
    return;
  }

  CellRecord& cell_record = cells_[normalized_id];
  cell_record.id = normalized_id;
  cell_record.raw = raw;
  refresh_literal_cell(cell_record);
}

const CellRecord& Workbook::cell(const std::string& cell_id) const {
  const string normalized_id = normalize_cell_id(cell_id);
  auto iterator = cells_.find(normalized_id);
  if (iterator == cells_.end()) {
    empty_cell_ = {};
    empty_cell_.id = normalized_id;
    empty_cell_.kind = CellKind::Empty;
    empty_cell_.display.clear();
    empty_cell_.raw.clear();
    empty_cell_.error.clear();
    empty_cell_.has_numeric_value = false;
    empty_cell_.numeric_value = 0.0;
    return empty_cell_;
  }
  return iterator->second;
}

bool Workbook::has_cell(const std::string& cell_id) const {
  const string normalized_id = normalize_cell_id(cell_id);
  return cells_.find(normalized_id) != cells_.end();
}

vector<string> Workbook::ordered_cell_ids() const {
  vector<string> ids;
  ids.reserve(cells_.size());
  for (const auto& item : cells_) {
    ids.push_back(item.first);
  }

  sort(ids.begin(), ids.end(), [](const string& left, const string& right) {
    CellCoord lhs = parse_cell_id(left);
    CellCoord rhs = parse_cell_id(right);
    if (lhs.row != rhs.row) {
      return lhs.row < rhs.row;
    }
    return lhs.column < rhs.column;
  });
  return ids;
}

const std::unordered_map<std::string, CellRecord>& Workbook::cells() const {
  return cells_;
}

std::unordered_map<std::string, CellRecord>& Workbook::mutable_cells() {
  return cells_;
}

void Workbook::recalculate_all() {
  recalculate_all_cells(*this);
}

void Workbook::recalculate_from(const std::string& cell_id) {
  recalculate_impacted_cells(*this, cell_id);
}

double Workbook::last_compute_ms() const {
  return last_compute_ms_;
}

void Workbook::set_last_compute_ms(double value) {
  last_compute_ms_ = value;
}

}  // namespace minisheet
