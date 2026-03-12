#include "minisheet/m1_types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace std;

string column_index_to_name(int yiji_lie) {
  if (yiji_lie < 1 || yiji_lie > kMaxColumns) {
    throw invalid_argument("column out of range");
  }

  string lie_ming;
  int lie_zhi = yiji_lie;
  while (lie_zhi > 0) {
    lie_zhi -= 1;
    lie_ming.push_back(static_cast<char>('A' + (lie_zhi % 26)));
    lie_zhi /= 26;
  }
  reverse(lie_ming.begin(), lie_ming.end());
  return lie_ming;
}

int column_name_to_index(const string& lie_ming) {
  if (lie_ming.empty()) {
    throw invalid_argument("empty column name");
  }

  int lie_zhi = 0;
  for (char zifu : lie_ming) {
    if (!isalpha(static_cast<unsigned char>(zifu))) {
      throw invalid_argument("invalid column name");
    }
    lie_zhi = lie_zhi * 26 + (toupper(static_cast<unsigned char>(zifu)) - 'A' + 1);
  }

  if (lie_zhi < 1 || lie_zhi > kMaxColumns) {
    throw invalid_argument("column out of range");
  }
  return lie_zhi;
}

CellCoord parse_cell_id(const string& danyuange_id) {
  string lie_ming;
  string hang_haoma;

  for (char zifu : danyuange_id) {
    if (isalpha(static_cast<unsigned char>(zifu)) && hang_haoma.empty()) {
      lie_ming.push_back(static_cast<char>(toupper(static_cast<unsigned char>(zifu))));
    } else if (isdigit(static_cast<unsigned char>(zifu))) {
      hang_haoma.push_back(zifu);
    } else {
      throw invalid_argument("invalid cell id");
    }
  }

  if (lie_ming.empty() || hang_haoma.empty()) {
    throw invalid_argument("invalid cell id");
  }

  CellCoord zuobiao {stoi(hang_haoma), column_name_to_index(lie_ming)};
  if (!is_valid_coord(zuobiao)) {
    throw invalid_argument("cell id out of range");
  }
  return zuobiao;
}

string to_cell_id(const CellCoord& zuobiao) {
  if (!is_valid_coord(zuobiao)) {
    throw invalid_argument("coord out of range");
  }
  return column_index_to_name(zuobiao.lie) + to_string(zuobiao.hang);
}

bool is_valid_coord(const CellCoord& zuobiao) {
  return zuobiao.hang >= 1 && zuobiao.hang <= kMaxRows && zuobiao.lie >= 1 &&
         zuobiao.lie <= kMaxColumns;
}

bool is_valid_cell_id(const string& danyuange_id) {
  try {
    (void)parse_cell_id(danyuange_id);
    return true;
  } catch (...) {
    return false;
  }
}

string trim(const string& zhi) {
  size_t qidian = 0;
  while (qidian < zhi.size() && isspace(static_cast<unsigned char>(zhi[qidian]))) {
    qidian += 1;
  }

  size_t zhongdian = zhi.size();
  while (zhongdian > qidian && isspace(static_cast<unsigned char>(zhi[zhongdian - 1]))) {
    zhongdian -= 1;
  }

  return zhi.substr(qidian, zhongdian - qidian);
}

bool try_parse_number(const string& yuanshi, double& zhi, bool& shi_zhengshu) {
  string houxuan = trim(yuanshi);
  if (houxuan.empty()) {
    return false;
  }

  char* jieshu_zhizhen = nullptr;
  zhi = strtod(houxuan.c_str(), &jieshu_zhizhen);
  if (jieshu_zhizhen == nullptr || *jieshu_zhizhen != '\0') {
    return false;
  }

  shi_zhengshu = houxuan.find_first_of(".eE") == string::npos;
  return true;
}

string format_number(double zhi) {
  if (!isfinite(zhi)) {
    return "#NA";
  }

  double quzheng_zhi = round(zhi);
  if (fabs(zhi - quzheng_zhi) < 1e-9) {
    return to_string(static_cast<long long>(quzheng_zhi));
  }

  ostringstream liu;
  liu << fixed << setprecision(10) << zhi;
  string wenben = liu.str();
  while (!wenben.empty() && wenben.back() == '0') {
    wenben.pop_back();
  }
  if (!wenben.empty() && wenben.back() == '.') {
    wenben.pop_back();
  }
  return wenben;
}

string read_text_file(const string& lujing) {
  ifstream shuru(lujing, ios::binary);
  if (!shuru) {
    throw runtime_error("failed to open file for reading");
  }

  ostringstream huanchong;
  huanchong << shuru.rdbuf();
  return huanchong.str();
}

void write_text_file(const string& lujing, const string& neirong) {
  filesystem::path wenjian_lujing(lujing);
  if (!wenjian_lujing.parent_path().empty()) {
    filesystem::create_directories(wenjian_lujing.parent_path());
  }

  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open file for writing");
  }
  shuchu << neirong;
}
