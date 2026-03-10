#include "minisheet/m6_storage.h"

#include "minisheet/m1_types.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace minisheet {
namespace {

constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};
constexpr std::uint32_t kVersion = 1;

template <typename T>
void append_value(std::vector<char>& bytes, T value) {
  const char* raw = reinterpret_cast<const char*>(&value);
  bytes.insert(bytes.end(), raw, raw + sizeof(T));
}

template <typename T>
T read_value(const std::vector<char>& bytes, std::size_t& offset) {
  if (offset + sizeof(T) > bytes.size()) {
    throw std::runtime_error("corrupt dat file");
  }

  T value {};
  std::memcpy(&value, bytes.data() + offset, sizeof(T));
  offset += sizeof(T);
  return value;
}

std::vector<std::string> parse_csv_row(const std::string& line) {
  std::string normalized = line;
  if (!normalized.empty() && normalized.back() == '\r') {
    normalized.pop_back();
  }

  std::vector<std::string> values;
  std::string current;
  bool in_quotes = false;

  for (std::size_t index = 0; index < normalized.size(); ++index) {
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
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open csv");
  }

  Workbook workbook;
  std::string line;
  int row = 1;
  while (std::getline(input, line)) {
    std::vector<std::string> values = parse_csv_row(line);
    for (std::size_t column = 0; column < values.size() && column < static_cast<std::size_t>(kMaxColumns);
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

std::vector<char> serialize_workbook(const Workbook& workbook) {
  std::vector<char> bytes;
  bytes.insert(bytes.end(), kMagic, kMagic + 4);
  append_value<std::uint32_t>(bytes, kVersion);

  std::vector<std::string> ids = workbook.ordered_cell_ids();
  append_value<std::uint32_t>(bytes, static_cast<std::uint32_t>(ids.size()));

  for (const std::string& id : ids) {
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

Workbook deserialize_workbook(const std::vector<char>& bytes) {
  if (bytes.size() < 12) {
    throw std::runtime_error("dat file too small");
  }

  if (!std::equal(bytes.begin(), bytes.begin() + 4, kMagic)) {
    throw std::runtime_error("invalid dat header");
  }

  std::size_t offset = 4;
  std::uint32_t version = read_value<std::uint32_t>(bytes, offset);
  if (version != kVersion) {
    throw std::runtime_error("unsupported dat version");
  }

  std::uint32_t count = read_value<std::uint32_t>(bytes, offset);
  Workbook workbook;
  for (std::uint32_t index = 0; index < count; ++index) {
    std::uint16_t row = read_value<std::uint16_t>(bytes, offset);
    std::uint16_t column = read_value<std::uint16_t>(bytes, offset);
    std::uint8_t kind = read_value<std::uint8_t>(bytes, offset);
    std::uint32_t raw_size = read_value<std::uint32_t>(bytes, offset);
    if (offset + raw_size > bytes.size()) {
      throw std::runtime_error("corrupt dat record");
    }

    std::string raw(bytes.data() + offset, bytes.data() + offset + raw_size);
    offset += raw_size;
    (void)kind;
    workbook.set_cell(to_cell_id({static_cast<int>(row), static_cast<int>(column)}), raw);
  }

  workbook.recalculate_all();
  return workbook;
}

void save_dat(const std::string& path, const Workbook& workbook) {
  std::vector<char> bytes = serialize_workbook(workbook);
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("failed to open dat for writing");
  }
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

Workbook load_dat(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open dat for reading");
  }

  std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  return deserialize_workbook(bytes);
}

BenchmarkResult run_benchmark(const std::vector<std::string>& csv_paths) {
  BenchmarkResult result;
  if (csv_paths.empty()) {
    return result;
  }

  double total_ms = 0.0;
  double total_efficiency = 0.0;

  int index = 0;
  for (const std::string& path : csv_paths) {
    auto start = std::chrono::steady_clock::now();
    Workbook workbook = load_csv(path);
    auto end = std::chrono::steady_clock::now();

    std::filesystem::path dat_path =
        std::filesystem::temp_directory_path() / ("minisheet_benchmark_" + std::to_string(index) + ".dat");
    save_dat(dat_path.string(), workbook);

    double csv_size = static_cast<double>(std::filesystem::file_size(path));
    double dat_size = static_cast<double>(std::filesystem::file_size(dat_path));
    double elapsed_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count();
    double efficiency = csv_size > 0.0 ? (dat_size / csv_size) * 100.0 : 0.0;

    result.cases.push_back({path, elapsed_ms, csv_size, dat_size});
    total_ms += elapsed_ms;
    total_efficiency += efficiency;
    index += 1;
  }

  result.average_ms = total_ms / static_cast<double>(csv_paths.size());
  result.storage_efficiency_pct = total_efficiency / static_cast<double>(csv_paths.size());
  return result;
}

}  // namespace minisheet
