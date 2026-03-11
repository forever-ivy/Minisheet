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

constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};
constexpr std::uint32_t kVersion = 1;

template <typename T>
void append_value(vector<char>& bytes, T value) {
  const char* raw = reinterpret_cast<const char*>(&value);
  bytes.insert(bytes.end(), raw, raw + sizeof(T));
}

template <typename T>
T read_value(const vector<char>& bytes, size_t& offset) {
  if (offset + sizeof(T) > bytes.size()) {
    throw runtime_error("corrupt dat file");
  }

  T value {};
  std::memcpy(&value, bytes.data() + offset, sizeof(T));
  offset += sizeof(T);
  return value;
}

vector<string> parse_csv_row(const string& line) {
  string normalized = line;
  if (!normalized.empty() && normalized.back() == '\r') {
    normalized.pop_back();
  }

  vector<string> values;
  string current;
  bool in_quotes = false;

  for (size_t index = 0; index < normalized.size(); ++index) {
    char ch = normalized[index];
    if (ch == '"') {
      if (in_quotes && index + 1 < normalized.size() && normalized[index + 1] == '"') {
        current.push_back('"');
        index += 1;
      } else {
        in_quotes = !in_quotes;
      }
    } else if (ch == ',' && !in_quotes) {
      values.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }

  values.push_back(current);
  return values;
}

}  // namespace

Workbook load_csv(const std::string& path) {
  ifstream input(path);
  if (!input) {
    throw runtime_error("failed to open csv");
  }

  Workbook workbook;
  string line;
  int row = 1;
  while (std::getline(input, line)) {
    vector<string> values = parse_csv_row(line);
    for (size_t column = 0; column < values.size() && column < static_cast<size_t>(kMaxColumns);
         ++column) {
      if (!values[column].empty()) {
        workbook.set_cell(to_cell_id({row, static_cast<int>(column) + 1}), values[column]);
      }
    }
    row += 1;
    if (row > kMaxRows) {
      break;
    }
  }

  workbook.recalculate_all();
  return workbook;
}

vector<char> serialize_workbook(const Workbook& workbook) {
  vector<char> bytes;
  bytes.insert(bytes.end(), kMagic, kMagic + 4);
  append_value<std::uint32_t>(bytes, kVersion);

  vector<string> ids = workbook.ordered_cell_ids();
  append_value<std::uint32_t>(bytes, static_cast<std::uint32_t>(ids.size()));

  for (const string& id : ids) {
    const CellRecord& cell = workbook.cell(id);
    CellCoord coord = parse_cell_id(id);
    append_value<std::uint16_t>(bytes, static_cast<std::uint16_t>(coord.row));
    append_value<std::uint16_t>(bytes, static_cast<std::uint16_t>(coord.column));
    append_value<std::uint8_t>(bytes, static_cast<std::uint8_t>(cell.kind));
    append_value<std::uint32_t>(bytes, static_cast<std::uint32_t>(cell.raw.size()));
    bytes.insert(bytes.end(), cell.raw.begin(), cell.raw.end());
  }

  return bytes;
}

Workbook deserialize_workbook(const vector<char>& bytes) {
  if (bytes.size() < 12) {
    throw runtime_error("dat file too small");
  }

  if (!std::equal(bytes.begin(), bytes.begin() + 4, kMagic)) {
    throw runtime_error("invalid dat header");
  }

  size_t offset = 4;
  std::uint32_t version = read_value<std::uint32_t>(bytes, offset);
  if (version != kVersion) {
    throw runtime_error("unsupported dat version");
  }

  std::uint32_t count = read_value<std::uint32_t>(bytes, offset);
  Workbook workbook;
  for (std::uint32_t index = 0; index < count; ++index) {
    std::uint16_t row = read_value<std::uint16_t>(bytes, offset);
    std::uint16_t column = read_value<std::uint16_t>(bytes, offset);
    std::uint8_t kind = read_value<std::uint8_t>(bytes, offset);
    std::uint32_t raw_size = read_value<std::uint32_t>(bytes, offset);
    if (offset + raw_size > bytes.size()) {
      throw runtime_error("corrupt dat record");
    }

    string raw(bytes.data() + offset, bytes.data() + offset + raw_size);
    offset += raw_size;
    (void)kind;
    workbook.set_cell(to_cell_id({static_cast<int>(row), static_cast<int>(column)}), raw);
  }

  workbook.recalculate_all();
  return workbook;
}

void save_dat(const std::string& path, const Workbook& workbook) {
  vector<char> bytes = serialize_workbook(workbook);
  ofstream output(path, ios::binary);
  if (!output) {
    throw runtime_error("failed to open dat for writing");
  }
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

Workbook load_dat(const std::string& path) {
  ifstream input(path, ios::binary);
  if (!input) {
    throw runtime_error("failed to open dat for reading");
  }

  vector<char> bytes((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
  return deserialize_workbook(bytes);
}

}  // namespace minisheet
