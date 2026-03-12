// ========================================
// 存储往返测试程序
// 这个测试验证CSV和DAT格式的正确性
// 测试流程：
//   1. 创建一个测试用的CSV文件
//   2. 加载CSV到工作簿
//   3. 保存工作簿为DAT文件
//   4. 从DAT加载到另一个工作簿
//   5. 保存第二个工作簿为新的CSV
//   6. 比较原始CSV和最终CSV是否一致
// 如果一致，说明序列化和反序列化是正确的
// ========================================

#include "minisheet/m6_storage.h"  // 需要存储功能

#include <filesystem>  // 创建目录
#include <fstream>     // 文件读写
#include <iostream>    // 输入输出
#include <iterator>    // 迭代器
#include <string>      // 字符串

using namespace std;

// 读取整个文件内容为字符串
string read_all(const string& lujing) {
  ifstream shuru(lujing, ios::binary);
  // 用迭代器一次性读取全部内容
  return string((istreambuf_iterator<char>(shuru)), istreambuf_iterator<char>());
}

// 主函数
int main() {
  // 测试目录
  const string mubiao_mulu = "backend/build-gcc/storage-roundtrip-test";

  // 创建测试目录（如果不存在）
  filesystem::create_directories(mubiao_mulu);

  // 定义文件路径
  const string csv_lujing = mubiao_mulu + "/input.csv";           // 原始CSV
  const string dat_lujing = mubiao_mulu + "/sheet.dat";           // 中间DAT
  const string huanyuan_csv_lujing = mubiao_mulu + "/roundtrip.csv";  // 最终CSV

  // 第一步：创建测试CSV文件
  {
    ofstream shuchu(csv_lujing, ios::binary);
    // 第一行：数字、空单元格、公式
    shuchu << "1,,=A1+2\n";
    // 第二行：空、文本、带逗号的文本（需要引号）
    shuchu << ",text,\"x,y\"\n";
    // 这里测试了：
    // - 普通数字
    // - 空单元格
    // - 公式
    // - 文本
    // - 带逗号的文本（CSV转义）
  }

  // 第二步到第五步：往返转换
  // CSV -> Workbook -> DAT -> Workbook -> CSV
  Workbook gongzuobu = load_csv(csv_lujing);     // CSV加载到工作簿
  save_dat(dat_lujing, gongzuobu);               // 保存为DAT
  Workbook huanyuan = load_dat(dat_lujing);      // 从DAT加载
  save_csv(huanyuan_csv_lujing, huanyuan);       // 保存为新的CSV

  // 第六步：比较原始CSV和最终CSV
  const string yuanshi = read_all(csv_lujing);           // 读取原始内容
  const string huanyuan_jieguo = read_all(huanyuan_csv_lujing);  // 读取最终内容

  // 检查是否一致
  if (yuanshi != huanyuan_jieguo) {
    cerr << "roundtrip mismatch\n";  // 不一致！
    cerr << "expected:\n" << yuanshi << "\n";
    cerr << "actual:\n" << huanyuan_jieguo << "\n";
    return 1;  // 测试失败，返回非0
  }

  // 测试通过！
  cout << "storage_roundtrip ok\n";
  return 0;  // 返回0表示成功
}
