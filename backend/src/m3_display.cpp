// ========================================
// 显示处理实现文件
// 负责把用户输入转换成应该显示的内容
// ========================================

#include "minisheet/m3_display.h"

// 判断用户输入的内容是什么类型
// 返回值：Empty（空）、Integer（整数）、Float（小数）、String（文字）、Formula（公式）
CellKind classify_raw_kind(const string& yuanshi, double& shuzhi, bool& shi_zhengshu) {
  // 先初始化输出参数
  shuzhi = 0.0;
  shi_zhengshu = false;

  // 空字符串就是Empty类型
  if (yuanshi.empty()) {
    return CellKind::Empty;
  }

  // 如果以=开头，就是公式类型
  // 比如 =A1+B1、=SUM(A1:A10)
  if (yuanshi.front() == '=') {
    return CellKind::Formula;
  }

  // 尝试解析成数字
  if (try_parse_number(yuanshi, shuzhi, shi_zhengshu)) {
    // 解析成功了，根据shi_zhengshu判断是整数还是小数
    return shi_zhengshu ? CellKind::Integer : CellKind::Float;
  }

  // 都不是，就当作文本字符串
  return CellKind::String;
}

// 刷新普通单元格的显示值
// 这个函数处理非公式单元格
void refresh_literal_cell(CellRecord& danyuange) {
  // 先清空之前的状态
  danyuange.cuowu.clear();      // 清除错误信息
  danyuange.you_shuzhi = false; // 重置数值标志
  danyuange.shuzhi = 0.0;       // 重置数值

  // 用来接收classify_raw_kind的输出
  double jiexi_shuzhi = 0.0;
  bool shi_zhengshu = false;

  // 判断类型
  danyuange.leixing = classify_raw_kind(danyuange.yuanshi, jiexi_shuzhi, shi_zhengshu);

  // 根据类型设置显示内容
  switch (danyuange.leixing) {
    case CellKind::Empty:
      // 空的就显示空字符串
      danyuange.xianshi.clear();
      break;

    case CellKind::Integer:
    case CellKind::Float:
      // 数字类型要记录数值，并格式化成字符串显示
      danyuange.you_shuzhi = true;        // 标记有数值
      danyuange.shuzhi = jiexi_shuzhi;    // 保存数值
      danyuange.xianshi = format_number(jiexi_shuzhi);  // 格式化显示
      break;

    case CellKind::String:
      // 字符串原样显示
      danyuange.xianshi = danyuange.yuanshi;
      break;

    case CellKind::Formula:
      // 公式类型的显示值暂时清空，等公式模块计算后再填
      danyuange.xianshi.clear();
      break;
  }
}
