#pragma once

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

using namespace std;

struct FormulaEvalResult {
  bool chenggong = false;
  double shuzhi = 0.0;
};

bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,
                           vector<string>& yiwancheng,
                           double& shuzhi);
bool evaluate_range_numeric(Workbook& gongzuobu,
                            const CellRange& fanwei,
                            bool shi_pingjun,
                            vector<string>& fangwen_zhong,
                            vector<string>& yiwancheng,
                            double& shuzhi);
FormulaEvalResult evaluate_formula(Workbook& gongzuobu,
                                   const string& gongshi,
                                   vector<string>& fangwen_zhong,
                                   vector<string>& yiwancheng);
