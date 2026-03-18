#include "oj_cli.h"

#include "minisheet/m2_workbook.h"

#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace {

vector<string> split_csv_line(const string& line, int columns) {
  vector<string> tokens;
  tokens.reserve(static_cast<size_t>(columns));

  string token;
  for (char ch : line) {
    if (ch == ',') {
      tokens.push_back(token);
      token.clear();
    } else {
      token.push_back(ch);
    }
  }
  tokens.push_back(token);

  if (static_cast<int>(tokens.size()) < columns) {
    tokens.resize(static_cast<size_t>(columns));
  } else if (static_cast<int>(tokens.size()) > columns) {
    string merged = tokens[static_cast<size_t>(columns - 1)];
    for (size_t i = static_cast<size_t>(columns); i < tokens.size(); ++i) {
      merged.push_back(',');
      merged += tokens[i];
    }
    tokens.resize(static_cast<size_t>(columns));
    tokens[static_cast<size_t>(columns - 1)] = merged;
  }

  return tokens;
}

double oj_value_for_cell(const Workbook& workbook, const string& cell_id) {
  if (!has_cell(workbook, cell_id)) {
    return 0.0;
  }

  const CellRecord& record = cell(workbook, cell_id);
  if ((record.leixing == CellKind::Integer || record.leixing == CellKind::Float ||
       record.leixing == CellKind::Formula) &&
      record.you_shuzhi && isfinite(record.shuzhi)) {
    return record.shuzhi;
  }

  return 0.0;
}

}  // namespace

int run_oj_cli(istream& input, ostream& output, ostream& error) {
  int rows = 0;
  int columns = 0;
  if (!(input >> rows >> columns)) {
    return 0;
  }

  string ignored;
  getline(input, ignored);

  if (rows < 0 || rows > 100 || columns < 0 || columns > 26) {
    error << "invalid grid size\n";
    return 1;
  }

  Workbook workbook;
  workbook.yuan_csv_hang_shu = rows;
  workbook.yuan_csv_lie_shu = columns;

  for (int row = 0; row < rows; ++row) {
    string line;
    if (!getline(input, line)) {
      line.clear();
    }

    vector<string> tokens = split_csv_line(line, columns);
    for (int column = 0; column < columns; ++column) {
      const string raw_value = trim(tokens[static_cast<size_t>(column)]);
      if (raw_value.empty()) {
        continue;
      }
      set_cell(workbook, to_cell_id({row + 1, column + 1}), raw_value);
    }
  }

  recalculate_all(workbook);

  output << fixed << setprecision(2);
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      double value = oj_value_for_cell(workbook, to_cell_id({row + 1, column + 1}));
      if (fabs(value) < 1e-10 || !isfinite(value)) {
        value = 0.0;
      }
      output << value << ' ';
    }
    output << '\n';
  }

  return 0;
}

int main() {
  try {
    return run_oj_cli(cin, cout, cerr);
  } catch (const exception& error) {
    cerr << "myxls error: " << error.what() << '\n';
    return 1;
  } catch (...) {
    cerr << "myxls error: unknown exception\n";
    return 1;
  }
}
