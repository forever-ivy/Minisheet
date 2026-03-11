/**
 * @file m3_display.cpp
 * @brief 单元格类型分类和显示刷新
 *
 * 本文件实现：
 * - 根据原始内容自动识别单元格类型（空/整数/浮点数/字符串/公式）
 * - 刷新单元格的显示值和数值属性
 */

#include "minisheet/m3_display.h"

namespace minisheet {

// ----------------------------------------------------------------------------
// 根据原始内容分类单元格类型
// 优先级：空字符串 > 公式（以=开头）> 数字 > 字符串
// ----------------------------------------------------------------------------
// 参数：
//   yuanshi    - 原始输入内容
//   shuzhi     - 输出：解析出的数值（如果是数字类型）
//   shi_zhengshu - 输出：是否为整数
// 返回：
//   CellKind 类型枚举值
// ----------------------------------------------------------------------------
CellKind classify_raw_kind(const std::string& yuanshi, double& shuzhi, bool& shi_zhengshu) {
  if (yuanshi.empty()) {
    shuzhi = 0.0;
    shi_zhengshu = false;
    return CellKind::Empty;
  }

  // 以 = 开头的是公式
  if (!yuanshi.empty() && yuanshi.front() == '=') {
    shuzhi = 0.0;
    shi_zhengshu = false;
    return CellKind::Formula;
  }

  // 尝试解析为数字
  if (try_parse_number(yuanshi, shuzhi, shi_zhengshu)) {
    return shi_zhengshu ? CellKind::Integer : CellKind::Float;
  }

  // 其他情况为字符串
  shuzhi = 0.0;
  shi_zhengshu = false;
  return CellKind::String;
}

// ----------------------------------------------------------------------------
// 刷新字面量单元格的显示值
// 根据单元格类型更新显示文本和数值属性：
// - Empty：显示为空
// - Integer/Float：格式化显示数值
// - String：原样显示
// - Formula：暂不处理（由重计算模块处理）
// ----------------------------------------------------------------------------
void refresh_literal_cell(CellRecord& danyuange) {
  // 重置错误状态和数值标志
  danyuange.cuowu.clear();
  danyuange.you_shuzhi = false;
  danyuange.shuzhi = 0.0;

  // 分类原始内容
  double jiexi_shuzhi = 0.0;
  bool shi_zhengshu = false;
  danyuange.leixing = classify_raw_kind(danyuange.yuanshi, jiexi_shuzhi, shi_zhengshu);

  // 根据类型更新显示
  switch (danyuange.leixing) {
    case CellKind::Empty:
      danyuange.xianshi.clear();
      break;
    case CellKind::Integer:
    case CellKind::Float:
      danyuange.you_shuzhi = true;
      danyuange.shuzhi = jiexi_shuzhi;
      danyuange.xianshi = format_number(jiexi_shuzhi);
      break;
    case CellKind::String:
      danyuange.xianshi = danyuange.yuanshi;
      break;
    case CellKind::Formula:
      danyuange.xianshi.clear();
      break;
  }
}

}  // namespace minisheet
