#pragma once

#include "minisheet/m2_workbook.h"

#include <string>
#include <unordered_map>

namespace minisheet {

std::string workbook_snapshot_json(const Workbook& workbook);
void restore_workbook_from_browser_draft(Workbook& workbook, const std::unordered_map<std::string, std::string>& cells);

}  // namespace minisheet
