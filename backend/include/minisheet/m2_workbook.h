#pragma once

/**
 * @file m2_workbook.h
 * @brief 工作簿核心类定义
 *
 * 本文件定义 CellRecord 结构体和 Workbook 类，
 * 用于存储和管理电子表格的所有单元格数据。
 */

#include "minisheet/m1_types.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace minisheet {

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @brief 单元格记录
 *
 * 存储单个单元格的完整信息，包括：
 * - 标识、原始内容、显示值
 * - 类型、数值状态、计算值
 * - 错误信息（如果有）
 */
struct CellRecord {
  std::string biaoshi;   /** 单元格标识，如 "A1" */
  std::string yuanshi;   /** 用户输入的原始内容 */
  std::string xianshi;   /** 格式化后的显示值 */
  std::string cuowu;     /** 错误信息，无错误为空 */
  CellKind leixing = CellKind::Empty;  /** 单元格类型 */
  bool you_shuzhi = false;  /** 是否包含有效数值 */
  double shuzhi = 0.0;      /** 数值（如果是数字类型或公式结果） */
};

// ============================================================================
// 工作簿类
// ============================================================================

/**
 * @brief 工作簿类
 *
 * 管理整个电子表格的数据，提供单元格的增删改查接口，
 * 支持公式重计算。
 */
class Workbook {
 public:
  /** 默认构造函数 */
  Workbook() = default;

  // --------------------------------------------------------------------------
  // 单元格操作
  // --------------------------------------------------------------------------

  /** 清空工作簿，删除所有单元格 */
  void clear();

  /**
   * @brief 设置单元格内容
   * @param danyuange_id 单元格ID，如 "A1"
   * @param yuanshi 原始内容，空字符串表示删除
   */
  void set_cell(const std::string& danyuange_id, const std::string& yuanshi);

  /**
   * @brief 获取单元格记录（只读）
   * @param danyuange_id 单元格ID
   * @return 单元格记录引用，不存在返回空单元格
   */
  const CellRecord& cell(const std::string& danyuange_id) const;

  /**
   * @brief 检查单元格是否存在
   * @param danyuange_id 单元格ID
   * @return true 如果单元格存在
   */
  bool has_cell(const std::string& danyuange_id) const;

  /**
   * @brief 获取按行列排序的单元格ID列表
   * @return 排序后的ID列表（先按行，再按列）
   */
  std::vector<std::string> ordered_cell_ids() const;

  // --------------------------------------------------------------------------
  // 批量访问（用于内部操作）
  // --------------------------------------------------------------------------

  /** 获取所有单元格的只读引用 */
  const std::unordered_map<std::string, CellRecord>& cells() const;

  /** 获取所有单元格的可写引用（供重计算模块使用） */
  std::unordered_map<std::string, CellRecord>& mutable_cells();

  // --------------------------------------------------------------------------
  // 计算管理
  // --------------------------------------------------------------------------

  /** 重新计算所有公式单元格 */
  void recalculate_all();

  /**
   * @brief 从指定单元格开始重计算
   * @param danyuange_id 起始单元格ID
   * @note 当前实现为全量重算，后续可优化为增量计算
   */
  void recalculate_from(const std::string& danyuange_id);

 private:
  /** 单元格存储表，以单元格ID为键 */
  std::unordered_map<std::string, CellRecord> danyuange_biao_;

  /** 空单元格缓存（用于 const 方法返回不存在单元格） */
  mutable CellRecord kong_danyuange_;
};

}  // namespace minisheet
