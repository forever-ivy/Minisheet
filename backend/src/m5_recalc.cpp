#include "minisheet/m5_recalc.h"

#include "minisheet/m3_display.h"
#include "minisheet/m4_formula.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace minisheet {
namespace {

enum class EvalState {
  Unvisited,
  Visiting,
  Done,
};

std::optional<double> evaluate_cell_numeric(Workbook& workbook,
                                            const std::string& cell_id,
                                            std::unordered_map<std::string, EvalState>& states);

std::optional<double> evaluate_range_numeric(Workbook& workbook,
                                             const CellRange& range,
                                             bool average,
                                             std::unordered_map<std::string, EvalState>& states) {
  int min_row = std::min(range.start.row, range.end.row);
  int max_row = std::max(range.start.row, range.end.row);
  int min_column = std::min(range.start.column, range.end.column);
  int max_column = std::max(range.start.column, range.end.column);

  double total = 0.0;
  int count = 0;

  for (int row = min_row; row <= max_row; ++row) {
    for (int column = min_column; column <= max_column; ++column) {
      const std::string current_id = to_cell_id({row, column});
      bool exists = workbook.has_cell(current_id);
      const CellRecord& current = workbook.cell(current_id);

      if (!exists || current.kind == CellKind::Empty || current.kind == CellKind::String) {
        continue;
      }

      std::optional<double> value = evaluate_cell_numeric(workbook, current_id, states);
      if (!value.has_value()) {
        return std::nullopt;
      }

      total += value.value();
      count += 1;
    }
  }

  if (average) {
    if (count == 0) {
      return std::nullopt;
    }
    return total / static_cast<double>(count);
  }

  return total;
}

std::optional<double> evaluate_cell_numeric(Workbook& workbook,
                                            const std::string& cell_id,
                                            std::unordered_map<std::string, EvalState>& states) {
  if (!is_valid_cell_id(cell_id)) {
    return std::nullopt;
  }

  if (!workbook.has_cell(cell_id)) {
    return 0.0;
  }

  CellRecord& cell = workbook.mutable_cells().at(cell_id);
  switch (cell.kind) {
    case CellKind::Empty:
      return 0.0;
    case CellKind::Integer:
    case CellKind::Float:
      return cell.numeric_value;
    case CellKind::String:
      return std::nullopt;
    case CellKind::Formula:
      break;
  }

  EvalState state = states[cell_id];
  if (state == EvalState::Visiting) {
    cell.error = "#NA";
    cell.display = "#NA";
    cell.has_numeric_value = false;
    cell.numeric_value = 0.0;
    return std::nullopt;
  }

  if (state == EvalState::Done) {
    if (cell.has_numeric_value) {
      return cell.numeric_value;
    }
    return std::nullopt;
  }

  states[cell_id] = EvalState::Visiting;
  FormulaEvalResult eval_result = evaluate_formula(
      cell.raw,
      [&](const std::string& dependency_id) -> std::optional<double> {
        return evaluate_cell_numeric(workbook, dependency_id, states);
      },
      [&](const CellRange& range, bool average) -> std::optional<double> {
        return evaluate_range_numeric(workbook, range, average, states);
      });

  cell.precedents = eval_result.references;
  if (!eval_result.ok || !std::isfinite(eval_result.value)) {
    cell.error = "#NA";
    cell.display = "#NA";
    cell.has_numeric_value = false;
    cell.numeric_value = 0.0;
    states[cell_id] = EvalState::Done;
    return std::nullopt;
  }

  cell.error.clear();
  cell.has_numeric_value = true;
  cell.numeric_value = eval_result.value;
  cell.display = format_number(eval_result.value);
  states[cell_id] = EvalState::Done;
  return eval_result.value;
}

void refresh_all_literal_cells(Workbook& workbook) {
  for (auto& item : workbook.mutable_cells()) {
    refresh_literal_cell(item.second);
  }
}

void clear_dependencies(Workbook& workbook) {
  for (auto& item : workbook.mutable_cells()) {
    item.second.precedents.clear();
    item.second.dependents.clear();
  }
}

}  // namespace

void rebuild_dependencies(Workbook& workbook) {
  clear_dependencies(workbook);
  for (auto& item : workbook.mutable_cells()) {
    CellRecord& cell = item.second;
    if (cell.kind != CellKind::Formula) {
      continue;
    }

    cell.precedents = extract_formula_references(cell.raw);
    for (const std::string& dependency_id : cell.precedents) {
      auto dependency = workbook.mutable_cells().find(dependency_id);
      if (dependency != workbook.mutable_cells().end()) {
        dependency->second.dependents.insert(cell.id);
      }
    }
  }
}

void recalculate_all_cells(Workbook& workbook) {
  auto start = std::chrono::steady_clock::now();

  refresh_all_literal_cells(workbook);
  rebuild_dependencies(workbook);

  std::unordered_map<std::string, EvalState> states;
  for (const auto& item : workbook.cells()) {
    states[item.first] = EvalState::Unvisited;
  }

  for (const auto& item : workbook.cells()) {
    if (item.second.kind == CellKind::Formula) {
      (void)evaluate_cell_numeric(workbook, item.first, states);
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
