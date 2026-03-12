#include "minisheet/m6_storage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace std;

string read_all(const string& lujing) {
  ifstream shuru(lujing, ios::binary);
  return string((istreambuf_iterator<char>(shuru)), istreambuf_iterator<char>());
}

int main() {
  const string mubiao_mulu = "backend/build-gcc/storage-roundtrip-test";
  filesystem::create_directories(mubiao_mulu);

  const string csv_lujing = mubiao_mulu + "/input.csv";
  const string dat_lujing = mubiao_mulu + "/sheet.dat";
  const string huanyuan_csv_lujing = mubiao_mulu + "/roundtrip.csv";

  {
    ofstream shuchu(csv_lujing, ios::binary);
    shuchu << "1,,=A1+2\n";
    shuchu << ",text,\"x,y\"\n";
  }

  Workbook gongzuobu = load_csv(csv_lujing);
  save_dat(dat_lujing, gongzuobu);
  Workbook huanyuan = load_dat(dat_lujing);
  save_csv(huanyuan_csv_lujing, huanyuan);

  const string yuanshi = read_all(csv_lujing);
  const string huanyuan_jieguo = read_all(huanyuan_csv_lujing);

  if (yuanshi != huanyuan_jieguo) {
    cerr << "roundtrip mismatch\n";
    cerr << "expected:\n" << yuanshi << "\n";
    cerr << "actual:\n" << huanyuan_jieguo << "\n";
    return 1;
  }

  cout << "storage_roundtrip ok\n";
  return 0;
}
