/**
 * @file m1_types.cpp
 * @brief Minisheet 核心类型的实现
 *
 * 本文件实现了 m1_types.h 中声明的工具函数，包括：
 * - Excel 风格的列名与列号转换
 * - 单元格坐标解析与格式化
 * - 字符串和数值处理
 * - 文件读写操作
 */

#include "minisheet/m1_types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace minisheet {

// ============================================================================
// 列名转换
// ============================================================================

// ----------------------------------------------------------------------------
// 将列号转换为列名（1 -> A, 26 -> Z, 27 -> AA）
// 算法：26进制转换，但A=1而不是0，所以需要先减1
// ----------------------------------------------------------------------------
std::string column_index_to_name(int yiji_lie) {
  if (yiji_lie < 1 || yiji_lie > kMaxColumns) {
    throw std::invalid_argument("column out of range");
  }

  std::string lie_ming;
  int lie_zhi = yiji_lie;
  while (lie_zhi > 0) {
    lie_zhi -= 1;
    lie_ming.push_back(static_cast<char>('A' + (lie_zhi % 26)));
    lie_zhi /= 26;
  }
  std::reverse(lie_ming.begin(), lie_ming.end());
  return lie_ming;
}

// ----------------------------------------------------------------------------
// 将列名转换为列号（A -> 1, Z -> 26, AA -> 27）
// 算法：26进制转换，A=1, B=2, ..., Z=26
// ----------------------------------------------------------------------------
int column_name_to_index(const std::string& lie_ming) {
  if (lie_ming.empty()) {
    throw std::invalid_argument("empty column name");
  }

  int lie_zhi = 0;
  for (char zifu : lie_ming) {
    if (!std::isalpha(static_cast<unsigned char>(zifu))) {
      throw std::invalid_argument("invalid column name");
    }
    lie_zhi = lie_zhi * 26 + (std::toupper(static_cast<unsigned char>(zifu)) - 'A' + 1);
  }

  if (lie_zhi < 1 || lie_zhi > kMaxColumns) {
    throw std::invalid_argument("column out of range");
  }
  return lie_zhi;
}

// ============================================================================
// 单元格坐标处理
// ============================================================================

// ----------------------------------------------------------------------------
// 解析单元格ID，分离列名和行号
// 例如："A1" -> {row=1, column=1}, "BC123" -> {row=123, column=55}
// ----------------------------------------------------------------------------
CellCoord parse_cell_id(const std::string& danyuange_id) {
  std::string lie_ming;
  std::string hang_haoma;

  // 遍历字符，字母归为列名，数字归为行号
  for (char zifu : danyuange_id) {
    if (std::isalpha(static_cast<unsigned char>(zifu)) && hang_haoma.empty()) {
      // 列名必须出现在行号之前，且转为大写
      lie_ming.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(zifu))));
    } else if (std::isdigit(static_cast<unsigned char>(zifu))) {
      hang_haoma.push_back(zifu);
    } else {
      throw std::invalid_argument("invalid cell id");
    }
  }

  // 必须同时有列名和行号
  if (lie_ming.empty() || hang_haoma.empty()) {
    throw std::invalid_argument("invalid cell id");
  }

  // 构造坐标并验证范围
  CellCoord zuobiao {std::stoi(hang_haoma), column_name_to_index(lie_ming)};
  if (!is_valid_coord(zuobiao)) {
    throw std::invalid_argument("cell id out of range");
  }
  return zuobiao;
}

// ----------------------------------------------------------------------------
// 将坐标转换为单元格ID字符串
// 例如：{row=1, column=1} -> "A1"
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// 将坐标转换为单元格ID字符串
// 例如：{row=1, column=1} -> "A1"
// ----------------------------------------------------------------------------
std::string to_cell_id(const CellCoord& zuobiao) {
  if (!is_valid_coord(zuobiao)) {
    throw std::invalid_argument("coord out of range");
  }
  return column_index_to_name(zuobiao.lie) + std::to_string(zuobiao.hang);
}

// ----------------------------------------------------------------------------
// 检查坐标是否在有效范围内（1-based）
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// 检查坐标是否在有效范围内（1-based）
// ----------------------------------------------------------------------------
bool is_valid_coord(const CellCoord& zuobiao) {
  return zuobiao.hang >= 1 && zuobiao.hang <= kMaxRows && zuobiao.lie >= 1 &&
         zuobiao.lie <= kMaxColumns;
}

// ----------------------------------------------------------------------------
// 检查单元格ID字符串是否有效
// 通过尝试解析来判断，捕获异常则表示无效
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// 检查单元格ID字符串是否有效
// 通过尝试解析来判断，捕获异常则表示无效
// ----------------------------------------------------------------------------
bool is_valid_cell_id(const std::string& danyuange_id) {
  try {
    (void)parse_cell_id(danyuange_id);
    return true;
  } catch (...) {
    return false;
  }
}

// ============================================================================
// 字符串处理
// ============================================================================

// ----------------------------------------------------------------------------
// 去除字符串首尾空白字符（空格、制表符、换行等）
// ----------------------------------------------------------------------------
std::string trim(const std::string& zhi) {
  std::size_t qidian = 0;
  while (qidian < zhi.size() && std::isspace(static_cast<unsigned char>(zhi[qidian]))) {
    qidian += 1;
  }

  std::size_t zhongdian = zhi.size();
  while (zhongdian > qidian && std::isspace(static_cast<unsigned char>(zhi[zhongdian - 1]))) {
    zhongdian -= 1;
  }

  return zhi.substr(qidian, zhongdian - qidian);
}

// ============================================================================
// 数值处理
// ============================================================================

// ----------------------------------------------------------------------------
// 尝试将字符串解析为数字
// 逻辑：
// 1. 先 trim 去除空白
// 2. 用 strtod 解析
// 3. 检查是否包含 ".eE" 来判断是否为整数
// ----------------------------------------------------------------------------
bool try_parse_number(const std::string& yuanshi, double& zhi, bool& shi_zhengshu) {
  std::string houxuan = trim(yuanshi);
  if (houxuan.empty()) {
    return false;
  }

  char* jieshu_zhizhen = nullptr;
  zhi = std::strtod(houxuan.c_str(), &jieshu_zhizhen);
  if (jieshu_zhizhen == nullptr || *jieshu_zhizhen != '\0') {
    return false;
  }

  shi_zhengshu = houxuan.find_first_of(".eE") == std::string::npos;
  return true;
}

// ----------------------------------------------------------------------------
// 格式化数值为显示字符串
// 逻辑：
// 1. 非有限数（inf, nan）返回 "#NA"
// 2. 接近整数的值显示为整数
// 3. 小数去除末尾多余的0
// ----------------------------------------------------------------------------
std::string format_number(double zhi) {
  if (!std::isfinite(zhi)) {
    return "#NA";
  }

  double quzheng_zhi = std::round(zhi);
  if (std::fabs(zhi - quzheng_zhi) < 1e-9) {
    return std::to_string(static_cast<long long>(quzheng_zhi));
  }

  std::ostringstream liu;
  liu << std::fixed << std::setprecision(10) << zhi;
  std::string wenben = liu.str();
  while (!wenben.empty() && wenben.back() == '0') {
    wenben.pop_back();
  }
  if (!wenben.empty() && wenben.back() == '.') {
    wenben.pop_back();
  }
  return wenben;
}

// ============================================================================
// 文件 I/O
// ============================================================================

// ----------------------------------------------------------------------------
// 读取文本文件全部内容
// 以二进制模式打开，保留原始内容不变
// ----------------------------------------------------------------------------
std::string read_text_file(const std::string& lujing) {
  std::ifstream shuru(lujing, std::ios::binary);
  if (!shuru) {
    throw std::runtime_error("failed to open file for reading");
  }

  std::ostringstream huanchong;
  huanchong << shuru.rdbuf();
  return huanchong.str();
}

// ----------------------------------------------------------------------------
// 写入文本文件
// 自动创建父目录（如果不存在），以二进制模式写入
// ----------------------------------------------------------------------------
void write_text_file(const std::string& lujing, const std::string& neirong) {
  std::filesystem::path wenjian_lujing(lujing);
  if (!wenjian_lujing.parent_path().empty()) {
    std::filesystem::create_directories(wenjian_lujing.parent_path());
  }

  std::ofstream shuchu(lujing, std::ios::binary);
  if (!shuchu) {
    throw std::runtime_error("failed to open file for writing");
  }
  shuchu << neirong;
}

}  // namespace minisheet
