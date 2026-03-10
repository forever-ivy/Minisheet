#include "minisheet/m3_display.h"

namespace minisheet {

CellKind classify_raw_kind(const std::string& raw, double& numeric_value, bool& is_integer) {
  if (raw.empty()) {
    numeric_value = 0.0;
    is_integer = false;
    return CellKind::Empty;
  }

  if (!raw.empty() && raw.front() == '=') {
    numeric_value = 0.0;
    is_integer = false;
    return CellKind::Formula;
  }

  if (try_parse_number(raw, numeric_value, is_integer)) {
    return is_integer ? CellKind::Integer : CellKind::Float;
  }

  numeric_value = 0.0;
  is_integer = false;
  return CellKind::String;
}

void refresh_literal_cell(CellRecord& cell) {
  cell.error.clear();
  cell.has_numeric_value = false;
  cell.numeric_value = 0.0;

  double numeric_value = 0.0;
  bool is_integer = false;
  cell.kind = classify_raw_kind(cell.raw, numeric_value, is_integer);

  switch (cell.kind) {
    case CellKind::Empty:
      cell.display.clear();
      break;
    case CellKind::Integer:
    case CellKind::Float:
      cell.has_numeric_value = true;
      cell.numeric_value = numeric_value;
      cell.display = format_number(numeric_value);
      break;
    case CellKind::String:
      cell.display = cell.raw;
      break;
    case CellKind::Formula:
      cell.display.clear();
      break;
  }
}

}  // namespace minisheet
