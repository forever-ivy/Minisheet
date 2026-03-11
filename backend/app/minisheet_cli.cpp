/**
 * @file minisheet_cli.cpp
 * @brief 命令行工具入口
 *
 * 提供命令行操作电子表格的功能：
 * - import: 从 CSV 文件导入并显示
 * - save:   从 CSV 导入并保存为 DAT 格式
 * - load:   加载 DAT 文件并显示
 */

#include "minisheet/m6_storage.h"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

namespace {

// ----------------------------------------------------------------------------
// 打印使用说明
// ----------------------------------------------------------------------------
void print_usage() {
  cout << "Usage:\n"
       << "  minisheet_cli import <input.csv>\n"
       << "  minisheet_cli save <input.csv> <output.dat>\n"
       << "  minisheet_cli load <input.dat>\n";
}

// ----------------------------------------------------------------------------
// 打印工作簿内容
// 按顺序显示所有单元格的 ID、原始值和显示值
// ----------------------------------------------------------------------------
void print_workbook(const minisheet::Workbook& gongzuobu) {
  for (const string& danyuange_id : gongzuobu.ordered_cell_ids()) {
    const minisheet::CellRecord& danyuange = gongzuobu.cell(danyuange_id);
    cout << danyuange_id << " raw=" << danyuange.yuanshi << " display=" << danyuange.xianshi
         << '\n';
  }
}

}  // namespace

// ----------------------------------------------------------------------------
// 主函数
// 命令：
//   import <csv>    - 导入 CSV 并显示
//   save <csv> <dat> - 导入 CSV 并保存为 DAT
//   load <dat>      - 加载 DAT 并显示
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      print_usage();
      return 1;
    }

    string mingling = argv[1];
    if (mingling == "import") {
      minisheet::Workbook gongzuobu = minisheet::load_csv(argv[2]);
      print_workbook(gongzuobu);
      return 0;
    }

    if (mingling == "save" && argc >= 4) {
      minisheet::Workbook gongzuobu = minisheet::load_csv(argv[2]);
      minisheet::save_dat(argv[3], gongzuobu);
      print_workbook(gongzuobu);
      return 0;
    }

    if (mingling == "load") {
      minisheet::Workbook gongzuobu = minisheet::load_dat(argv[2]);
      print_workbook(gongzuobu);
      return 0;
    }

    print_usage();
    return 1;
  } catch (const std::exception& cuowu) {
    cerr << "minisheet_cli error: " << cuowu.what() << '\n';
    return 1;
  }
}
