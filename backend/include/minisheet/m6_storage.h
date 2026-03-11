#pragma once

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

namespace minisheet {

Workbook load_csv(const std::string& path);
std::vector<char> serialize_workbook(const Workbook& workbook);
Workbook deserialize_workbook(const std::vector<char>& bytes);
void save_dat(const std::string& path, const Workbook& workbook);
Workbook load_dat(const std::string& path);

}  // namespace minisheet
