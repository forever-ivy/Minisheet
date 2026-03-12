#pragma once

#include <string>

using namespace std;

constexpr int kMaxRows = 32767;
constexpr int kMaxColumns = 256;

enum class CellKind {
  Empty,
  Integer,
  Float,
  String,
  Formula,
};

struct CellCoord {
  int hang = 0;
  int lie = 0;
};

struct CellRange {
  CellCoord qishi;
  CellCoord jieshu;
};

string column_index_to_name(int yiji_lie);
int column_name_to_index(const string& lie_ming);
CellCoord parse_cell_id(const string& danyuange_id);
string to_cell_id(const CellCoord& zuobiao);
bool is_valid_coord(const CellCoord& zuobiao);
bool is_valid_cell_id(const string& danyuange_id);
string trim(const string& zhi);
bool try_parse_number(const string& yuanshi, double& zhi, bool& shi_zhengshu);
string format_number(double zhi);
string read_text_file(const string& lujing);
void write_text_file(const string& lujing, const string& neirong);
