#pragma once

// ========================================
// 这个文件负责单元格的显示处理
// 就是把用户输入的东西转换成应该显示的东西
// 它只处理“单个单元格自身就能决定的内容”：
// - 数字怎么显示
// - 字符串怎么显示
// - 公式先标记成 Formula 类型
// 它不负责跨单元格依赖，不会去读别的单元格；依赖计算由 m4 / m5 负责
// ========================================

#include "minisheet/m2_workbook.h"

// 判断用户输入的内容是什么类型
// 参数yuanshi是用户输入的原始字符串
// 返回值是类型（Empty/Integer/Float/String/Formula）
// 如果是数字类型，会通过shuzhi参数返回数值，shi_zhengshu参数告诉你是整数还是小数
CellKind classify_raw_kind(const string& yuanshi, double& shuzhi, bool& shi_zhengshu);

// 刷新普通单元格的显示值
// 这个函数处理非公式单元格（空/数字/文字）
// 根据类型设置正确的显示内容：
// - Empty: 显示空字符串
// - Integer/Float: 显示格式化后的数字
// - String: 原样显示
// - Formula: 不处理，留给公式模块处理
// 所以 m2::set_cell() 写入内容后，第一步总是先调用这里
// 公式单元格后续再由 m5 -> m4 补上真正的显示值
void refresh_literal_cell(CellRecord& danyuange);
