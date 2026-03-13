// ========================================
// MiniSheet命令行工具
// 这是一个命令行程序，用来在终端里操作电子表格
// 功能包括：导入CSV、保存为DAT格式、加载DAT、CSV和DAT互转等
// 适合批量处理或脚本中使用
// ========================================

#include "minisheet/m6_storage.h"  // 文件存储功能

#include <exception>  // 异常处理
#include <iostream>   // 输入输出
#include <string>     // 字符串

using namespace std;

// 打印使用帮助
// 告诉用户这个程序怎么用
void print_usage() {
  cout << "Usage:\n"
       << "  minisheet_cli import <input.csv>           # 导入CSV并显示内容\n"
       << "  minisheet_cli save <input.csv> <output.dat> # 导入CSV并保存为DAT\n"
       << "  minisheet_cli load <input.dat>              # 加载DAT并显示内容\n"
       << "  minisheet_cli pack <input.csv> <output.dat> # CSV打包成DAT（跟save一样）\n"
       << "  minisheet_cli unpack <input.dat> <output.csv> # DAT解包成CSV\n";
}

// 打印工作簿内容
// 遍历所有单元格，显示每个单元格的ID、原始内容、显示内容
void print_workbook(const Workbook& gongzuobu) {
  // ordered_cell_ids返回按行列排序好的ID列表
  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);
    cout << danyuange_id << " raw=" << danyuange.yuanshi << " display=" << danyuange.xianshi
         << '\n';
  }
}

// 主函数
int main(int argc, char** argv) {
  try {
    // 检查参数数量，至少需要命令和文件名
    if (argc < 3) {
      print_usage();
      return 1;  // 返回非0表示出错
    }

    string mingling = argv[1];  // 第一个参数是命令

    // 处理 import 命令
    // 用法：minisheet_cli import <input.csv>
    // 功能：加载CSV文件并显示所有单元格内容
    if (mingling == "import") {
      print_workbook(load_csv(argv[2]));
      return 0;  // 成功返回0
    }

    // 处理 save 命令
    // 用法：minisheet_cli save <input.csv> <output.dat>
    // 功能：加载CSV然后保存为DAT格式
    if (mingling == "save" && argc >= 4) {
      Workbook gongzuobu = load_csv(argv[2]);
      save_dat(argv[3], gongzuobu);
      print_workbook(gongzuobu);  // 顺便显示一下
      return 0;
    }

    // 处理 pack 命令
    // 用法：minisheet_cli pack <input.csv> <output.dat>
    // 功能：跟save一样，只是换个叫法，更直观
    if (mingling == "pack" && argc >= 4) {
      Workbook gongzuobu = load_csv(argv[2]);
      save_dat(argv[3], gongzuobu);
      cout << "packed " << argv[2] << " -> " << argv[3] << '\n';
      return 0;
    }

    // 处理 load 命令
    // 用法：minisheet_cli load <input.dat>
    // 功能：加载DAT二进制文件并显示内容
    if (mingling == "load") {
      print_workbook(load_dat(argv[2]));
      return 0;
    }

    // 处理 unpack 命令
    // 用法：minisheet_cli unpack <input.dat> <output.csv>
    // 功能：把DAT文件解包成CSV
    if (mingling == "unpack" && argc >= 4) {
      Workbook gongzuobu = load_dat(argv[2]);
      save_csv(argv[3], gongzuobu);
      cout << "unpacked " << argv[2] << " -> " << argv[3] << '\n';
      return 0;
    }

    // 命令不认识，打印帮助
    print_usage();
    return 1;

  } catch (const exception& cuowu) {
    // 捕获所有异常，打印错误信息
    cerr << "minisheet_cli error: " << cuowu.what() << '\n';
    return 1;
  }
}
