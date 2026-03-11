/**
 * @file m4_formula.cpp
 * @brief 公式解析和求值
 *
 * 本文件实现：
 * - 公式表达式解析和重写
 * - 单元格引用和范围引用（如 A1:B2）识别
 * - 支持函数：SUM、AVG
 * - 使用 tinyexpr 库进行数学表达式求值
 */

#include "minisheet/m4_formula.h"

#include "minisheet/m1_types.h"

#include "../third_party/tinyexpr/tinyexpr.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

using namespace std;

namespace minisheet {
namespace {

// ----------------------------------------------------------------------------
// 字符串大小写转换
// ----------------------------------------------------------------------------

string uppercase(string wenben) {
  transform(wenben.begin(), wenben.end(), wenben.begin(), [](unsigned char zifu) {
    return static_cast<char>(std::toupper(zifu));
  });
  return wenben;
}

string lowercase(string wenben) {
  transform(wenben.begin(), wenben.end(), wenben.begin(), [](unsigned char zifu) {
    return static_cast<char>(std::tolower(zifu));
  });
  return wenben;
}

// ----------------------------------------------------------------------------
// 去除公式前缀 "=" 并 trim
// ----------------------------------------------------------------------------
string strip_formula_prefix(const string& gongshi) {
  string wenben = trim(gongshi);
  if (!wenben.empty() && wenben.front() == '=') {
    wenben.erase(wenben.begin());
  }
  return trim(wenben);
}

// ----------------------------------------------------------------------------
// 检查字符是否为标识符字符（字母数字或下划线）
// ----------------------------------------------------------------------------
bool is_identifier_char(char zifu) {
  return std::isalnum(static_cast<unsigned char>(zifu)) || zifu == '_';
}

// ----------------------------------------------------------------------------
// 跳过字符串中的空白字符，返回下一个非空白位置
// ----------------------------------------------------------------------------
size_t skip_spaces(const string& wenben, size_t qidian) {
  while (qidian < wenben.size() && std::isspace(static_cast<unsigned char>(wenben[qidian]))) {
    qidian += 1;
  }
  return qidian;
}

// ----------------------------------------------------------------------------
// 解析范围引用，如 "A1:B2" 或 "B2:A1"
// 返回：是否解析成功，结果存入 fanwei 参数
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// 查找匹配的右括号位置
// 参数：
//   wenben - 表达式字符串
//   kai_kuohao_xiabiao - 左括号 '(' 的位置
//   bi_kuohao_xiabiao - 输出：匹配的右括号位置
// 返回：是否找到匹配的括号
// ----------------------------------------------------------------------------
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

// 前向声明：求值普通表达式
bool evaluate_plain_expression(const string& biaodashi,
                               const CellResolver& danyuange_jiexiqi,
                               double& shuzhi);

// ----------------------------------------------------------------------------
// 求值函数调用
// 支持：SUM（求和）、AVG（平均值）
// 参数可以是范围引用（如 A1:B2）或表达式
// ----------------------------------------------------------------------------
bool evaluate_function_call(const string& xiaoxie_ming,
                            const string& canshu_wenben,
                            const CellResolver& danyuange_jiexiqi,
                            const RangeResolver& fanwei_jiexiqi,
                            double& shuzhi) {
  if (xiaoxie_ming == "sum" || xiaoxie_ming == "avg") {
    CellRange fanwei;
    if (parse_range_ref(canshu_wenben, fanwei)) {
      return fanwei_jiexiqi(fanwei, xiaoxie_ming == "avg", shuzhi);
    }

    string neiceng = strip_formula_prefix(canshu_wenben);
    if (neiceng.empty()) {
      return false;
    }

    return evaluate_plain_expression(neiceng, danyuange_jiexiqi, shuzhi);
  }

  return false;
}

// ----------------------------------------------------------------------------
// 重写表达式：将单元格引用和函数调用替换为可求值的形式
// - 单元格引用保留（如 A1）
// - SUM/AVG 函数直接求值替换为数值
// - 其他函数递归处理参数
// ----------------------------------------------------------------------------
bool rewrite_expression(const string& shuru,
                        string& shuchu,
                        const CellResolver& danyuange_jiexiqi,
                        const RangeResolver& fanwei_jiexiqi) {
  shuchu.clear();

  for (size_t xiabiao = 0; xiabiao < shuru.size();) {
    char dangqian_zi = shuru[xiabiao];
    if (std::isalpha(static_cast<unsigned char>(dangqian_zi))) {
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

        string xiaoxie_ming = lowercase(biaoshifu);
        string canshu = shuru.substr(houkan_xiabiao + 1, bi_kuohao_xiabiao - houkan_xiabiao - 1);
        if (xiaoxie_ming == "sum" || xiaoxie_ming == "avg") {
          // SUM/AVG 直接求值
          double hanshu_zhi = 0.0;
          if (!evaluate_function_call(xiaoxie_ming, canshu, danyuange_jiexiqi, fanwei_jiexiqi,
                                      hanshu_zhi)) {
            return false;
          }
          shuchu += format_number(hanshu_zhi);
        } else {
          // 其他函数递归处理
          string gaixiehou_canshu;
          if (!rewrite_expression(canshu, gaixiehou_canshu, danyuange_jiexiqi, fanwei_jiexiqi)) {
            return false;
          }
          shuchu += xiaoxie_ming;
          shuchu += "(";
          shuchu += gaixiehou_canshu;
          shuchu += ")";
        }
        xiabiao = bi_kuohao_xiabiao + 1;
      } else if (is_valid_cell_id(biaoshifu)) {
        // 单元格引用转为大写
        shuchu += uppercase(biaoshifu);
      } else {
        shuchu += biaoshifu;
      }
      continue;
    }

    shuchu.push_back(dangqian_zi);
    xiabiao += 1;
  }

  return true;
}

// ----------------------------------------------------------------------------
// 收集表达式中的单元格变量名（去重）
// 例如 "A1+B1+A1" 返回 ["A1", "B1"]
// ----------------------------------------------------------------------------
vector<string> collect_cell_variables(const string& wenben) {
  vector<string> mingcheng_men;
  for (size_t xiabiao = 0; xiabiao < wenben.size();) {
    if (!std::isalpha(static_cast<unsigned char>(wenben[xiabiao]))) {
      xiabiao += 1;
      continue;
    }

    size_t qidian = xiabiao;
    while (xiabiao < wenben.size() && is_identifier_char(wenben[xiabiao])) {
      xiabiao += 1;
    }

    string biaoshifu = wenben.substr(qidian, xiabiao - qidian);
    size_t houkan_xiabiao = skip_spaces(wenben, xiabiao);
    if (houkan_xiabiao < wenben.size() && wenben[houkan_xiabiao] == '(') {
      // 函数调用，跳过
      continue;
    }

    if (is_valid_cell_id(biaoshifu)) {
      string guifanhou = uppercase(biaoshifu);
      if (find(mingcheng_men.begin(), mingcheng_men.end(), guifanhou) == mingcheng_men.end()) {
        mingcheng_men.push_back(guifanhou);
      }
    }
  }
  return mingcheng_men;
}

// ----------------------------------------------------------------------------
// 使用 tinyexpr 求值普通数学表达式
// 步骤：
// 1. 收集所有单元格变量名
// 2. 通过 CellResolver 获取每个变量的数值
// 3. 编译并求值表达式
// ----------------------------------------------------------------------------
bool evaluate_plain_expression(const string& biaodashi,
                               const CellResolver& danyuange_jiexiqi,
                               double& shuzhi) {
  vector<string> mingcheng_men = collect_cell_variables(biaodashi);
  vector<double> zhi_men;
  vector<te_variable> bianliang_men;
  zhi_men.reserve(mingcheng_men.size());
  bianliang_men.reserve(mingcheng_men.size());

  for (const string& mingcheng : mingcheng_men) {
    double danyuange_shuzhi = 0.0;
    if (!danyuange_jiexiqi(mingcheng, danyuange_shuzhi)) {
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
  if (!std::isfinite(shuzhi)) {
    return false;
  }
  return true;
}

}  // namespace

// ----------------------------------------------------------------------------
// 主入口：求值公式
// 步骤：
// 1. 去除 "=" 前缀
// 2. 重写表达式（处理 SUM/AVG 等函数）
// 3. 求值普通表达式
// ----------------------------------------------------------------------------
FormulaEvalResult evaluate_formula(const std::string& gongshi,
                                   const CellResolver& danyuange_jiexiqi,
                                   const RangeResolver& fanwei_jiexiqi) {
  FormulaEvalResult jieguo;
  string zhuti = strip_formula_prefix(gongshi);
  if (zhuti.empty()) {
    return jieguo;
  }

  string gaixiehou;
  if (!rewrite_expression(zhuti, gaixiehou, danyuange_jiexiqi, fanwei_jiexiqi)) {
    return jieguo;
  }

  double shuzhi = 0.0;
  if (!evaluate_plain_expression(gaixiehou, danyuange_jiexiqi, shuzhi)) {
    return jieguo;
  }

  jieguo.chenggong = true;
  jieguo.shuzhi = shuzhi;
  return jieguo;
}

}  // namespace minisheet
