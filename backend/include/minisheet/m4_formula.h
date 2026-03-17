#pragma once

// ========================================
// 公式计算模块的头文件
// 负责解析和计算公式，比如 =A1+B1 这种
// 这个模块是最复杂的，要处理依赖关系、范围函数等
// 它的典型上游和下游关系是：
// - 上游：主要由 m5_recalc 调用，也会被 m6_storage 间接触发
// - 下游：依赖 m2_workbook 读取/写回单元格状态，依赖 tinyexpr 做纯数学求值
// 读代码时可以把它理解成“公式引擎”
// ========================================

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

using namespace std;

// 公式计算结果结构体
// chenggong: 计算是否成功
// shuzhi: 计算出的数值（只有chenggong为true时才有效）
struct FormulaEvalResult
{
  bool chenggong = false; // 是否成功
  double shuzhi = 0.0;    // 计算结果
};

// 计算单个单元格的数值
// 如果单元格是数字类型，直接返回值
// 如果单元格是公式类型，递归计算公式
// 这是 m4 对外最核心的入口，m5 扫描到公式单元格时最终就是调它
// fangwen_zhong: 为了兼容调用接口保留下来的递归上下文，当前简化版不使用
// yiwancheng: 已经计算完成的单元格列表
// shuzhi: 输出的数值
// 返回值true表示成功，false表示失败（公式错误、字符串参与数值计算等）
bool evaluate_cell_numeric(Workbook &gongzuobu,
                           const string &danyuange_id,
                           vector<string> &fangwen_zhong,
                           vector<string> &yiwancheng,
                           double &shuzhi);

// 计算一个范围内的数值总和或平均值
// fanwei: 单元格范围，比如 A1:B10
// shi_pingjun: true算平均值，false算总和
// 其他的参数跟上面那个函数一样
// 这个函数通常不会被 app 层直接调用，而是被 evaluate_formula 内部用于 SUM/AVG
bool evaluate_range_numeric(Workbook &gongzuobu,
                            const CellRange &fanwei,
                            bool shi_pingjun,
                            vector<string> &fangwen_zhong,
                            vector<string> &yiwancheng,
                            double &shuzhi);

// 计算公式字符串
// gongshi: 公式字符串，比如 "=A1+B1" 或 "SUM(A1:A10)"
// 返回计算结果结构体，包含成功标志和数值
// 这是“字符串公式 -> 数值结果”的入口，通常由 evaluate_cell_numeric 在内部调用
FormulaEvalResult evaluate_formula(Workbook &gongzuobu,
                                   const string &gongshi,
                                   vector<string> &fangwen_zhong,
                                   vector<string> &yiwancheng);
