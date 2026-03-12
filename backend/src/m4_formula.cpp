#include "minisheet/m4_formula.h"

#include "../third_party/tinyexpr/tinyexpr.h"

#include <cctype>
#include <cmath>
#include <vector>

using namespace std;

string uppercase(string wenben) {
  for (char& zifu : wenben) {
    zifu = static_cast<char>(toupper(static_cast<unsigned char>(zifu)));
  }
  return wenben;
}

string lowercase(string wenben) {
  for (char& zifu : wenben) {
    zifu = static_cast<char>(tolower(static_cast<unsigned char>(zifu)));
  }
  return wenben;
}

string strip_formula_prefix(const string& gongshi) {
  string wenben = trim(gongshi);
  if (!wenben.empty() && wenben.front() == '=') {
    wenben.erase(wenben.begin());
  }
  return trim(wenben);
}

bool is_identifier_char(char zifu) {
  return isalnum(static_cast<unsigned char>(zifu)) || zifu == '_';
}

size_t skip_spaces(const string& wenben, size_t qidian) {
  while (qidian < wenben.size() && isspace(static_cast<unsigned char>(wenben[qidian]))) {
    qidian += 1;
  }
  return qidian;
}

bool parse_range_ref(const string& wenben, CellRange& fanwei) {
  string houxuan = trim(wenben);
  size_t maohao = houxuan.find(':');
  if (maohao == string::npos) {
    return false;
  }

  string diyi = uppercase(trim(houxuan.substr(0, maohao)));
  string dier = uppercase(trim(houxuan.substr(maohao + 1)));
  if (!is_valid_cell_id(diyi) || !is_valid_cell_id(dier)) {
    return false;
  }

  fanwei = {parse_cell_id(diyi), parse_cell_id(dier)};
  return true;
}

bool find_matching_paren(const string& wenben, size_t kai_kuohao_xiabiao, size_t& bi_kuohao_xiabiao) {
  int shendu = 0;
  for (size_t xiabiao = kai_kuohao_xiabiao; xiabiao < wenben.size(); ++xiabiao) {
    if (wenben[xiabiao] == '(') {
      shendu += 1;
    } else if (wenben[xiabiao] == ')') {
      shendu -= 1;
      if (shendu == 0) {
        bi_kuohao_xiabiao = xiabiao;
        return true;
      }
      if (shendu < 0) {
        return false;
      }
    }
  }
  return false;
}

bool contains_id(const vector<string>& id_men, const string& mubiao_id) {
  for (const string& id : id_men) {
    if (id == mubiao_id) {
      return true;
    }
  }
  return false;
}

void remove_id(vector<string>& id_men, const string& mubiao_id) {
  for (size_t xiabiao = 0; xiabiao < id_men.size(); ++xiabiao) {
    if (id_men[xiabiao] == mubiao_id) {
      id_men.erase(id_men.begin() + static_cast<vector<string>::difference_type>(xiabiao));
      return;
    }
  }
}

void set_formula_error(CellRecord& danyuange) {
  danyuange.cuowu = "#NA";
  danyuange.xianshi = "#NA";
  danyuange.you_shuzhi = false;
  danyuange.shuzhi = 0.0;
}

bool evaluate_plain_expression(Workbook& gongzuobu,
                               const string& biaodashi,
                               vector<string>& fangwen_zhong,
                               vector<string>& yiwancheng,
                               double& shuzhi);

bool rewrite_expression(Workbook& gongzuobu,
                        const string& shuru,
                        vector<string>& fangwen_zhong,
                        vector<string>& yiwancheng,
                        string& shuchu) {
  shuchu.clear();

  for (size_t xiabiao = 0; xiabiao < shuru.size();) {
    if (!isalpha(static_cast<unsigned char>(shuru[xiabiao]))) {
      shuchu.push_back(shuru[xiabiao]);
      xiabiao += 1;
      continue;
    }

    size_t qidian = xiabiao;
    while (xiabiao < shuru.size() && is_identifier_char(shuru[xiabiao])) {
      xiabiao += 1;
    }

    string biaoshifu = shuru.substr(qidian, xiabiao - qidian);
    size_t houkan_xiabiao = skip_spaces(shuru, xiabiao);
    if (houkan_xiabiao < shuru.size() && shuru[houkan_xiabiao] == '(') {
      size_t bi_kuohao_xiabiao = 0;
      if (!find_matching_paren(shuru, houkan_xiabiao, bi_kuohao_xiabiao)) {
        return false;
      }

      string hanshu_ming = lowercase(biaoshifu);

      string canshu = shuru.substr(houkan_xiabiao + 1, bi_kuohao_xiabiao - houkan_xiabiao - 1);
      if (hanshu_ming == "sum" || hanshu_ming == "avg") {
        double hanshu_zhi = 0.0;
        CellRange fanwei;
        if (parse_range_ref(canshu, fanwei)) {
          if (!evaluate_range_numeric(gongzuobu, fanwei, hanshu_ming == "avg", fangwen_zhong,
                                      yiwancheng, hanshu_zhi)) {
            return false;
          }
        } else {
          string neiceng = strip_formula_prefix(canshu);
          if (neiceng.empty() ||
              !evaluate_plain_expression(gongzuobu, neiceng, fangwen_zhong, yiwancheng,
                                         hanshu_zhi)) {
            return false;
          }
        }
        shuchu += format_number(hanshu_zhi);
      } else {
        string gaixiehou_canshu;
        if (!rewrite_expression(gongzuobu, canshu, fangwen_zhong, yiwancheng, gaixiehou_canshu)) {
          return false;
        }
        shuchu += hanshu_ming + "(" + gaixiehou_canshu + ")";
      }

      xiabiao = bi_kuohao_xiabiao + 1;
      continue;
    }

    if (is_valid_cell_id(biaoshifu)) {
      shuchu += uppercase(biaoshifu);
    } else {
      shuchu += biaoshifu;
    }
  }

  return true;
}

bool evaluate_plain_expression(Workbook& gongzuobu,
                               const string& biaodashi,
                               vector<string>& fangwen_zhong,
                               vector<string>& yiwancheng,
                               double& shuzhi) {
  vector<string> mingcheng_men;
  for (size_t xiabiao = 0; xiabiao < biaodashi.size();) {
    if (!isalpha(static_cast<unsigned char>(biaodashi[xiabiao]))) {
      xiabiao += 1;
      continue;
    }

    size_t qidian = xiabiao;
    while (xiabiao < biaodashi.size() && is_identifier_char(biaodashi[xiabiao])) {
      xiabiao += 1;
    }

    string biaoshifu = biaodashi.substr(qidian, xiabiao - qidian);
    size_t houkan_xiabiao = skip_spaces(biaodashi, xiabiao);
    if (houkan_xiabiao < biaodashi.size() && biaodashi[houkan_xiabiao] == '(') {
      continue;
    }
    if (!is_valid_cell_id(biaoshifu)) {
      continue;
    }

    string guifanhou = uppercase(biaoshifu);
    if (!contains_id(mingcheng_men, guifanhou)) {
      mingcheng_men.push_back(guifanhou);
    }
  }

  vector<double> zhi_men;
  vector<te_variable> bianliang_men;
  zhi_men.reserve(mingcheng_men.size());
  bianliang_men.reserve(mingcheng_men.size());

  for (const string& mingcheng : mingcheng_men) {
    double danyuange_shuzhi = 0.0;
    if (!evaluate_cell_numeric(gongzuobu, mingcheng, fangwen_zhong, yiwancheng,
                               danyuange_shuzhi)) {
      return false;
    }
    zhi_men.push_back(danyuange_shuzhi);
  }

  for (size_t xiabiao = 0; xiabiao < mingcheng_men.size(); ++xiabiao) {
    bianliang_men.push_back(
        {mingcheng_men[xiabiao].c_str(), &zhi_men[xiabiao], TE_VARIABLE, nullptr});
  }

  int cuowu = 0;
  te_expr* biaodashi_shu =
      te_compile(biaodashi.c_str(), bianliang_men.empty() ? nullptr : bianliang_men.data(),
                 static_cast<int>(bianliang_men.size()), &cuowu);
  if (biaodashi_shu == nullptr || cuowu != 0) {
    te_free(biaodashi_shu);
    return false;
  }

  shuzhi = te_eval(biaodashi_shu);
  te_free(biaodashi_shu);
  return isfinite(shuzhi);
}

bool evaluate_range_numeric(Workbook& gongzuobu,
                            const CellRange& fanwei,
                            bool shi_pingjun,
                            vector<string>& fangwen_zhong,
                            vector<string>& yiwancheng,
                            double& shuzhi) {
  int zuixiao_hang = fanwei.qishi.hang;
  int zuida_hang = fanwei.jieshu.hang;
  if (zuixiao_hang > zuida_hang) {
    int linshi = zuixiao_hang;
    zuixiao_hang = zuida_hang;
    zuida_hang = linshi;
  }

  int zuixiao_lie = fanwei.qishi.lie;
  int zuida_lie = fanwei.jieshu.lie;
  if (zuixiao_lie > zuida_lie) {
    int linshi = zuixiao_lie;
    zuixiao_lie = zuida_lie;
    zuida_lie = linshi;
  }

  double zonghe = 0.0;
  int shuliang = 0;
  for (int hang = zuixiao_hang; hang <= zuida_hang; ++hang) {
    for (int lie = zuixiao_lie; lie <= zuida_lie; ++lie) {
      string dangqian_id = to_cell_id({hang, lie});
      if (!has_cell(gongzuobu, dangqian_id)) {
        continue;
      }

      const CellRecord& dangqian_danyuange = cell(gongzuobu, dangqian_id);
      if (dangqian_danyuange.leixing == CellKind::Empty ||
          dangqian_danyuange.leixing == CellKind::String) {
        continue;
      }

      double danyuange_shuzhi = 0.0;
      if (!evaluate_cell_numeric(gongzuobu, dangqian_id, fangwen_zhong, yiwancheng,
                                 danyuange_shuzhi)) {
        return false;
      }

      zonghe += danyuange_shuzhi;
      shuliang += 1;
    }
  }

  if (!shi_pingjun) {
    shuzhi = zonghe;
    return true;
  }
  if (shuliang == 0) {
    return false;
  }

  shuzhi = zonghe / static_cast<double>(shuliang);
  return true;
}

FormulaEvalResult evaluate_formula(Workbook& gongzuobu,
                                   const string& gongshi,
                                   vector<string>& fangwen_zhong,
                                   vector<string>& yiwancheng) {
  FormulaEvalResult jieguo;
  string zhuti = strip_formula_prefix(gongshi);
  if (zhuti.empty()) {
    return jieguo;
  }

  string gaixiehou;
  if (!rewrite_expression(gongzuobu, zhuti, fangwen_zhong, yiwancheng, gaixiehou)) {
    return jieguo;
  }

  double shuzhi = 0.0;
  if (!evaluate_plain_expression(gongzuobu, gaixiehou, fangwen_zhong, yiwancheng, shuzhi)) {
    return jieguo;
  }

  jieguo.chenggong = true;
  jieguo.shuzhi = shuzhi;
  return jieguo;
}

bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,
                           vector<string>& yiwancheng,
                           double& shuzhi) {
  if (!is_valid_cell_id(danyuange_id)) {
    return false;
  }
  if (!has_cell(gongzuobu, danyuange_id)) {
    shuzhi = 0.0;
    return true;
  }

  CellRecord& danyuange = mutable_cells(gongzuobu).at(danyuange_id);
  switch (danyuange.leixing) {
    case CellKind::Empty:
      shuzhi = 0.0;
      return true;
    case CellKind::Integer:
    case CellKind::Float:
      shuzhi = danyuange.shuzhi;
      return true;
    case CellKind::String:
      return false;
    case CellKind::Formula:
      break;
  }

  if (contains_id(yiwancheng, danyuange_id)) {
    if (!danyuange.you_shuzhi) {
      return false;
    }
    shuzhi = danyuange.shuzhi;
    return true;
  }

  if (contains_id(fangwen_zhong, danyuange_id)) {
    set_formula_error(danyuange);
    return false;
  }

  fangwen_zhong.push_back(danyuange_id);
  FormulaEvalResult jieguo = evaluate_formula(gongzuobu, danyuange.yuanshi, fangwen_zhong,
                                              yiwancheng);
  remove_id(fangwen_zhong, danyuange_id);
  yiwancheng.push_back(danyuange_id);

  if (!jieguo.chenggong || !isfinite(jieguo.shuzhi)) {
    set_formula_error(danyuange);
    return false;
  }

  danyuange.cuowu.clear();
  danyuange.you_shuzhi = true;
  danyuange.shuzhi = jieguo.shuzhi;
  danyuange.xianshi = format_number(jieguo.shuzhi);
  shuzhi = jieguo.shuzhi;
  return true;
}
