/**
 * @file m6_storage.cpp
 * @brief 存储持久化实现
 *
 * 本文件实现：
 * - CSV 格式导入（兼容 Excel 格式）
 * - 二进制 DAT 格式序列化/反序列化
 * - 支持引号和转义的 CSV 解析
 */

#include "minisheet/m6_storage.h"

#include "minisheet/m1_types.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace std;

namespace minisheet {
namespace {

// ----------------------------------------------------------------------------
// DAT 文件格式常量
// ----------------------------------------------------------------------------
constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};  // 魔数：Minisheet
constexpr std::uint32_t kVersion = 1;              // 文件格式版本

// ----------------------------------------------------------------------------
// 将值追加到字节数组（小端序）
// ----------------------------------------------------------------------------
template <typename T>
void append_value(vector<char>& zijie_men, T zhi) {
  const char* yuanshi = reinterpret_cast<const char*>(&zhi);
  zijie_men.insert(zijie_men.end(), yuanshi, yuanshi + sizeof(T));
}

// ----------------------------------------------------------------------------
// 从字节数组读取值（小端序）
// ----------------------------------------------------------------------------
template <typename T>
T read_value(const vector<char>& zijie_men, size_t& pianyi) {
  if (pianyi + sizeof(T) > zijie_men.size()) {
    throw runtime_error("corrupt dat file");
  }

  T zhi {};
  std::memcpy(&zhi, zijie_men.data() + pianyi, sizeof(T));
  pianyi += sizeof(T);
  return zhi;
}

// ----------------------------------------------------------------------------
// 解析 CSV 行
// 处理规则：
// - 支持引号包裹的字段
// - 支持双引号转义（"" 表示 "）
// - 正确处理包含逗号的字段
// ----------------------------------------------------------------------------
vector<string> parse_csv_row(const string& yi_hang) {
  string guifanhou = yi_hang;
  if (!guifanhou.empty() && guifanhou.back() == '\r') {
    guifanhou.pop_back();
  }

  vector<string> zhiduan_men;
  string dangqian_zhiduan;
  bool zai_yinhao_nei = false;

  for (size_t xiabiao = 0; xiabiao < guifanhou.size(); ++xiabiao) {
    char zifu = guifanhou[xiabiao];
    if (zifu == '"') {
      // 检查是否是转义引号 ("")
      if (zai_yinhao_nei && xiabiao + 1 < guifanhou.size() && guifanhou[xiabiao + 1] == '"') {
        dangqian_zhiduan.push_back('"');
        xiabiao += 1;
      } else {
        // 切换引号状态
        zai_yinhao_nei = !zai_yinhao_nei;
      }
    } else if (zifu == ',' && !zai_yinhao_nei) {
      // 字段分隔符（不在引号内）
      zhiduan_men.push_back(dangqian_zhiduan);
      dangqian_zhiduan.clear();
    } else {
      dangqian_zhiduan.push_back(zifu);
    }
  }

  zhiduan_men.push_back(dangqian_zhiduan);
  return zhiduan_men;
}

}  // namespace

// ----------------------------------------------------------------------------
// 从 CSV 文件加载工作簿
// 行号从 1 开始，列号从 1 开始对应 CSV 的第 1 列
// ----------------------------------------------------------------------------
Workbook load_csv(const std::string& lujing) {
  ifstream shuru(lujing);
  if (!shuru) {
    throw runtime_error("failed to open csv");
  }

  Workbook gongzuobu;
  string yi_hang;
  int hang = 1;
  while (std::getline(shuru, yi_hang)) {
    vector<string> zhiduan_men = parse_csv_row(yi_hang);
    for (size_t lie = 0; lie < zhiduan_men.size() && lie < static_cast<size_t>(kMaxColumns); ++lie) {
      if (!zhiduan_men[lie].empty()) {
        gongzuobu.set_cell(to_cell_id({hang, static_cast<int>(lie) + 1}), zhiduan_men[lie]);
      }
    }
    hang += 1;
    if (hang > kMaxRows) {
      break;
    }
  }

  gongzuobu.recalculate_all();
  return gongzuobu;
}

// ----------------------------------------------------------------------------
// 将工作簿序列化为二进制格式
// DAT 文件格式：
// - 4 bytes: 魔数 "MSHT"
// - 4 bytes: 版本号 (uint32)
// - 4 bytes: 单元格数量 (uint32)
// - 每个单元格：
//   - 2 bytes: 行号 (uint16)
//   - 2 bytes: 列号 (uint16)
//   - 1 byte:  类型 (uint8)
//   - 4 bytes: 原始内容长度 (uint32)
//   - N bytes: 原始内容
// ----------------------------------------------------------------------------
vector<char> serialize_workbook(const Workbook& gongzuobu) {
  vector<char> zijie_men;
  zijie_men.insert(zijie_men.end(), kMagic, kMagic + 4);
  append_value<std::uint32_t>(zijie_men, kVersion);

  vector<string> id_men = gongzuobu.ordered_cell_ids();
  append_value<std::uint32_t>(zijie_men, static_cast<std::uint32_t>(id_men.size()));

  for (const string& danyuange_id : id_men) {
    const CellRecord& danyuange = gongzuobu.cell(danyuange_id);
    CellCoord zuobiao = parse_cell_id(danyuange_id);
    append_value<std::uint16_t>(zijie_men, static_cast<std::uint16_t>(zuobiao.hang));
    append_value<std::uint16_t>(zijie_men, static_cast<std::uint16_t>(zuobiao.lie));
    append_value<std::uint8_t>(zijie_men, static_cast<std::uint8_t>(danyuange.leixing));
    append_value<std::uint32_t>(zijie_men, static_cast<std::uint32_t>(danyuange.yuanshi.size()));
    zijie_men.insert(zijie_men.end(), danyuange.yuanshi.begin(), danyuange.yuanshi.end());
  }

  return zijie_men;
}

// ----------------------------------------------------------------------------
// 从二进制数据反序列化工作簿
// ----------------------------------------------------------------------------
Workbook deserialize_workbook(const vector<char>& zijie_men) {
  if (zijie_men.size() < 12) {
    throw runtime_error("dat file too small");
  }

  if (!std::equal(zijie_men.begin(), zijie_men.begin() + 4, kMagic)) {
    throw runtime_error("invalid dat header");
  }

  size_t pianyi = 4;
  std::uint32_t banben = read_value<std::uint32_t>(zijie_men, pianyi);
  if (banben != kVersion) {
    throw runtime_error("unsupported dat version");
  }

  std::uint32_t ge_shu = read_value<std::uint32_t>(zijie_men, pianyi);
  Workbook gongzuobu;
  for (std::uint32_t xuhao = 0; xuhao < ge_shu; ++xuhao) {
    std::uint16_t hang = read_value<std::uint16_t>(zijie_men, pianyi);
    std::uint16_t lie = read_value<std::uint16_t>(zijie_men, pianyi);
    std::uint8_t leixing = read_value<std::uint8_t>(zijie_men, pianyi);
    std::uint32_t yuanshi_changdu = read_value<std::uint32_t>(zijie_men, pianyi);
    if (pianyi + yuanshi_changdu > zijie_men.size()) {
      throw runtime_error("corrupt dat record");
    }

    string yuanshi(zijie_men.data() + pianyi, zijie_men.data() + pianyi + yuanshi_changdu);
    pianyi += yuanshi_changdu;
    (void)leixing;
    gongzuobu.set_cell(to_cell_id({static_cast<int>(hang), static_cast<int>(lie)}), yuanshi);
  }

  gongzuobu.recalculate_all();
  return gongzuobu;
}

// ----------------------------------------------------------------------------
// 保存工作簿到 DAT 文件
// ----------------------------------------------------------------------------
void save_dat(const std::string& lujing, const Workbook& gongzuobu) {
  vector<char> zijie_men = serialize_workbook(gongzuobu);
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open dat for writing");
  }
  shuchu.write(zijie_men.data(), static_cast<std::streamsize>(zijie_men.size()));
}

// ----------------------------------------------------------------------------
// 从 DAT 文件加载工作簿
// ----------------------------------------------------------------------------
Workbook load_dat(const std::string& lujing) {
  ifstream shuru(lujing, ios::binary);
  if (!shuru) {
    throw runtime_error("failed to open dat for reading");
  }

  vector<char> zijie_men((istreambuf_iterator<char>(shuru)), istreambuf_iterator<char>());
  return deserialize_workbook(zijie_men);
}

}  // namespace minisheet
