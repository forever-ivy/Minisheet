#pragma once

/**
 * @file m1_types.h
 * @brief Minisheet 核心类型定义
 *
 * 本文件包含电子表格系统的基础数据类型、常量定义和工具函数。
 * 所有内容都位于 minisheet 命名空间下。
 */

#include <string>

namespace minisheet {

// ============================================================================
// 常量定义
// ============================================================================

/** 最大行数限制 (Excel 兼容) */
constexpr int kMaxRows = 32767;
/** 最大列数限制 (Excel 兼容: A-IV) */
constexpr int kMaxColumns = 256;

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @brief 单元格数据类型枚举
 *
 * 表示单元格中存储的数据种类，用于运行时类型识别。
 */
enum class CellKind {
  Empty,    /** 空单元格，无数据 */
  Integer,  /** 整数类型 */
  Float,    /** 浮点数类型 */
  String,   /** 字符串类型 */
  Formula,  /** 公式类型，需要计算 */
};

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @brief 单元格坐标
 *
 * 使用 0-based 索引表示单元格位置：
 * - hang: 行号，0 表示第 1 行
 * - lie: 列号，0 表示 A 列
 */
struct CellCoord {
  int hang = 0;
  int lie = 0;
};

/**
 * @brief 单元格范围
 *
 * 表示一个矩形区域，包含起始单元格和结束单元格（包含边界）。
 * 例如：A1:C3 表示从 A1 到 C3 的 3x3 区域。
 */
struct CellRange {
  CellCoord qishi;
  CellCoord jieshu;
};

// ============================================================================
// 坐标转换工具函数
// ============================================================================

/**
 * @brief 将列号转换为列名
 * @param yiji_lie 1-based 列号（1 = A, 2 = B, ...）
 * @return 列名字符串（如 "A", "Z", "AA"）
 */
std::string column_index_to_name(int yiji_lie);

/**
 * @brief 将列名转换为列号
 * @param lie_ming 列名（如 "A", "AA"）
 * @return 1-based 列号，无效名称返回 -1
 */
int column_name_to_index(const std::string& lie_ming);

/**
 * @brief 解析单元格标识符
 * @param danyuange_id 单元格 ID（如 "A1", "BC123"）
 * @return 解析后的坐标，无效 ID 返回 {-1, -1}
 */
CellCoord parse_cell_id(const std::string& danyuange_id);

/**
 * @brief 将坐标转换为单元格标识符
 * @param zuobiao 单元格坐标
 * @return 单元格 ID（如 "A1"）
 */
std::string to_cell_id(const CellCoord& zuobiao);

/**
 * @brief 检查坐标是否在有效范围内
 * @param zuobiao 待检查的坐标
 * @return true 如果坐标有效
 */
bool is_valid_coord(const CellCoord& zuobiao);

/**
 * @brief 检查单元格标识符是否有效
 * @param danyuange_id 待检查的单元格 ID
 * @return true 如果格式正确且在范围内
 */
bool is_valid_cell_id(const std::string& danyuange_id);

// ============================================================================
// 字符串和数值工具函数
// ============================================================================

/**
 * @brief 去除字符串首尾空白字符
 * @param zhi 输入字符串
 * @return 处理后的字符串
 */
std::string trim(const std::string& zhi);

/**
 * @brief 尝试将字符串解析为数字
 * @param yuanshi 输入字符串
 * @param zhi 输出：解析后的数值
 * @param shi_zhengshu 输出：是否为整数
 * @return true 如果成功解析为数字
 */
bool try_parse_number(const std::string& yuanshi, double& zhi, bool& shi_zhengshu);

/**
 * @brief 格式化数值为显示字符串
 * @param zhi 数值
 * @return 格式化后的字符串
 */
std::string format_number(double zhi);

// ============================================================================
// 文件 I/O 工具函数
// ============================================================================

/**
 * @brief 读取文本文件内容
 * @param lujing 文件路径
 * @return 文件内容，失败返回空字符串
 */
std::string read_text_file(const std::string& lujing);

/**
 * @brief 写入文本文件
 * @param lujing 文件路径
 * @param neirong 要写入的内容
 */
void write_text_file(const std::string& lujing, const std::string& neirong);

}  // namespace minisheet
