#include "minisheet/m2_workbook.h"
#include "minisheet/m6_storage.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <miniz.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};
constexpr uint32_t kExpectedVersion = 3;

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

void expect_true(const string& mingcheng, bool tiaojian, int& shibai_shu) {
  if (!tiaojian) {
    cerr << "FAIL " << mingcheng << '\n';
    shibai_shu += 1;
  }
}

void append_u32(vector<char>& zijie_men, uint32_t zhi) {
  for (int pianyi = 0; pianyi < 4; ++pianyi) {
    zijie_men.push_back(static_cast<char>((zhi >> (pianyi * 8)) & 0xFFu));
  }
}

void overwrite_u32(vector<char>& zijie_men, size_t qidian, uint32_t zhi) {
  if (qidian + 4 > zijie_men.size()) {
    throw runtime_error("测试数据长度不足");
  }

  memcpy(zijie_men.data() + qidian, &zhi, sizeof(zhi));
}

uint32_t read_u32(const vector<char>& zijie_men, size_t qidian) {
  if (qidian + 4 > zijie_men.size()) {
    throw runtime_error("测试数据长度不足");
  }

  uint32_t zhi = 0;
  memcpy(&zhi, zijie_men.data() + qidian, sizeof(zhi));
  return zhi;
}

vector<char> inflate_payload(const vector<char>& yasuobao, uint32_t jiesuya_changdu) {
  vector<char> jieya_hou(jiesuya_changdu);
  uLongf mubiao_changdu = static_cast<uLongf>(jieya_hou.size());
  int jieguo = uncompress(reinterpret_cast<Bytef*>(jieya_hou.data()),
                          &mubiao_changdu,
                          reinterpret_cast<const Bytef*>(yasuobao.data()),
                          static_cast<uLong>(yasuobao.size()));
  if (jieguo != Z_OK) {
    throw runtime_error("zlib解压失败");
  }
  jieya_hou.resize(static_cast<size_t>(mubiao_changdu));
  return jieya_hou;
}

Workbook make_sample_workbook() {
  Workbook gongzuobu;
  gongzuobu.yuan_csv_hang_shu = 64;
  gongzuobu.yuan_csv_lie_shu = 6;

  for (int hang = 1; hang <= 64; ++hang) {
    set_cell(gongzuobu, "A" + to_string(hang), "重复文本重复文本重复文本");
    set_cell(gongzuobu, "B" + to_string(hang), to_string(hang));
    set_cell(gongzuobu, "C" + to_string(hang), "=" + string("B") + to_string(hang) + "*2");
    set_cell(gongzuobu, "D" + to_string(hang), "42.5");
    if (hang % 4 == 0) {
      set_cell(gongzuobu, "F" + to_string(hang), "分组-" + to_string(hang % 8));
    }
  }

  recalculate_all(gongzuobu);
  return gongzuobu;
}

void expect_round_trip(const Workbook& yuanshi, int& shibai_shu) {
  vector<char> dat = serialize_workbook(yuanshi);
  Workbook huifu = deserialize_workbook(dat);

  expect_true("round trip row count", huifu.yuan_csv_hang_shu == yuanshi.yuan_csv_hang_shu,
              shibai_shu);
  expect_true("round trip col count", huifu.yuan_csv_lie_shu == yuanshi.yuan_csv_lie_shu,
              shibai_shu);
  expect_true("round trip A1", cell(huifu, "A1").yuanshi == cell(yuanshi, "A1").yuanshi,
              shibai_shu);
  expect_true("round trip C7 formula", cell(huifu, "C7").yuanshi == "=B7*2", shibai_shu);
  expect_true("round trip C7 display", cell(huifu, "C7").xianshi == cell(yuanshi, "C7").xianshi,
              shibai_shu);
  expect_true("round trip F8 string", cell(huifu, "F8").yuanshi == "分组-0", shibai_shu);
}

}  // namespace

int main() {
  int shibai_shu = 0;

  Workbook gongzuobu = make_sample_workbook();
  vector<char> dat = serialize_workbook(gongzuobu);

  expect_true("magic header", dat.size() >= 12 && equal(dat.begin(), dat.begin() + 4, kMagic),
              shibai_shu);
  expect_true("compressed dat version", dat.size() >= 8 && read_u32(dat, 4) == kExpectedVersion,
              shibai_shu);

  if (dat.size() >= 12 && read_u32(dat, 4) == kExpectedVersion) {
    uint32_t wei_yasuo_changdu = read_u32(dat, 8);
    vector<char> yasuobao(dat.begin() + 12, dat.end());
    vector<char> jieya_hou = inflate_payload(yasuobao, wei_yasuo_changdu);
    expect_true("compressed payload smaller than raw payload", dat.size() < jieya_hou.size() + 12,
                shibai_shu);
  }

  expect_round_trip(gongzuobu, shibai_shu);

  filesystem::path linshi_dat =
      filesystem::temp_directory_path() / "minisheet_storage_compression_test.dat";
  save_dat(linshi_dat.string(), gongzuobu);
  Workbook cong_wenjian_huifu = load_dat(linshi_dat.string());
  expect_true("file round trip A10",
              cell(cong_wenjian_huifu, "A10").yuanshi == cell(gongzuobu, "A10").yuanshi,
              shibai_shu);
  expect_true("file round trip C10 display",
              cell(cong_wenjian_huifu, "C10").xianshi == cell(gongzuobu, "C10").xianshi,
              shibai_shu);
  filesystem::remove(linshi_dat);

  expect_exception_message<runtime_error>(
      "old version rejected",
      "不支持的DAT文件版本",
      [] {
        vector<char> jiuban = {'M', 'S', 'H', 'T'};
        append_u32(jiuban, 2);
        append_u32(jiuban, 0);
        append_u32(jiuban, 0);
        append_u32(jiuban, 0);
        (void)deserialize_workbook(jiuban);
      },
      shibai_shu);

  expect_exception_message<runtime_error>(
      "truncated compressed payload",
      "DAT压缩数据已损坏",
      [&] {
        if (dat.size() < 13) {
          throw runtime_error("测试数据长度不足");
        }
        vector<char> sunhuai(dat.begin(), dat.end() - 1);
        (void)deserialize_workbook(sunhuai);
      },
      shibai_shu);

  expect_exception_message<runtime_error>(
      "wrong uncompressed size",
      "DAT压缩数据已损坏",
      [&] {
        if (dat.size() < 12) {
          throw runtime_error("测试数据长度不足");
        }
        vector<char> sunhuai = dat;
        overwrite_u32(sunhuai, 8, 1);
        (void)deserialize_workbook(sunhuai);
      },
      shibai_shu);

  if (shibai_shu != 0) {
    cerr << "总计失败 " << shibai_shu << " 项" << '\n';
    return 1;
  }

  cout << "DAT压缩测试通过" << '\n';
  return 0;
}
