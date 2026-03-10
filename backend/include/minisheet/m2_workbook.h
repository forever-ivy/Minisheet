#pragma once

#include "minisheet/m1_types.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minisheet {

struct CellRecord {
  std::string id;
  std::string raw;
  std::string display;
  std::string error;
  CellKind kind = CellKind::Empty;
  bool has_numeric_value = false;
  double numeric_value = 0.0;
  std::unordered_set<std::string> precedents;
  std::unordered_set<std::string> dependents;
};

class Workbook {
 public:
  Workbook() = default;

  void clear();
  void set_cell(const std::string& cell_id, const std::string& raw);
  const CellRecord& cell(const std::string& cell_id) const;
  bool has_cell(const std::string& cell_id) const;
  std::vector<std::string> ordered_cell_ids() const;
  const std::unordered_map<std::string, CellRecord>& cells() const;
  std::unordered_map<std::string, CellRecord>& mutable_cells();
  void recalculate_all();
  void recalculate_from(const std::string& cell_id);
  double last_compute_ms() const;
  void set_last_compute_ms(double value);

 private:
  std::unordered_map<std::string, CellRecord> cells_;
  mutable CellRecord empty_cell_;
  double last_compute_ms_ = 0.0;
};

}  // namespace minisheet
