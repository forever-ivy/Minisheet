#pragma once

// ========================================
// 这是工作簿（Workbook）的核心定义文件
// 工作簿就是整个电子表格，里面包含了很多单元格
// ========================================

#include "minisheet/m1_types.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// 单元格记录结构体
// 每个单元格都要保存这些信息：
// - biaoshi: 单元格的ID，比如"A1"
// - yuanshi: 用户输入的原始内容，比如"=A1+B1"或"123"
// - xianshi: 显示给用户看的内容，比如计算后的"300"或原样显示的"hello"
// - cuowu: 如果有错误，这里放错误信息，比如"#NA"
// - leixing: 这个单元格是什么类型（空/整数/小数/文字/公式）
// - you_shuzhi: 这个单元格有没有数值（公式计算完或者输入的是数字）
// - shuzhi: 具体的数值是多少
struct CellRecord {
  string biaoshi;           // 单元格标识，如"A1"
  string yuanshi;           // 原始输入内容
  string xianshi;           // 显示内容
  string cuowu;             // 错误信息
  CellKind leixing = CellKind::Empty;  // 单元格类型
  bool you_shuzhi = false;  // 是否有数值
  double shuzhi = 0.0;      // 数值
};

// 工作簿结构体
// 这就是整个电子表格的核心数据结构
// - danyuange_biao_: 用哈希表存所有单元格，key是单元格ID（如"A1"）
// - kong_danyuange_: 一个空的单元格记录，用来返回给查询不存在的单元格时用
// - yuan_csv_hang_shu/lie_shu: 记录原始CSV文件的行列数，用来保存时保持形状
struct Workbook {
  unordered_map<string, CellRecord> danyuange_biao_;  // 单元格表
  mutable CellRecord kong_danyuange_;                 // 空单元格模板
  int yuan_csv_hang_shu = 0;  // 原始CSV行数
  int yuan_csv_lie_shu = 0;   // 原始CSV列数
};

// 清空整个工作簿，把所有单元格都删掉
// 同时重置CSV行列数为0
void clear(Workbook& gongzuobu);

// 设置单元格的内容
// 如果yuanshi是空字符串，就删除这个单元格
// 否则就创建或更新这个单元格，并刷新它的显示值
void set_cell(Workbook& gongzuobu, const string& danyuange_id, const string& yuanshi);

// 获取单元格的信息
// 如果单元格不存在，返回kong_danyuange_（一个空的单元格记录）
const CellRecord& cell(const Workbook& gongzuobu, const string& danyuange_id);

// 检查工作簿里有没有这个单元格
// 返回true表示有，false表示没有
bool has_cell(const Workbook& gongzuobu, const string& danyuange_id);

// 获取所有单元格ID，按行列顺序排好
// 顺序是先行号从小到大，同行再按列号从小到大
// 比如 A1, A2, B1, B2 这样
vector<string> ordered_cell_ids(const Workbook& gongzuobu);

// 获取单元格表的只读引用
// 用来遍历所有单元格，但不能修改
const unordered_map<string, CellRecord>& cells(const Workbook& gongzuobu);

// 获取单元格表的可写引用
// 用来遍历并修改单元格
unordered_map<string, CellRecord>& mutable_cells(Workbook& gongzuobu);

// 重新计算所有公式单元格
// 遍历所有单元格，把所有Formula类型的都重新算一遍
void recalculate_all(Workbook& gongzuobu);

// 从某个单元格开始重新计算
// 这个会计算所有依赖这个单元格的公式（暂时实现是重算所有）
void recalculate_from(Workbook& gongzuobu, const string& danyuange_id);
