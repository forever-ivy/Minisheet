#pragma once

#include "minisheet/m1_types.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

namespace minisheet {

struct FormulaEvalResult {
  bool ok = false;
  double value = 0.0;
  std::unordered_set<std::string> references;
};

using CellResolver = std::function<std::optional<double>(const std::string&)>;
using RangeResolver = std::function<std::optional<double>(const CellRange&, bool average)>;

std::unordered_set<std::string> extract_formula_references(const std::string& formula);
FormulaEvalResult evaluate_formula(const std::string& formula,
                                   const CellResolver& cell_resolver,
                                   const RangeResolver& range_resolver);

}  // namespace minisheet

