#pragma once

#include <string>

namespace minisheet {

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
  int row = 0;
  int column = 0;
};

struct CellRange {
  CellCoord start;
  CellCoord end;
};

std::string column_index_to_name(int one_based_column);
int column_name_to_index(const std::string& column_name);
CellCoord parse_cell_id(const std::string& cell_id);
std::string to_cell_id(const CellCoord& coord);
bool is_valid_coord(const CellCoord& coord);
bool is_valid_cell_id(const std::string& cell_id);
std::string trim(const std::string& value);
bool try_parse_number(const std::string& raw, double& value, bool& is_integer);
std::string format_number(double value);
std::string read_text_file(const std::string& path);
void write_text_file(const std::string& path, const std::string& content);

}  // namespace minisheet
