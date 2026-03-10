#include "minisheet/m6_storage.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_usage() {
  std::cout << "Usage:\n"
            << "  minisheet_cli import <input.csv>\n"
            << "  minisheet_cli save <input.csv> <output.dat>\n"
            << "  minisheet_cli load <input.dat>\n"
            << "  minisheet_cli benchmark <case1.csv> <case2.csv> <case3.csv>\n";
}

void print_workbook(const minisheet::Workbook& workbook) {
  for (const std::string& id : workbook.ordered_cell_ids()) {
    const minisheet::CellRecord& cell = workbook.cell(id);
    std::cout << id << " raw=" << cell.raw << " display=" << cell.display << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      print_usage();
      return 1;
    }

    std::string command = argv[1];
    if (command == "import") {
      minisheet::Workbook workbook = minisheet::load_csv(argv[2]);
      print_workbook(workbook);
      std::cout << "computeMs=" << workbook.last_compute_ms() << '\n';
      return 0;
    }

    if (command == "save" && argc >= 4) {
      minisheet::Workbook workbook = minisheet::load_csv(argv[2]);
      minisheet::save_dat(argv[3], workbook);
      print_workbook(workbook);
      return 0;
    }

    if (command == "load") {
      minisheet::Workbook workbook = minisheet::load_dat(argv[2]);
      print_workbook(workbook);
      return 0;
    }

    if (command == "benchmark" && argc >= 5) {
      std::vector<std::string> paths = {argv[2], argv[3], argv[4]};
      minisheet::BenchmarkResult result = minisheet::run_benchmark(paths);
      std::cout << "averageMs=" << result.average_ms << '\n';
      std::cout << "storageEfficiencyPct=" << result.storage_efficiency_pct << '\n';
      for (const auto& item : result.cases) {
        std::cout << item.input_path << " elapsedMs=" << item.elapsed_ms << " csvBytes=" << item.csv_size_bytes
                  << " datBytes=" << item.dat_size_bytes << '\n';
      }
      return 0;
    }

    print_usage();
    return 1;
  } catch (const std::exception& error) {
    std::cerr << "minisheet_cli error: " << error.what() << '\n';
    return 1;
  }
}
