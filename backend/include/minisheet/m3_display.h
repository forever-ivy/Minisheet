#pragma once

#include "minisheet/m2_workbook.h"

namespace minisheet {

CellKind classify_raw_kind(const std::string& raw, double& numeric_value, bool& is_integer);
void refresh_literal_cell(CellRecord& cell);

}  // namespace minisheet

