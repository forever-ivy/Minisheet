#pragma once

#include "minisheet/m2_workbook.h"

#include <string>

namespace minisheet {

std::string workbook_snapshot_json(const Workbook& workbook);

}  // namespace minisheet

