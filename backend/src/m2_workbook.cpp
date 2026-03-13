// ========================================
// 工作簿核心实现文件
// 实现Workbook的各种操作：设置单元格、获取单元格等
// 这个文件不直接面对用户输入输出，而是给上层模块提供统一的数据操作接口：
// - CLI / Server / m6_storage 最终都会通过这里改 Workbook
// - 真正的显示刷新委托给 m3
// - 真正的公式重算委托给 m5
// ========================================

#include "minisheet/m2_workbook.h"

#include "minisheet/m3_display.h"  // 需要刷新单元格显示
#include "minisheet/m5_recalc.h"   // 需要重新计算公式

#include <algorithm>  // 为了用sort排序

using namespace std;

// 比较两个单元格ID的大小，用于排序
// 排序规则：先比行号，行号小的在前；行号相同再比列号
// 比如 A1 < A2 < B1 < B2
bool compare_cell_id(const string& zuo_id, const string& you_id) {
  // 先把ID解析成坐标
  CellCoord zuo_zuobiao = parse_cell_id(zuo_id);
  CellCoord you_zuobiao = parse_cell_id(you_id);

  // 行号不同就比行号
  if (zuo_zuobiao.hang != you_zuobiao.hang) {
    return zuo_zuobiao.hang < you_zuobiao.hang;
  }
  // 行号相同就比列号
  return zuo_zuobiao.lie < you_zuobiao.lie;
}

// 清空工作簿
void clear(Workbook& gongzuobu) {
  // 清空单元格哈希表
  gongzuobu.danyuange_biao_.clear();
  // 重置CSV行列数为0
  gongzuobu.yuan_csv_hang_shu = 0;
  gongzuobu.yuan_csv_lie_shu = 0;
}

// 设置单元格内容
void set_cell(Workbook& gongzuobu, const string& danyuange_id, const string& yuanshi) {
  // 先把ID规范化（比如"a1"变成"A1"）
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));

  // 如果输入是空字符串，就删除这个单元格
  if (yuanshi.empty()) {
    gongzuobu.danyuange_biao_.erase(guifanhou_id);
    return;
  }

  // 获取或创建这个单元格（用[]操作符会自动创建不存在的）
  CellRecord& danyuange_jilu = gongzuobu.danyuange_biao_[guifanhou_id];

  // 设置基本信息
  danyuange_jilu.biaoshi = guifanhou_id;  // 标识
  danyuange_jilu.yuanshi = yuanshi;       // 原始输入内容

  // 这里只刷新“字面值”状态：
  // - 纯数字 / 字符串会立刻得到显示值
  // - 公式只会被标记成 Formula，真正计算要等外层调用 recalculate_all
  refresh_literal_cell(danyuange_jilu);
}

// 获取单元格信息
const CellRecord& cell(const Workbook& gongzuobu, const string& danyuange_id) {
  // 规范化ID
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));

  // 在哈希表里查找
  auto dieqi = gongzuobu.danyuange_biao_.find(guifanhou_id);
  if (dieqi != gongzuobu.danyuange_biao_.end()) {
    return dieqi->second;  // 找到了，返回引用
  }

  // 没找到，返回一个空的单元格记录
  // 注意这里要修改kong_danyuange_，但函数是const的
  // 所以成员变量要声明为mutable
  gongzuobu.kong_danyuange_ = {};  // 清空
  gongzuobu.kong_danyuange_.biaoshi = guifanhou_id;
  gongzuobu.kong_danyuange_.leixing = CellKind::Empty;
  return gongzuobu.kong_danyuange_;
}

// 检查工作簿里有没有这个单元格
bool has_cell(const Workbook& gongzuobu, const string& danyuange_id) {
  // 规范化ID
  const string guifanhou_id = to_cell_id(parse_cell_id(danyuange_id));
  // 在哈希表里查找
  return gongzuobu.danyuange_biao_.find(guifanhou_id) != gongzuobu.danyuange_biao_.end();
}

// 获取所有单元格ID，按顺序排好
vector<string> ordered_cell_ids(const Workbook& gongzuobu) {
  vector<string> id_men;
  // 先预留空间，避免多次内存分配
  id_men.reserve(gongzuobu.danyuange_biao_.size());

  // 把所有ID取出来
  for (const auto& xiang : gongzuobu.danyuange_biao_) {
    id_men.push_back(xiang.first);
  }

  // 用自定义的比较函数排序
  sort(id_men.begin(), id_men.end(), compare_cell_id);
  return id_men;
}

// 获取单元格表的只读引用
const unordered_map<string, CellRecord>& cells(const Workbook& gongzuobu) {
  return gongzuobu.danyuange_biao_;
}

// 获取单元格表的可写引用
unordered_map<string, CellRecord>& mutable_cells(Workbook& gongzuobu) {
  return gongzuobu.danyuange_biao_;
}

// 重新计算所有公式
void recalculate_all(Workbook& gongzuobu) {
  // m2 自己不实现重算逻辑，而是把调度工作交给 m5
  // 这样 app 层只需要记住“改完数据后调 recalculate_all”这一层接口
  recalculate_all_cells(gongzuobu);
}

// 从某个单元格开始重新计算
void recalculate_from(Workbook& gongzuobu, const string& danyuange_id) {
  // 目前虽然名字叫“从某个单元格开始”，但实现仍然委托给 m5 的简化版全量重算
  recalculate_impacted_cells(gongzuobu, danyuange_id);
}
