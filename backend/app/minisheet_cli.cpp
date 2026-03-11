#include "minisheet/m6_storage.h"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

namespace {

void print_usage() {
  cout << "Usage:\n"
       << "  minisheet_cli import <input.csv>\n"
       << "  minisheet_cli save <input.csv> <output.dat>\n"
       << "  minisheet_cli load <input.dat>\n";
}

void print_workbook(const minisheet::Workbook& workbook) {
  for (const string& id : workbook.ordered_cell_ids()) {
    const minisheet::CellRecord& cell = workbook.cell(id);
    cout << id << " raw=" << cell.raw << " display=" << cell.display << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      print_usage();
      return 1;
    }

    string command = argv[1];
    if (command == "import") {
      minisheet::Workbook workbook = minisheet::load_csv(argv[2]);
      print_workbook(workbook);
      cout << "computeMs=" << workbook.last_compute_ms() << '\n';
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

    print_usage();
    return 1;
  } catch (const std::exception& error) {
    cerr << "minisheet_cli error: " << error.what() << '\n';
    return 1;
  }
}
