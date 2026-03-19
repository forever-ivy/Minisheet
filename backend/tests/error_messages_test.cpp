#include "minisheet/m1_types.h"
#include "minisheet/m6_storage.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

template <typename ExceptionType>
void expect_exception_message(const string& mingcheng,
                              const string& qiwang_xiaoxi,
                              const function<void()>& dongzuo,
                              int& shibai_shu) {
  try {
    dongzuo();
    cerr << "FAIL " << mingcheng << ": 没有抛出异常" << '\n';
    shibai_shu += 1;
  } catch (const ExceptionType& cuowu) {
    if (string(cuowu.what()) != qiwang_xiaoxi) {
      cerr << "FAIL " << mingcheng << ": 期望 \"" << qiwang_xiaoxi << "\"，实际是 \""
           << cuowu.what() << "\"" << '\n';
      shibai_shu += 1;
    }
  } catch (const exception& cuowu) {
    cerr << "FAIL " << mingcheng << ": 异常类型不对，实际消息是 \"" << cuowu.what() << "\""
         << '\n';
    shibai_shu += 1;
  }
}

void append_u32(vector<char>& zijie_men, uint32_t zhi) {
  for (int pianyi = 0; pianyi < 4; ++pianyi) {
    zijie_men.push_back(static_cast<char>((zhi >> (pianyi * 8)) & 0xFFu));
  }
}

void append_u16(vector<char>& zijie_men, uint16_t zhi) {
  for (int pianyi = 0; pianyi < 2; ++pianyi) {
    zijie_men.push_back(static_cast<char>((zhi >> (pianyi * 8)) & 0xFFu));
  }
}

void append_u8(vector<char>& zijie_men, uint8_t zhi) {
  zijie_men.push_back(static_cast<char>(zhi));
}

vector<char> make_dat_header(uint32_t banben,
                             uint32_t hang_shu = 0,
                             uint32_t lie_shu = 0,
                             uint32_t danyuange_shu = 0) {
  vector<char> zijie_men = {'M', 'S', 'H', 'T'};
  append_u32(zijie_men, banben);
  append_u32(zijie_men, hang_shu);
  append_u32(zijie_men, lie_shu);
  append_u32(zijie_men, danyuange_shu);
  return zijie_men;
}

}  // namespace

int main() {
  int shibai_shu = 0;

  const filesystem::path linshi_mulu = filesystem::temp_directory_path();
  const filesystem::path bucunzai_wenjian =
      linshi_mulu / "minisheet_error_messages_missing_file.tmp";
  filesystem::remove(bucunzai_wenjian);

  expect_exception_message<invalid_argument>(
      "column_index_to_name", "列号超出范围", [] { (void)column_index_to_name(0); }, shibai_shu);
  expect_exception_message<invalid_argument>(
      "column_name_to_index empty", "列名不能为空", [] { (void)column_name_to_index(""); },
      shibai_shu);
  expect_exception_message<invalid_argument>(
      "column_name_to_index invalid", "列名不合法", [] { (void)column_name_to_index("A1"); },
      shibai_shu);
  expect_exception_message<invalid_argument>(
      "parse_cell_id invalid", "单元格ID不合法", [] { (void)parse_cell_id("1A"); }, shibai_shu);
  expect_exception_message<invalid_argument>(
      "parse_cell_id column out of range", "列号超出范围",
      [] { (void)parse_cell_id("IW1"); }, shibai_shu);
  expect_exception_message<invalid_argument>(
      "parse_cell_id coord out of range", "单元格ID超出范围",
      [] { (void)parse_cell_id("A0"); }, shibai_shu);
  expect_exception_message<invalid_argument>(
      "to_cell_id", "单元格坐标超出范围", [] { (void)to_cell_id({0, 1}); }, shibai_shu);
  expect_exception_message<runtime_error>(
      "read_text_file", "打开文件读取失败",
      [&] { (void)read_text_file(bucunzai_wenjian.string()); }, shibai_shu);
  expect_exception_message<runtime_error>(
      "write_text_file", "打开文件写入失败",
      [&] { write_text_file(linshi_mulu.string(), "abc"); }, shibai_shu);

  expect_exception_message<runtime_error>(
      "load_csv", "打开CSV文件失败", [&] { (void)load_csv(bucunzai_wenjian.string()); },
      shibai_shu);
  expect_exception_message<runtime_error>(
      "save_csv", "打开CSV文件写入失败", [&] { save_csv(linshi_mulu.string(), Workbook {}); },
      shibai_shu);

  expect_exception_message<runtime_error>(
      "deserialize_workbook too small", "DAT文件过小",
      [] { (void)deserialize_workbook({}); }, shibai_shu);
  expect_exception_message<runtime_error>(
      "deserialize_workbook invalid header", "DAT文件头无效",
      [] {
        vector<char> zijie_men = {'B', 'A', 'D', '!'};
        append_u32(zijie_men, 2);
        append_u32(zijie_men, 0);
        (void)deserialize_workbook(zijie_men);
      },
      shibai_shu);
  expect_exception_message<runtime_error>(
      "deserialize_workbook unsupported version", "不支持的DAT文件版本",
      [] { (void)deserialize_workbook(make_dat_header(99)); }, shibai_shu);
  expect_exception_message<runtime_error>(
      "deserialize_workbook corrupt file", "DAT文件已损坏",
      [] {
        vector<char> zijie_men = {'M', 'S', 'H', 'T'};
        append_u32(zijie_men, 2);
        append_u32(zijie_men, 0);
        (void)deserialize_workbook(zijie_men);
      },
      shibai_shu);
  expect_exception_message<runtime_error>(
      "deserialize_workbook corrupt record", "DAT记录已损坏",
      [] {
        vector<char> zijie_men = make_dat_header(2, 0, 0, 1);
        append_u16(zijie_men, 1);
        append_u16(zijie_men, 1);
        append_u8(zijie_men, 0);
        append_u32(zijie_men, 5);
        zijie_men.push_back('a');
        zijie_men.push_back('b');
        zijie_men.push_back('c');
        (void)deserialize_workbook(zijie_men);
      },
      shibai_shu);

  expect_exception_message<runtime_error>(
      "save_dat", "打开DAT文件写入失败", [&] { save_dat(linshi_mulu.string(), Workbook {}); },
      shibai_shu);
  expect_exception_message<runtime_error>(
      "load_dat", "打开DAT文件读取失败", [&] { (void)load_dat(bucunzai_wenjian.string()); },
      shibai_shu);

  if (shibai_shu != 0) {
    cerr << "总计失败 " << shibai_shu << " 项" << '\n';
    return 1;
  }

  cout << "全部异常文案测试通过" << '\n';
  return 0;
}
