#pragma once

#include "minisheet/m2_workbook.h"

CellKind classify_raw_kind(const string& yuanshi, double& shuzhi, bool& shi_zhengshu);
void refresh_literal_cell(CellRecord& danyuange);
