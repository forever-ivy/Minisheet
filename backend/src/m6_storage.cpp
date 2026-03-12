#include "minisheet/m6_storage.h"

#include "minisheet/m1_types.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace std;

constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};
constexpr uint32_t kVersion = 2;

template <typename T>
void append_value(vector<char>& zijie_men, T zhi) {
  const char* yuanshi = reinterpret_cast<const char*>(&zhi);
  zijie_men.insert(zijie_men.end(), yuanshi, yuanshi + sizeof(T));
}

template <typename T>
T read_value(const vector<char>& zijie_men, size_t& pianyi) {
  if (pianyi + sizeof(T) > zijie_men.size()) {
    throw runtime_error("corrupt dat file");
  }

  T zhi {};
  memcpy(&zhi, zijie_men.data() + pianyi, sizeof(T));
  pianyi += sizeof(T);
  return zhi;
}

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
      if (zai_yinhao_nei && xiabiao + 1 < guifanhou.size() && guifanhou[xiabiao + 1] == '"') {
        dangqian_zhiduan.push_back('"');
        xiabiao += 1;
      } else {
        zai_yinhao_nei = !zai_yinhao_nei;
      }
    } else if (zifu == ',' && !zai_yinhao_nei) {
      zhiduan_men.push_back(dangqian_zhiduan);
      dangqian_zhiduan.clear();
    } else {
      dangqian_zhiduan.push_back(zifu);
    }
  }

  zhiduan_men.push_back(dangqian_zhiduan);
  return zhiduan_men;
}

Workbook load_csv(const string& lujing) {
  ifstream shuru(lujing);
  if (!shuru) {
    throw runtime_error("failed to open csv");
  }

  Workbook gongzuobu;
  string yi_hang;
  int hang = 1;
  int zui_da_lie = 0;
  while (getline(shuru, yi_hang)) {
    vector<string> zhiduan_men = parse_csv_row(yi_hang);
    zui_da_lie = max(zui_da_lie, static_cast<int>(zhiduan_men.size()));
    for (size_t lie = 0; lie < zhiduan_men.size() && lie < static_cast<size_t>(kMaxColumns); ++lie) {
      if (!zhiduan_men[lie].empty()) {
        set_cell(gongzuobu, to_cell_id({hang, static_cast<int>(lie) + 1}), zhiduan_men[lie]);
      }
    }

    hang += 1;
    if (hang > kMaxRows) {
      break;
    }
  }

  gongzuobu.yuan_csv_hang_shu = min(hang - 1, kMaxRows);
  gongzuobu.yuan_csv_lie_shu = min(zui_da_lie, kMaxColumns);
  recalculate_all(gongzuobu);
  return gongzuobu;
}

string escape_csv_field(const string& yuanshi) {
  bool xuyao_yinhao = false;
  for (char zifu : yuanshi) {
    if (zifu == ',' || zifu == '"' || zifu == '\n' || zifu == '\r') {
      xuyao_yinhao = true;
      break;
    }
  }

  if (!xuyao_yinhao) {
    return yuanshi;
  }

  string zhuanyi = "\"";
  for (char zifu : yuanshi) {
    if (zifu == '"') {
      zhuanyi += "\"\"";
    } else {
      zhuanyi.push_back(zifu);
    }
  }
  zhuanyi.push_back('"');
  return zhuanyi;
}

pair<int, int> workbook_shape(const Workbook& gongzuobu) {
  int hang_shu = gongzuobu.yuan_csv_hang_shu;
  int lie_shu = gongzuobu.yuan_csv_lie_shu;

  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    CellCoord zuobiao = parse_cell_id(danyuange_id);
    hang_shu = max(hang_shu, zuobiao.hang);
    lie_shu = max(lie_shu, zuobiao.lie);
  }

  return {hang_shu, lie_shu};
}

void save_csv(const string& lujing, const Workbook& gongzuobu) {
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open csv for writing");
  }

  pair<int, int> chicun = workbook_shape(gongzuobu);
  for (int hang = 1; hang <= chicun.first; ++hang) {
    for (int lie = 1; lie <= chicun.second; ++lie) {
      if (lie > 1) {
        shuchu << ',';
      }

      const CellRecord& danyuange = cell(gongzuobu, to_cell_id({hang, lie}));
      shuchu << escape_csv_field(danyuange.yuanshi);
    }
    shuchu << '\n';
  }
}

vector<char> serialize_workbook(const Workbook& gongzuobu) {
  vector<char> zijie_men;
  zijie_men.insert(zijie_men.end(), kMagic, kMagic + 4);
  append_value<uint32_t>(zijie_men, kVersion);
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(max(gongzuobu.yuan_csv_hang_shu, 0)));
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(max(gongzuobu.yuan_csv_lie_shu, 0)));

  vector<string> id_men = ordered_cell_ids(gongzuobu);
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(id_men.size()));

  for (const string& danyuange_id : id_men) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);
    CellCoord zuobiao = parse_cell_id(danyuange_id);
    append_value<uint16_t>(zijie_men, static_cast<uint16_t>(zuobiao.hang));
    append_value<uint16_t>(zijie_men, static_cast<uint16_t>(zuobiao.lie));
    append_value<uint8_t>(zijie_men, static_cast<uint8_t>(danyuange.leixing));
    append_value<uint32_t>(zijie_men, static_cast<uint32_t>(danyuange.yuanshi.size()));
    zijie_men.insert(zijie_men.end(), danyuange.yuanshi.begin(), danyuange.yuanshi.end());
  }

  return zijie_men;
}

Workbook deserialize_workbook(const vector<char>& zijie_men) {
  if (zijie_men.size() < 12) {
    throw runtime_error("dat file too small");
  }
  if (!equal(zijie_men.begin(), zijie_men.begin() + 4, kMagic)) {
    throw runtime_error("invalid dat header");
  }

  size_t pianyi = 4;
  uint32_t banben = read_value<uint32_t>(zijie_men, pianyi);
  if (banben != 1 && banben != kVersion) {
    throw runtime_error("unsupported dat version");
  }

  Workbook gongzuobu;
  if (banben >= 2) {
    gongzuobu.yuan_csv_hang_shu = static_cast<int>(read_value<uint32_t>(zijie_men, pianyi));
    gongzuobu.yuan_csv_lie_shu = static_cast<int>(read_value<uint32_t>(zijie_men, pianyi));
  }

  uint32_t ge_shu = read_value<uint32_t>(zijie_men, pianyi);
  for (uint32_t xuhao = 0; xuhao < ge_shu; ++xuhao) {
    uint16_t hang = read_value<uint16_t>(zijie_men, pianyi);
    uint16_t lie = read_value<uint16_t>(zijie_men, pianyi);
    uint8_t leixing = read_value<uint8_t>(zijie_men, pianyi);
    uint32_t yuanshi_changdu = read_value<uint32_t>(zijie_men, pianyi);
    if (pianyi + yuanshi_changdu > zijie_men.size()) {
      throw runtime_error("corrupt dat record");
    }

    string yuanshi(zijie_men.data() + pianyi, zijie_men.data() + pianyi + yuanshi_changdu);
    pianyi += yuanshi_changdu;
    (void)leixing;
    set_cell(gongzuobu, to_cell_id({static_cast<int>(hang), static_cast<int>(lie)}), yuanshi);
  }

  recalculate_all(gongzuobu);
  return gongzuobu;
}

void save_dat(const string& lujing, const Workbook& gongzuobu) {
  vector<char> zijie_men = serialize_workbook(gongzuobu);
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open dat for writing");
  }
  shuchu.write(zijie_men.data(), static_cast<streamsize>(zijie_men.size()));
}

Workbook load_dat(const string& lujing) {
  ifstream shuru(lujing, ios::binary);
  if (!shuru) {
    throw runtime_error("failed to open dat for reading");
  }

  vector<char> zijie_men((istreambuf_iterator<char>(shuru)), istreambuf_iterator<char>());
  return deserialize_workbook(zijie_men);
}
