// ========================================
// 基础类型实现文件
// 实现m1_types.h里声明的各种工具函数
// ========================================

#include "minisheet/m1_types.h"

#include <algorithm>   // 为了用reverse函数
#include <cctype>     // 为了用isalpha, isdigit等字符判断函数
#include <cmath>      // 为了用isfinite检查数值是否有效
#include <cstdlib>    // 为了用strtod把字符串转数字
#include <filesystem> // 为了创建目录
#include <fstream>    // 为了读写文件
#include <iomanip>    // 为了设置数字精度
#include <sstream>    // 为了用ostringstream格式化数字
#include <stdexcept>  // 为了抛异常

using namespace std;

// 把列号（比如1）转成列名（比如"A"）
// 这个算法有点绕，因为是26进制的转换
// 比如1->A, 26->Z, 27->AA, 28->AB
// 注意：Excel的列名没有0，是从1开始的26进制
string column_index_to_name(int yiji_lie) {
  // 先检查一下范围对不对
  if (yiji_lie < 1 || yiji_lie > kMaxColumns) {
    throw invalid_argument("列号超出范围");
  }

  string lie_ming;      // 存结果的字符串
  int lie_zhi = yiji_lie;  // 用临时变量来计算

  // 循环除以26，直到变成0
  while (lie_zhi > 0) {
    lie_zhi -= 1;  // 先减1，因为A对应的是1不是0
    // 取模得到当前位的字母，'A' + 0 = 'A', 'A' + 25 = 'Z'
    lie_ming.push_back(static_cast<char>('A' + (lie_zhi % 26)));
    lie_zhi /= 26;  // 除以26进一位
  }

  // 现在得到的字母是反的（低位在前），要反转一下
  reverse(lie_ming.begin(), lie_ming.end());
  return lie_ming;
}

// 把列名（比如"A"）转成列号（比如1）
// 是上面那个函数的反向操作
int column_name_to_index(const string& lie_ming) {
  // 空字符串不行
  if (lie_ming.empty()) {
    throw invalid_argument("列名不能为空");
  }

  int lie_zhi = 0;
  // 遍历每个字符
  for (char zifu : lie_ming) {
    // 必须是字母
    if (!isalpha(static_cast<unsigned char>(zifu))) {
      throw invalid_argument("列名不合法");
    }
    // 26进制转换：前面累积的乘以26，加上当前位的值
    // 这里用toupper确保大小写都能处理，A=1, B=2, ...
    lie_zhi = lie_zhi * 26 + (toupper(static_cast<unsigned char>(zifu)) - 'A' + 1);
  }

  // 检查范围
  if (lie_zhi < 1 || lie_zhi > kMaxColumns) {
    throw invalid_argument("列号超出范围");
  }
  return lie_zhi;
}

// 解析单元格ID，比如把"A1"解析成坐标{hang=1, lie=1}
CellCoord parse_cell_id(const string& danyuange_id) {
  string lie_ming;    // 存字母部分
  string hang_haoma;  // 存数字部分

  // 遍历字符串的每个字符
  for (char zifu : danyuange_id) {
    // 如果是字母，而且还没开始读数字，就加到列名里
    if (isalpha(static_cast<unsigned char>(zifu)) && hang_haoma.empty()) {
      lie_ming.push_back(static_cast<char>(toupper(static_cast<unsigned char>(zifu))));
    }
    // 如果是数字，加到行号里
    else if (isdigit(static_cast<unsigned char>(zifu))) {
      hang_haoma.push_back(zifu);
    }
    // 其他字符都是非法的
    else {
      throw invalid_argument("单元格ID不合法");
    }
  }

  // 列名和行号都不能为空
  if (lie_ming.empty() || hang_haoma.empty()) {
    throw invalid_argument("单元格ID不合法");
  }

  // 组装坐标结构体
  CellCoord zuobiao {stoi(hang_haoma), column_name_to_index(lie_ming)};

  // 检查坐标是否在合法范围内
  if (!is_valid_coord(zuobiao)) {
    throw invalid_argument("单元格ID超出范围");
  }
  return zuobiao;
}

// 把坐标转回字符串ID
string to_cell_id(const CellCoord& zuobiao) {
  // 先检查坐标是否合法
  if (!is_valid_coord(zuobiao)) {
    throw invalid_argument("单元格坐标超出范围");
  }
  // 列名+行号，比如A1
  return column_index_to_name(zuobiao.lie) + to_string(zuobiao.hang);
}

// 检查坐标是否合法
bool is_valid_coord(const CellCoord& zuobiao) {
  // 行号要在1到kMaxRows之间，列号要在1到kMaxColumns之间
  return zuobiao.hang >= 1 && zuobiao.hang <= kMaxRows && zuobiao.lie >= 1 &&
         zuobiao.lie <= kMaxColumns;
}

// 检查单元格ID字符串是否合法
// 方法就是尝试解析，解析成功就合法，抛异常就不合法
bool is_valid_cell_id(const string& danyuange_id) {
  try {
    (void)parse_cell_id(danyuange_id);  // 尝试解析，结果丢弃
    return true;  // 没抛异常，合法
  } catch (...) {
    return false;  // 抛异常了，不合法
  }
}

// 去掉字符串前后的空白字符（空格、制表符、换行等）
string trim(const string& zhi) {
  // 找第一个非空白字符的位置
  size_t qidian = 0;
  while (qidian < zhi.size() && isspace(static_cast<unsigned char>(zhi[qidian]))) {
    qidian += 1;
  }

  // 找最后一个非空白字符的位置
  size_t zhongdian = zhi.size();
  while (zhongdian > qidian && isspace(static_cast<unsigned char>(zhi[zhongdian - 1]))) {
    zhongdian -= 1;
  }

  // 截取中间部分返回
  return zhi.substr(qidian, zhongdian - qidian);
}

// 尝试把字符串解析成数字
bool try_parse_number(const string& yuanshi, double& zhi, bool& shi_zhengshu) {
  // 先去掉前后空白
  string houxuan = trim(yuanshi);
  if (houxuan.empty()) {
    return false;  // 空字符串不是数字
  }

  // 用strtod来解析，这是C标准库的函数，比较强大
  char* jieshu_zhizhen = nullptr;
  zhi = strtod(houxuan.c_str(), &jieshu_zhizhen);

  // 如果jieshu_zhizhen指向的不是字符串结尾，说明后面还有非数字字符
  if (jieshu_zhizhen == nullptr || *jieshu_zhizhen != '\0') {
    return false;  // 解析失败
  }

  // 判断是否整数：看原始字符串里有没有小数点或科学计数法的e/E
  shi_zhengshu = houxuan.find_first_of(".eE") == string::npos;
  return true;
}

// 把数字格式化成字符串
string format_number(double zhi) {
  // 先检查是不是无穷大或NaN
  if (!isfinite(zhi)) {
    return "#NA";  // 无效的显示成#NA
  }

  // 四舍五入到整数，看看跟原值差多少
  double quzheng_zhi = round(zhi);
  // 如果差很小，认为是整数，显示成整数形式
  if (fabs(zhi - quzheng_zhi) < 1e-9) {
    return to_string(static_cast<long long>(quzheng_zhi));
  }

  // 否则按小数显示，先设置精度为10位小数
  ostringstream liu;
  liu << fixed << setprecision(10) << zhi;
  string wenben = liu.str();

  // 去掉后面多余的0
  while (!wenben.empty() && wenben.back() == '0') {
    wenben.pop_back();
  }
  // 如果最后一位是小数点，也去掉
  if (!wenben.empty() && wenben.back() == '.') {
    wenben.pop_back();
  }
  return wenben;
}

// 读取整个文本文件
string read_text_file(const string& lujing) {
  // 用二进制模式打开，这样不会转换换行符
  ifstream shuru(lujing, ios::binary);
  if (!shuru) {
    throw runtime_error("打开文件读取失败");
  }

  // 用ostringstream一次性读取全部内容
  ostringstream huanchong;
  huanchong << shuru.rdbuf();
  return huanchong.str();
}

// 写入文本文件
void write_text_file(const string& lujing, const string& neirong) {
  // 检查文件路径是否有目录部分
  filesystem::path wenjian_lujing(lujing);
  if (!wenjian_lujing.parent_path().empty()) {
    // 有目录就先创建目录
    filesystem::create_directories(wenjian_lujing.parent_path());
  }

  // 打开文件写入
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("打开文件写入失败");
  }
  shuchu << neirong;
}
