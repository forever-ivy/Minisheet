#include "minisheet/m1_types.h"
#include "minisheet/m2_workbook.h"
#include "minisheet/m6_storage.h"
#include "minisheet/m7_api.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void expect_true(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_equal(const std::string& actual, const std::string& expected, const std::string& message) {
  if (actual != expected) {
    throw std::runtime_error(message + " expected=" + expected + " actual=" + actual);
  }
}

void expect_close(double actual, double expected, const std::string& message) {
  if (std::fabs(actual - expected) > 1e-9) {
    throw std::runtime_error(message + " expected=" + std::to_string(expected) + " actual=" +
                             std::to_string(actual));
  }
}

}  // namespace

int main() {
  using minisheet::CellKind;
  using minisheet::Workbook;

  expect_equal(minisheet::column_index_to_name(1), "A", "col 1 should be A");
  expect_equal(minisheet::column_index_to_name(27), "AA", "col 27 should be AA");
  expect_equal(minisheet::column_index_to_name(256), "IV", "col 256 should be IV");
  expect_true(minisheet::parse_cell_id("A1").row == 1, "A1 row should be 1");
  expect_true(minisheet::parse_cell_id("IV32767").column == 256, "IV32767 column should be 256");

  Workbook workbook;
  workbook.set_cell("A1", "12");
  workbook.set_cell("A2", "3.5");
  workbook.set_cell("A3", "hello");
  workbook.set_cell("B1", "=1+2*3");
  workbook.set_cell("B2", "=sqrt(9)+abs(-2)");
  workbook.set_cell("B3", "=A1+A2");
  workbook.set_cell("B4", "=sum(A1:B3)");
  workbook.set_cell("B5", "=avg(A1:A2)");
  workbook.set_cell("B6", "=SUM(A1:B3)");
  workbook.set_cell("B7", "=AVG(A1:A2)");
  workbook.set_cell("B8", "=SQRT(9)+ABS(-2)");
  workbook.recalculate_all();

  expect_true(workbook.cell("A1").kind == CellKind::Integer, "A1 should be integer");
  expect_true(workbook.cell("A2").kind == CellKind::Float, "A2 should be float");
  expect_true(workbook.cell("A3").kind == CellKind::String, "A3 should be string");
  expect_equal(workbook.cell("B1").display, "7", "B1 should be 7");
  expect_equal(workbook.cell("B2").display, "5", "B2 should be 5");
  expect_equal(workbook.cell("B3").display, "15.5", "B3 should be 15.5");
  expect_equal(workbook.cell("B4").display, "43", "B4 should be 43");
  expect_equal(workbook.cell("B5").display, "7.75", "B5 should be 7.75");
  expect_equal(workbook.cell("B6").display, "43", "B6 should accept uppercase SUM");
  expect_equal(workbook.cell("B7").display, "7.75", "B7 should accept uppercase AVG");
  expect_equal(workbook.cell("B8").display, "5", "B8 should accept uppercase SQRT/ABS");

  workbook.set_cell("C1", "=A3+1");
  workbook.set_cell("C2", "=1/0");
  workbook.set_cell("C3", "=A99999");
  workbook.set_cell("C4", "=D4");
  workbook.set_cell("D4", "=C4");
  workbook.recalculate_all();
  expect_equal(workbook.cell("C1").display, "#NA", "string arithmetic should fail");
  expect_equal(workbook.cell("C2").display, "#NA", "divide by zero should fail");
  expect_equal(workbook.cell("C3").display, "#NA", "invalid reference should fail");
  expect_equal(workbook.cell("C4").display, "#NA", "cycle should fail");
  expect_equal(workbook.cell("D4").display, "#NA", "cycle peer should fail");

  workbook.set_cell("e1", "99");
  workbook.recalculate_from("e1");
  expect_equal(workbook.cell("E1").display, "99", "lowercase ids should normalize");

  Workbook restored;
  restored.set_cell("Z9", "legacy");
  minisheet::restore_workbook_from_browser_draft(
      restored,
      {
          {"A1", "8"},
          {"B1", "=A1+2"},
          {"C1", "=D1"},
          {"D1", "=C1"},
      });
  expect_true(!restored.has_cell("Z9"), "browser draft restore should replace previous workbook contents");
  expect_equal(restored.cell("A1").display, "8", "restored A1 should keep literal values");
  expect_equal(restored.cell("B1").display, "10", "restored formulas should recalculate");
  expect_equal(restored.cell("C1").display, "#NA", "restored cycles should stay invalid");
  expect_equal(restored.cell("D1").display, "#NA", "restored cycle peers should stay invalid");

  std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
  std::filesystem::path dat_path = temp_dir / "minisheet_test.dat";
  std::filesystem::path csv1 = temp_dir / "minisheet_case_1.csv";
  std::filesystem::path csv2 = temp_dir / "minisheet_case_2.csv";
  std::filesystem::path csv3 = temp_dir / "minisheet_case_3.csv";

  minisheet::save_dat(dat_path.string(), workbook);
  Workbook reloaded = minisheet::load_dat(dat_path.string());
  reloaded.recalculate_all();
  expect_equal(reloaded.cell("B4").display, "43", "dat reload should keep formulas");
  expect_equal(reloaded.cell("C4").display, "#NA", "dat reload should keep errors");

  minisheet::write_text_file(csv1.string(), "1,2,=A1+B1\n");
  minisheet::write_text_file(csv2.string(), "4,5,=sum(A1:B1)\n");
  minisheet::write_text_file(csv3.string(), "9,=sqrt(A1),=avg(A1:B1)\n");
  minisheet::BenchmarkResult result =
      minisheet::run_benchmark({csv1.string(), csv2.string(), csv3.string()});
  expect_true(result.average_ms >= 0.0, "average_ms should be non-negative");
  expect_true(result.storage_efficiency_pct > 0.0, "storage_efficiency_pct should be positive");

  std::cout << "core_tests passed\n";
  return 0;
}
