#pragma once

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

using namespace std;

Workbook load_csv(const string& lujing);
void save_csv(const string& lujing, const Workbook& gongzuobu);
vector<char> serialize_workbook(const Workbook& gongzuobu);
Workbook deserialize_workbook(const vector<char>& zijie_men);
void save_dat(const string& lujing, const Workbook& gongzuobu);
Workbook load_dat(const string& lujing);
