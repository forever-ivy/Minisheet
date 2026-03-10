#pragma once

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

namespace minisheet {

struct BenchmarkCaseResult {
  std::string input_path;
  double elapsed_ms = 0.0;
  double csv_size_bytes = 0.0;
  double dat_size_bytes = 0.0;
};

struct BenchmarkResult {
  double average_ms = 0.0;
  double storage_efficiency_pct = 0.0;
  std::vector<BenchmarkCaseResult> cases;
};

Workbook load_csv(const std::string& path);
std::vector<char> serialize_workbook(const Workbook& workbook);
Workbook deserialize_workbook(const std::vector<char>& bytes);
void save_dat(const std::string& path, const Workbook& workbook);
Workbook load_dat(const std::string& path);
BenchmarkResult run_benchmark(const std::vector<std::string>& csv_paths);

}  // namespace minisheet

