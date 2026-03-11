#pragma once

/**
 * @file m4_formula.h
 * @brief 公式解析和求值模块
 *
 * 本文件定义公式求值的接口，支持：
 * - 数学表达式（通过 tinyexpr 库）
 * - 单元格引用（如 A1、B2）
 * - 范围函数（SUM、AVG）
 * - 嵌套函数调用
 */

#include "minisheet/m1_types.h"

#include <functional>
#include <string>

namespace minisheet {

// ============================================================================
// 结果和回调类型
// ============================================================================

/**
 * @brief 公式求值结果
 */
struct FormulaEvalResult {
  bool chenggong = false;  /** 求值是否成功 */
  double shuzhi = 0.0;     /** 求值结果（仅当 chenggong 为 true 时有效） */
};

/**
 * @brief 单元格解析器回调
 *
 * 用于在公式求值时获取单元格的数值。
 * @param cell_id 单元格ID，如 "A1"
 * @param value 输出：单元格的数值
 * @return true 如果成功获取到数值
 */
using CellResolver = std::function<bool(const std::string&, double&)>;

/**
 * @brief 范围解析器回调
 *
 * 用于在公式求值时计算范围函数（SUM、AVG）。
 * @param range 单元格范围
 * @param shi_pingjun true 表示计算平均值，false 表示计算总和
 * @param value 输出：计算结果
 * @return true 如果成功计算
 */
using RangeResolver = std::function<bool(const CellRange&, bool shi_pingjun, double&)>;

// ============================================================================
// 公式求值
// ============================================================================

/**
 * @brief 求值公式
 *
 * 完整的公式求值流程：
 * 1. 去除 "=" 前缀
 * 2. 重写表达式（处理 SUM/AVG 等函数）
 * 3. 使用 tinyexpr 求值数学表达式
 *
 * @param gongshi 公式字符串（如 "=A1+B1"、"=SUM(A1:A10)"）
 * @param danyuange_jiexiqi 单元格解析器，用于获取引用的单元格值
 * @param fanwei_jiexiqi 范围解析器，用于计算 SUM/AVG
 * @return 求值结果
 */
FormulaEvalResult evaluate_formula(const std::string& gongshi,
                                   const CellResolver& danyuange_jiexiqi,
                                   const RangeResolver& fanwei_jiexiqi);

}  // namespace minisheet
