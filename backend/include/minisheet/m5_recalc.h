#pragma once

#include "minisheet/m2_workbook.h"

namespace minisheet {

void recalculate_all_cells(Workbook& workbook);
void recalculate_impacted_cells(Workbook& workbook, const std::string& cell_id);

}  // namespace minisheet
