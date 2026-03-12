#include "minisheet/m7_api.h"

#include "json.hpp"

using namespace std;

const char* cell_kind_text(CellKind leixing) {
  switch (leixing) {
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

string workbook_snapshot_json(const Workbook& gongzuobu) {
  nlohmann::json danyuange_json = nlohmann::json::object();
  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);
    danyuange_json[danyuange_id] = {
        {"id", danyuange.biaoshi},
        {"raw", danyuange.yuanshi},
        {"display", danyuange.xianshi},
        {"type", cell_kind_text(danyuange.leixing)},
        {"error", danyuange.cuowu},
    };
  }

  nlohmann::json kuaizhao = {
      {"maxRows", kMaxRows},
      {"maxCols", kMaxColumns},
      {"cells", danyuange_json},
  };
  return kuaizhao.dump();
}
