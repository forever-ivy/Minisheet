#pragma once

#include "minisheet/m1_types.h"

#include <functional>
#include <string>

namespace minisheet {

struct FormulaEvalResult {
  bool ok = false;
  double value = 0.0;
};

using CellResolver = std::function<bool(const std::string&, double&)>;
using RangeResolver = std::function<bool(const CellRange&, bool average, double&)>;

FormulaEvalResult evaluate_formula(const std::string& formula,
                                   const CellResolver& cell_resolver,
                                   const RangeResolver& range_resolver);

}  // namespace minisheet
