#include "minisheet/m7_api.h"

#include "json.hpp"

namespace minisheet {
namespace {

std::string cell_kind_to_string(CellKind kind) {
  switch (kind) {
    case CellKind::Empty:
      return "empty";
    case CellKind::Integer:
      return "integer";
    case CellKind::Float:
      return "float";
    case CellKind::String:
      return "string";
    case CellKind::Formula:
      return "formula";
  }
  return "empty";
}

}  // namespace

std::string workbook_snapshot_json(const Workbook& workbook) {
  nlohmann::json cells = nlohmann::json::object();
  for (const std::string& id : workbook.ordered_cell_ids()) {
    const CellRecord& cell = workbook.cell(id);
    cells[id] = {
        {"id", cell.id},
        {"raw", cell.raw},
        {"display", cell.display},
        {"type", cell_kind_to_string(cell.kind)},
        {"error", cell.error},
    };
  }

  return nlohmann::json({
      {"maxRows", kMaxRows},
      {"maxCols", kMaxColumns},
      {"computeMs", workbook.last_compute_ms()},
      {"cells", cells},
  }).dump();
}

}  // namespace minisheet
