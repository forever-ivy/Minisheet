#pragma once

/**
 * @file m3_display.h
 * @brief 单元格显示处理模块
 *
 * 本文件提供单元格类型分类和显示刷新功能。
 * 根据单元格的原始内容自动识别类型（数字、字符串、公式等），
 * 并生成对应的显示值。
 */

#include "minisheet/m2_workbook.h"

namespace minisheet {

/**
 * @brief 根据原始内容分类单元格类型
 *
 * 分类优先级：
 * 1. 空字符串 -> Empty
 * 2. 以 = 开头 -> Formula
 * 3. 可解析为数字 -> Integer/Float
 * 4. 其他 -> String
 *
 * @param yuanshi 原始输入内容
 * @param shuzhi 输出：解析出的数值（如果是数字类型）
 * @param shi_zhengshu 输出：是否为整数
 * @return 单元格类型
 */
CellKind classify_raw_kind(const std::string& yuanshi, double& shuzhi, bool& shi_zhengshu);

/**
 * @brief 刷新字面量单元格的显示值
 *
 * 根据单元格类型更新显示文本和数值属性：
 * - Empty：显示为空
 * - Integer/Float：格式化显示数值
 * - String：原样显示
 * - Formula：暂不处理（由重计算模块处理）
 *
 * @param danyuange 要刷新的单元格记录（会被修改）
 */
void refresh_literal_cell(CellRecord& danyuange);

}  // namespace minisheet
