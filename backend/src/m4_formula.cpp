// ========================================
// 公式计算实现文件
// 这是最复杂的模块，要处理公式解析、计算、范围函数等
// ========================================

#include "minisheet/m4_formula.h"

#include "../third_party/tinyexpr/tinyexpr.h"  // 第三方表达式计算库

#include <cctype>    
#include <cmath>     
#include <vector>  

using namespace std;

namespace {

// 把字符串转成大写
// 用来统一单元格ID的大小写，比如"a1"变成"A1"
string uppercase(string wenben) {
  for (char& zifu : wenben) {
    zifu = static_cast<char>(toupper(static_cast<unsigned char>(zifu)));
  }
  return wenben;
}

// 把字符串转成小写
// 用来统一函数名的大小写，比如"SUM"和"sum"都能识别
string lowercase(string wenben) {
  for (char& zifu : wenben) {
    zifu = static_cast<char>(tolower(static_cast<unsigned char>(zifu)));
  }
  return wenben;
}

// 去掉公式前面的等号
// 比如 "=A1+B1" 变成 "A1+B1"
string strip_formula_prefix(const string& gongshi) {
  string wenben = trim(gongshi);  // 先去掉前后空白
  if (!wenben.empty() && wenben.front() == '=') {
    wenben.erase(wenben.begin());  // 删掉第一个字符（等号）
  }
  return trim(wenben);  // 再去掉空白返回
}

// 判断字符是不是标识符字符（字母、数字、下划线）
bool is_identifier_char(char zifu) {
  return isalnum(static_cast<unsigned char>(zifu)) || zifu == '_';
}


int text_length(const string& wenben) {
  return static_cast<int>(wenben.size());
}

// 在字符串里查找某个字符，找到返回位置，没找到返回-1
int find_char(const string& wenben, char mubiao) {
  for (int xiabiao = 0; xiabiao < text_length(wenben); ++xiabiao) {
    if (wenben[xiabiao] == mubiao) {
      return xiabiao;
    }
  }
  return -1;
}

// 跳过字符串中的空白字符，返回下一个非空白字符的位置
int skip_spaces(const string& wenben, int qidian) {
  while (qidian < text_length(wenben) &&
         isspace(static_cast<unsigned char>(wenben[qidian]))) {
    qidian += 1;
  }
  return qidian;
}

// 解析范围引用，比如 "A1:B10"
// 成功返回true，fanwei参数得到解析结果
bool parse_range_ref(const string& wenben, CellRange& fanwei) {
  string houxuan = trim(wenben);

  // 找冒号
  int maohao = find_char(houxuan, ':');
  if (maohao < 0) {
    return false;  // 没有冒号，不是范围
  }

  // 冒号前后各是一个单元格ID
  string diyi = uppercase(trim(houxuan.substr(0, maohao)));
  string dier = uppercase(trim(houxuan.substr(maohao + 1)));

  // 检查两个ID是否都合法
  if (!is_valid_cell_id(diyi) || !is_valid_cell_id(dier)) {
    return false;
  }

  // 解析成坐标
  fanwei = {parse_cell_id(diyi), parse_cell_id(dier)};
  return true;
}

// 找匹配的右括号
// 从kai_kuohao_xiabiao（左括号位置）开始找对应的右括号
// 要考虑嵌套，比如 (1+(2+3)) 要找第二个右括号
// 找到返回true，bi_kuohao_xiabiao得到右括号位置
bool find_matching_paren(const string& wenben,
                         int kai_kuohao_xiabiao,
                         int& bi_kuohao_xiabiao) {
  int shendu = 0;  // 深度计数器

  for (int xiabiao = kai_kuohao_xiabiao; xiabiao < text_length(wenben); ++xiabiao) {
    if (wenben[xiabiao] == '(') {
      shendu += 1;  // 遇到左括号，深度加1
    } else if (wenben[xiabiao] == ')') {
      shendu -= 1;  // 遇到右括号，深度减1
      if (shendu == 0) {
        // 深度回到0，找到匹配的右括号了
        bi_kuohao_xiabiao = xiabiao;
        return true;
      }
      if (shendu < 0) {
        // 深度变成负数，说明括号不匹配
        return false;
      }
    }
  }
  return false;  // 遍历完都没找到，括号不匹配
}

// 检查字符串数组里是否包含某个ID
bool contains_id(const vector<string>& id_men, const string& mubiao_id) {
  for (const string& id : id_men) {
    if (id == mubiao_id) {
      return true;
    }
  }
  return false;
}

// 设置公式错误状态
// 当公式计算出错时调用，把单元格标记为#NA
void set_formula_error(CellRecord& danyuange) {
  danyuange.cuowu = "#NA";      // 错误信息
  danyuange.xianshi = "#NA";    // 显示#NA
  danyuange.you_shuzhi = false; // 没有有效数值
  danyuange.shuzhi = 0.0;       // 数值归零
}

// 预声明，因为几个递归函数会互相调用
bool evaluate_plain_expression(Workbook& gongzuobu,
                               const string& biaodashi,
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
bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,
                           vector<string>& yiwancheng,
                           double& shuzhi);

// 改写表达式
// 把公式中的单元格引用替换成数值，函数调用（如SUM）也计算掉
// 这是递归函数，处理嵌套的函数调用
bool rewrite_expression(Workbook& gongzuobu,
                        const string& shuru,
                        vector<string>& fangwen_zhong,
                        vector<string>& yiwancheng,
                        string& shuchu) {
  shuchu.clear();  // 清空输出

  // 遍历输入字符串
  for (int xiabiao = 0; xiabiao < text_length(shuru);) {
    // 如果不是字母，直接复制到输出
    if (!isalpha(static_cast<unsigned char>(shuru[xiabiao]))) {
      shuchu.push_back(shuru[xiabiao]);
      xiabiao += 1;
      continue;
    }

    // 是字母，开始读取标识符（字母、数字、下划线）
    int qidian = xiabiao;
    while (xiabiao < text_length(shuru) && is_identifier_char(shuru[xiabiao])) {
      xiabiao += 1;
    }

    // 得到标识符
    string biaoshifu = shuru.substr(qidian, xiabiao - qidian);

    // 跳过空白，看后面是不是左括号
    int houkan_xiabiao = skip_spaces(shuru, xiabiao);
    if (houkan_xiabiao < text_length(shuru) && shuru[houkan_xiabiao] == '(') {
      // 是函数调用！
      int bi_kuohao_xiabiao = 0;
      // 找匹配的右括号
      if (!find_matching_paren(shuru, houkan_xiabiao, bi_kuohao_xiabiao)) {
        return false;  // 括号不匹配
      }

      // 函数名转小写，统一处理
      string hanshu_ming = lowercase(biaoshifu);

      // 提取参数部分（括号里的内容）
      string canshu = shuru.substr(houkan_xiabiao + 1, bi_kuohao_xiabiao - houkan_xiabiao - 1);

      if (hanshu_ming == "sum" || hanshu_ming == "avg") {
        double hanshu_zhi = 0.0;
        CellRange fanwei;

        // 先尝试解析成范围引用（如A1:B10）
        if (parse_range_ref(canshu, fanwei)) {
          // 是范围，计算范围的总和或平均值
          if (!evaluate_range_numeric(gongzuobu, fanwei, hanshu_ming == "avg", fangwen_zhong,
                                      yiwancheng, hanshu_zhi)) {
            return false;  // 计算失败
          }
        } else {
          // 不是范围，可能是单个表达式，递归计算
          string neiceng = strip_formula_prefix(canshu);
          if (neiceng.empty() ||
              !evaluate_plain_expression(gongzuobu, neiceng, fangwen_zhong, yiwancheng,
                                         hanshu_zhi)) {
            return false;  // 计算失败
          }
        }
        // 把计算结果作为数字写入输出
        shuchu += format_number(hanshu_zhi);
      } else {
        // 其他函数（比如sin, cos等数学函数），改写参数然后保留函数调用
        string gaixiehou_canshu;
        if (!rewrite_expression(gongzuobu, canshu, fangwen_zhong, yiwancheng, gaixiehou_canshu)) {
          return false;
        }
        shuchu += hanshu_ming + "(" + gaixiehou_canshu + ")";
      }

      // 跳到右括号后面继续处理
      xiabiao = bi_kuohao_xiabiao + 1;
      continue;
    }

    // 不是函数调用，检查是不是单元格ID
    if (is_valid_cell_id(biaoshifu)) {
      // 是单元格ID，转成大写统一格式
      shuchu += uppercase(biaoshifu);
    } else {
      // 不是单元格ID，可能是数学常数（如pi）或函数名，原样保留
      shuchu += biaoshifu;
    }
  }

  return true;
}

// 计算普通表达式
// 改写后的表达式（单元格都已替换成数值）用tinyexpr库计算
bool evaluate_plain_expression(Workbook& gongzuobu,
                               const string& biaodashi,
                               vector<string>& fangwen_zhong,
                               vector<string>& yiwancheng,
                               double& shuzhi) {
  // 第一遍扫描：找出所有单元格ID
  vector<string> mingcheng_men;
  for (int xiabiao = 0; xiabiao < text_length(biaodashi);) {
    if (!isalpha(static_cast<unsigned char>(biaodashi[xiabiao]))) {
      xiabiao += 1;
      continue;
    }

    int qidian = xiabiao;
    while (xiabiao < text_length(biaodashi) && is_identifier_char(biaodashi[xiabiao])) {
      xiabiao += 1;
    }

    string biaoshifu = biaodashi.substr(qidian, xiabiao - qidian);
    int houkan_xiabiao = skip_spaces(biaodashi, xiabiao);
    // 如果后面跟着左括号，是函数调用，跳过
    if (houkan_xiabiao < text_length(biaodashi) && biaodashi[houkan_xiabiao] == '(') {
      continue;
    }
    // 如果不是单元格ID，跳过
    if (!is_valid_cell_id(biaoshifu)) {
      continue;
    }

    // 是单元格ID，加入列表（去重）
    string guifanhou = uppercase(biaoshifu);
    if (!contains_id(mingcheng_men, guifanhou)) {
      mingcheng_men.push_back(guifanhou);
    }
  }

  // 计算所有引用到的单元格的值
  vector<double> zhi_men;           // 存数值
  vector<te_variable> bianliang_men; // 存tinyexpr的变量定义
  zhi_men.reserve(mingcheng_men.size());
  bianliang_men.reserve(mingcheng_men.size());

  for (const string& mingcheng : mingcheng_men) {
    double danyuange_shuzhi = 0.0;
    // 递归计算单元格的值
    if (!evaluate_cell_numeric(gongzuobu, mingcheng, fangwen_zhong, yiwancheng,
                               danyuange_shuzhi)) {
      return false;  // 计算失败（公式错误、字符串参与计算等）
    }
    zhi_men.push_back(danyuange_shuzhi);
  }

  // 创建tinyexpr的变量绑定
  for (int xiabiao = 0; xiabiao < static_cast<int>(mingcheng_men.size()); ++xiabiao) {
    bianliang_men.push_back(
        {mingcheng_men[xiabiao].c_str(), &zhi_men[xiabiao], TE_VARIABLE, nullptr});
  }

  // 编译表达式
  int cuowu = 0;
  te_expr* biaodashi_shu =
      te_compile(biaodashi.c_str(), bianliang_men.empty() ? nullptr : bianliang_men.data(),
                 static_cast<int>(bianliang_men.size()), &cuowu);

  // 检查编译是否成功
  if (biaodashi_shu == nullptr || cuowu != 0) {
    te_free(biaodashi_shu);
    return false;  // 编译失败（表达式语法错误）
  }

  // 执行计算
  shuzhi = te_eval(biaodashi_shu);
  te_free(biaodashi_shu);

  // 检查结果是否有效（不是无穷大或NaN）
  return isfinite(shuzhi);
}

// 计算范围内的数值总和或平均值
bool evaluate_range_numeric(Workbook& gongzuobu,
                            const CellRange& fanwei,
                            bool shi_pingjun,  // true算平均值，false算总和
                            vector<string>& fangwen_zhong,
                            vector<string>& yiwancheng,
                            double& shuzhi) {
  // 确定行范围（考虑反向选择，比如B10:A1）
  int zuixiao_hang = fanwei.qishi.hang;
  int zuida_hang = fanwei.jieshu.hang;
  if (zuixiao_hang > zuida_hang) {
    // 交换
    int linshi = zuixiao_hang;
    zuixiao_hang = zuida_hang;
    zuida_hang = linshi;
  }

  // 确定列范围
  int zuixiao_lie = fanwei.qishi.lie;
  int zuida_lie = fanwei.jieshu.lie;
  if (zuixiao_lie > zuida_lie) {
    // 交换
    int linshi = zuixiao_lie;
    zuixiao_lie = zuida_lie;
    zuida_lie = linshi;
  }

  // 遍历范围内的所有单元格
  double zonghe = 0.0;  // 总和
  int shuliang = 0;     // 有效单元格数量

  for (int hang = zuixiao_hang; hang <= zuida_hang; ++hang) {
    for (int lie = zuixiao_lie; lie <= zuida_lie; ++lie) {
      string dangqian_id = to_cell_id({hang, lie});

      // 如果这个单元格不存在，跳过
      if (!has_cell(gongzuobu, dangqian_id)) {
        continue;
      }

      // 获取单元格信息
      const CellRecord& dangqian_danyuange = cell(gongzuobu, dangqian_id);

      // 空单元格和字符串单元格不计入
      if (dangqian_danyuange.leixing == CellKind::Empty ||
          dangqian_danyuange.leixing == CellKind::String) {
        continue;
      }

      // 递归计算这个单元格的值
      double danyuange_shuzhi = 0.0;
      if (!evaluate_cell_numeric(gongzuobu, dangqian_id, fangwen_zhong, yiwancheng,
                                 danyuange_shuzhi)) {
        return false;  // 计算失败
      }

      // 累加
      zonghe += danyuange_shuzhi;
      shuliang += 1;
    }
  }

  // 计算结果
  if (!shi_pingjun) {
    // 要总和，直接返回
    shuzhi = zonghe;
    return true;
  }
  // 要平均值
  if (shuliang == 0) {
    return false;  // 没有有效单元格，无法算平均值
  }
  shuzhi = zonghe / static_cast<double>(shuliang);
  return true;
}

// 计算公式的主入口函数
FormulaEvalResult evaluate_formula(Workbook& gongzuobu,
                                   const string& gongshi,
                                   vector<string>& fangwen_zhong,
                                   vector<string>& yiwancheng) {
  FormulaEvalResult jieguo;  // 默认失败状态

  // 去掉前面的等号
  string zhuti = strip_formula_prefix(gongshi);
  if (zhuti.empty()) {
    return jieguo;  // 公式为空，返回失败
  }

  // 改写表达式（处理单元格引用和SUM/AVG等函数）
  string gaixiehou;
  if (!rewrite_expression(gongzuobu, zhuti, fangwen_zhong, yiwancheng, gaixiehou)) {
    return jieguo;  // 改写失败
  }

  // 计算改写后的表达式
  double shuzhi = 0.0;
  if (!evaluate_plain_expression(gongzuobu, gaixiehou, fangwen_zhong, yiwancheng, shuzhi)) {
    return jieguo;  // 计算失败
  }

  // 成功！
  jieguo.chenggong = true;
  jieguo.shuzhi = shuzhi;
  return jieguo;
}

// 计算单个单元格的数值
// 这是递归调用的核心函数
bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& fangwen_zhong,  // 当前访问栈
                           vector<string>& yiwancheng,      // 已完成的单元格
                           double& shuzhi) {
  if (!is_valid_cell_id(danyuange_id)) {
    return false;
  }

  // 如果单元格不存在，数值为0
  if (!has_cell(gongzuobu, danyuange_id)) {
    shuzhi = 0.0;
    return true;
  }

  // 获取单元格的可写引用（因为要更新计算结果）
  CellRecord& danyuange = mutable_cells(gongzuobu).at(danyuange_id);

  // 根据单元格类型分别处理
  switch (danyuange.leixing) {
    case CellKind::Empty:
      shuzhi = 0.0;
      return true;

    case CellKind::Integer:
    case CellKind::Float:
      // 数字类型直接返回保存的数值
      shuzhi = danyuange.shuzhi;
      return true;

    case CellKind::String:
      // 字符串没有数值
      return false;

    case CellKind::Formula:
      // 公式类型，继续下面的处理
      break;
  }

  // 检查是否已经计算完成了（避免重复计算）
  if (contains_id(yiwancheng, danyuange_id)) {
    if (!danyuange.you_shuzhi) {
      return false;  // 之前计算失败了
    }
    shuzhi = danyuange.shuzhi;
    return true;
  }

  // 如果当前单元格已经在访问栈里，说明出现了循环引用
  if (contains_id(fangwen_zhong, danyuange_id)) {
    set_formula_error(danyuange);
    return false;
  }

  fangwen_zhong.push_back(danyuange_id);

  // 计算公式
  FormulaEvalResult jieguo =
      evaluate_formula(gongzuobu, danyuange.yuanshi, fangwen_zhong, yiwancheng);
  fangwen_zhong.pop_back();

  // 标记为已完成
  yiwancheng.push_back(danyuange_id);

  // 检查计算结果
  if (!jieguo.chenggong || !isfinite(jieguo.shuzhi)) {
    set_formula_error(danyuange);
    return false;
  }

  // 保存计算结果到单元格
  danyuange.cuowu.clear();        // 清除错误
  danyuange.you_shuzhi = true;    // 标记有数值
  danyuange.shuzhi = jieguo.shuzhi;  // 保存数值
  danyuange.xianshi = format_number(jieguo.shuzhi);  // 更新显示值

  shuzhi = jieguo.shuzhi;
  return true;
}

}  // namespace

bool evaluate_range_numeric(Workbook& gongzuobu,
                            const CellRange& fanwei,
                            bool shi_pingjun,
                            vector<string>& yiwancheng,
                            double& shuzhi) {
  vector<string> fangwen_zhong;
  return evaluate_range_numeric(gongzuobu, fanwei, shi_pingjun, fangwen_zhong, yiwancheng,
                                shuzhi);
}

FormulaEvalResult evaluate_formula(Workbook& gongzuobu,
                                   const string& gongshi,
                                   vector<string>& yiwancheng) {
  vector<string> fangwen_zhong;
  return evaluate_formula(gongzuobu, gongshi, fangwen_zhong, yiwancheng);
}

bool evaluate_cell_numeric(Workbook& gongzuobu,
                           const string& danyuange_id,
                           vector<string>& yiwancheng,
                           double& shuzhi) {
  vector<string> fangwen_zhong;
  return evaluate_cell_numeric(gongzuobu, danyuange_id, fangwen_zhong, yiwancheng, shuzhi);
}
