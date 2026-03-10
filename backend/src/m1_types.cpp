#include "minisheet/m1_types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace minisheet {

std::string column_index_to_name(int one_based_column) {
  if (one_based_column < 1 || one_based_column > kMaxColumns) {
    throw std::invalid_argument("column out of range");
  }

  std::string name;
  int value = one_based_column;
  while (value > 0) {
    value -= 1;
    name.push_back(static_cast<char>('A' + (value % 26)));
    value /= 26;
  }
  std::reverse(name.begin(), name.end());
  return name;
}

int column_name_to_index(const std::string& column_name) {
  if (column_name.empty()) {
    throw std::invalid_argument("empty column name");
  }

  int value = 0;
  for (char ch : column_name) {
    if (!std::isalpha(static_cast<unsigned char>(ch))) {
      throw std::invalid_argument("invalid column name");
    }
    value = value * 26 + (std::toupper(static_cast<unsigned char>(ch)) - 'A' + 1);
  }

  if (value < 1 || value > kMaxColumns) {
    throw std::invalid_argument("column out of range");
  }
  return value;
}

CellCoord parse_cell_id(const std::string& cell_id) {
  std::string column_name;
  std::string row_name;

  for (char ch : cell_id) {
    if (std::isalpha(static_cast<unsigned char>(ch)) && row_name.empty()) {
      column_name.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if (std::isdigit(static_cast<unsigned char>(ch))) {
      row_name.push_back(ch);
    } else {
      throw std::invalid_argument("invalid cell id");
    }
  }

  if (column_name.empty() || row_name.empty()) {
    throw std::invalid_argument("invalid cell id");
  }

  CellCoord coord {std::stoi(row_name), column_name_to_index(column_name)};
  if (!is_valid_coord(coord)) {
    throw std::invalid_argument("cell id out of range");
  }
  return coord;
}

std::string to_cell_id(const CellCoord& coord) {
  if (!is_valid_coord(coord)) {
    throw std::invalid_argument("coord out of range");
  }
  return column_index_to_name(coord.column) + std::to_string(coord.row);
}

bool is_valid_coord(const CellCoord& coord) {
  return coord.row >= 1 && coord.row <= kMaxRows && coord.column >= 1 && coord.column <= kMaxColumns;
}

bool is_valid_cell_id(const std::string& cell_id) {
  try {
    (void)parse_cell_id(cell_id);
    return true;
  } catch (...) {
    return false;
  }
}

std::string trim(const std::string& value) {
  std::size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    start += 1;
  }

  std::size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end -= 1;
  }

  return value.substr(start, end - start);
}

bool try_parse_number(const std::string& raw, double& value, bool& is_integer) {
  std::string candidate = trim(raw);
  if (candidate.empty()) {
    return false;
  }

  char* end_ptr = nullptr;
  value = std::strtod(candidate.c_str(), &end_ptr);
  if (end_ptr == nullptr || *end_ptr != '\0') {
    return false;
  }

  is_integer = candidate.find_first_of(".eE") == std::string::npos;
  return true;
}

std::string format_number(double value) {
  if (!std::isfinite(value)) {
    return "#NA";
  }

  double rounded = std::round(value);
  if (std::fabs(value - rounded) < 1e-9) {
    return std::to_string(static_cast<long long>(rounded));
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(10) << value;
  std::string text = stream.str();
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

std::string read_text_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open file for reading");
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void write_text_file(const std::string& path, const std::string& content) {
  std::filesystem::path file_path(path);
  if (!file_path.parent_path().empty()) {
    std::filesystem::create_directories(file_path.parent_path());
  }

  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("failed to open file for writing");
  }
  output << content;
}

}  // namespace minisheet
