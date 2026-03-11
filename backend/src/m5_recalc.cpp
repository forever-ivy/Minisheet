#include "minisheet/m5_recalc.h"

#include "minisheet/m3_display.h"
#include "minisheet/m4_formula.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

using namespace std;

namespace minisheet {
namespace {

bool contains_id(const vector<string>& ids, const string& id) {
  return find(ids.begin(), ids.end(), id) != ids.end();
}

void forget_id(vector<string>& ids, const string& id) {
  ids.erase(remove(ids.begin(), ids.end(), id), ids.end());
}

void set_formula_error(CellRecord& cell) {
  cell.error = "#NA";
  cell.display = "#NA";
  cell.has_numeric_value = false;
  cell.numeric_value = 0.0;
}

bool evaluate_cell_numeric(Workbook& workbook,
                           const string& cell_id,
                           vector<string>& visiting,
                           vector<string>& finished,
                           double& value);

bool evaluate_range_numeric(Workbook& workbook,
                            const CellRange& range,
                            bool average,
                            vector<string>& visiting,
                            vector<string>& finished,
                            double& value) {
  int min_row = min(range.start.row, range.end.row);
  int max_row = max(range.start.row, range.end.row);
  int min_column = min(range.start.column, range.end.column);
  int max_column = max(range.start.column, range.end.column);

  double total = 0.0;
  int count = 0;

  for (int row = min_row; row <= max_row; ++row) {
    for (int column = min_column; column <= max_column; ++column) {
      const string current_id = to_cell_id({row, column});
      if (!workbook.has_cell(current_id)) {
        continue;
      }

      const CellRecord& current = workbook.cell(current_id);

      if (current.kind == CellKind::Empty || current.kind == CellKind::String) {
        continue;
      }

      double cell_value = 0.0;
      if (!evaluate_cell_numeric(workbook, current_id, visiting, finished, cell_value)) {
        return false;
      }

      total += cell_value;
      count += 1;
    }
  }

  if (average) {
    if (count == 0) {
      return false;
    }
    value = total / static_cast<double>(count);
    return true;
  }

  value = total;
  return true;
}

bool evaluate_cell_numeric(Workbook& workbook,
                           const string& cell_id,
                           vector<string>& visiting,
                           vector<string>& finished,
                           double& value) {
  if (!is_valid_cell_id(cell_id)) {
    return false;
  }

  if (!workbook.has_cell(cell_id)) {
    value = 0.0;
    return true;
  }

  CellRecord& cell = workbook.mutable_cells().at(cell_id);
  switch (cell.kind) {
    case CellKind::Empty:
      value = 0.0;
      return true;
    case CellKind::Integer:
    case CellKind::Float:
      value = cell.numeric_value;
      return true;
    case CellKind::String:
      return false;
    case CellKind::Formula:
      break;
  }

  if (contains_id(finished, cell_id)) {
    if (!cell.has_numeric_value) {
      return false;
    }
    value = cell.numeric_value;
    return true;
  }

  if (contains_id(visiting, cell_id)) {
    set_formula_error(cell);
    return false;
  }

  visiting.push_back(cell_id);
  FormulaEvalResult eval_result = evaluate_formula(
      cell.raw,
      [&](const string& dependency_id, double& dependency_value) -> bool {
        return evaluate_cell_numeric(workbook, dependency_id, visiting, finished, dependency_value);
      },
      [&](const CellRange& range, bool average, double& range_value) -> bool {
        return evaluate_range_numeric(workbook, range, average, visiting, finished, range_value);
      });

  if (!eval_result.ok || !std::isfinite(eval_result.value)) {
    set_formula_error(cell);
    forget_id(visiting, cell_id);
    finished.push_back(cell_id);
    return false;
  }

  cell.error.clear();
  cell.has_numeric_value = true;
  cell.numeric_value = eval_result.value;
  cell.display = format_number(eval_result.value);
  value = eval_result.value;
  forget_id(visiting, cell_id);
  finished.push_back(cell_id);
  return true;
}

void refresh_all_literal_cells(Workbook& workbook) {
  for (auto& item : workbook.mutable_cells()) {
    refresh_literal_cell(item.second);
  }
}

}  // namespace

void recalculate_all_cells(Workbook& workbook) {
  auto start = std::chrono::steady_clock::now();

  refresh_all_literal_cells(workbook);
  vector<string> visiting;
  vector<string> finished;

  for (const auto& item : workbook.cells()) {
    if (item.second.kind == CellKind::Formula) {
      double numeric_value = 0.0;
      (void)evaluate_cell_numeric(workbook, item.first, visiting, finished, numeric_value);
    }
  }

  auto end = std::chrono::steady_clock::now();
  double elapsed_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count();
  workbook.set_last_compute_ms(elapsed_ms);
}

void recalculate_impacted_cells(Workbook& workbook, const std::string& cell_id) {
  (void)cell_id;
  recalculate_all_cells(workbook);
}

}  // namespace minisheet
