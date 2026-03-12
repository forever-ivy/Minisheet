#include "minisheet/m6_storage.h"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

void print_usage() {
  cout << "Usage:\n"
       << "  minisheet_cli import <input.csv>\n"
       << "  minisheet_cli save <input.csv> <output.dat>\n"
       << "  minisheet_cli load <input.dat>\n"
       << "  minisheet_cli pack <input.csv> <output.dat>\n"
       << "  minisheet_cli unpack <input.dat> <output.csv>\n";
}

void print_workbook(const Workbook& gongzuobu) {
  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);
    cout << danyuange_id << " raw=" << danyuange.yuanshi << " display=" << danyuange.xianshi
         << '\n';
  }
}

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      print_usage();
      return 1;
    }

    string mingling = argv[1];
    if (mingling == "import") {
      print_workbook(load_csv(argv[2]));
      return 0;
    }
    if (mingling == "save" && argc >= 4) {
      Workbook gongzuobu = load_csv(argv[2]);
      save_dat(argv[3], gongzuobu);
      print_workbook(gongzuobu);
      return 0;
    }
    if (mingling == "pack" && argc >= 4) {
      Workbook gongzuobu = load_csv(argv[2]);
      save_dat(argv[3], gongzuobu);
      cout << "packed " << argv[2] << " -> " << argv[3] << '\n';
      return 0;
    }
    if (mingling == "load") {
      print_workbook(load_dat(argv[2]));
      return 0;
    }
    if (mingling == "unpack" && argc >= 4) {
      Workbook gongzuobu = load_dat(argv[2]);
      save_csv(argv[3], gongzuobu);
      cout << "unpacked " << argv[2] << " -> " << argv[3] << '\n';
      return 0;
    }

    print_usage();
    return 1;
  } catch (const exception& cuowu) {
    cerr << "minisheet_cli error: " << cuowu.what() << '\n';
    return 1;
  }
}
