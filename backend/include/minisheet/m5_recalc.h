#pragma once

#include "minisheet/m2_workbook.h"

void recalculate_all_cells(Workbook& gongzuobu);
void recalculate_impacted_cells(Workbook& gongzuobu, const string& danyuange_id);
