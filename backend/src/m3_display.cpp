#include "minisheet/m3_display.h"

CellKind classify_raw_kind(const string& yuanshi, double& shuzhi, bool& shi_zhengshu) {
  shuzhi = 0.0;
  shi_zhengshu = false;

  if (yuanshi.empty()) {
    return CellKind::Empty;
  }
  if (yuanshi.front() == '=') {
    return CellKind::Formula;
  }
  if (try_parse_number(yuanshi, shuzhi, shi_zhengshu)) {
    return shi_zhengshu ? CellKind::Integer : CellKind::Float;
  }
  return CellKind::String;
}

void refresh_literal_cell(CellRecord& danyuange) {
  danyuange.cuowu.clear();
  danyuange.you_shuzhi = false;
  danyuange.shuzhi = 0.0;

  double jiexi_shuzhi = 0.0;
  bool shi_zhengshu = false;
  danyuange.leixing = classify_raw_kind(danyuange.yuanshi, jiexi_shuzhi, shi_zhengshu);

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
