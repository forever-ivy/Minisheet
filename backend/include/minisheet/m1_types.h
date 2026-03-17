#pragma once

// ========================================
// 这是整个电子表格程序的基础类型定义文件
// 里面放的是所有模块都会用到的基本数据结构
// 可以把它看成最底层，基本不依赖业务模块：
// - m2_workbook 依赖它定义CellCoord / CellKind / 各种解析函数
// - m4_formula 依赖它做单元格ID解析、trim、数字格式化
// - m6_storage 依赖它做文件读写、坐标转换
// 自己不依赖 m2~m7，所以读代码时可以把 m1 当作地基层
// ========================================

#include <string>

using namespace std;

// 电子表格的最大尺寸限制
// 行数最多32767行（跟Excel2003一样）
// 列数最多256列（A到IV）
// 这些常量会被 m6_storage / m7_api / 前端接口一起复用
constexpr int kMaxRows = 32767;
constexpr int kMaxColumns = 256;

// 单元格的类型枚举
// Empty: 空的，什么都没输入
// Integer: 整数，比如 123
// Float: 小数，比如 3.14
// String: 文字，比如 "hello"
// Formula: 公式，比如 =A1+B1
enum class CellKind {
  Empty,
  Integer,
  Float,
  String,
  Formula,
};

// 单元格坐标结构体
// 用行号和列号来定位一个单元格
// 注意：这里的行和列都是从1开始计数的，不是从0开始
struct CellCoord {
  int hang = 0;  // 行号，比如A1中的1
  int lie = 0;   // 列号，比如A1中的1（代表A列）
};

// 单元格范围结构体
// 用来表示一个矩形区域，比如 A1:B10
// qishi是左上角，jieshu是右下角
struct CellRange {
  CellCoord qishi;  // 起始单元格
  CellCoord jieshu; // 结束单元格
};

// 把列号（1-256）转换成列名（A-IV）
// 比如 1->"A", 2->"B", 27->"AA"
string column_index_to_name(int yiji_lie);

// 把列名（A-IV）转换成列号（1-256）
// 是上面那个函数的反向操作
int column_name_to_index(const string& lie_ming);

// 解析单元格ID字符串，比如把"A1"解析成{hang=1, lie=1}
// 会检查格式是否正确，不对就抛异常
CellCoord parse_cell_id(const string& danyuange_id);

// 把坐标结构体转换回字符串ID
// 比如 {hang=1, lie=1} 转换成 "A1"
string to_cell_id(const CellCoord& zuobiao);

// 检查坐标是否在合法范围内
// 行号要在1到kMaxRows之间，列号要在1到kMaxColumns之间
bool is_valid_coord(const CellCoord& zuobiao);

// 检查单元格ID字符串是否合法
// 比如 "A1" 合法，"1A" 不合法，"XFD1" 也不合法（列太多了）
bool is_valid_cell_id(const string& danyuange_id);

// 去掉字符串前后的空白字符
// 比如 "  hello  " 变成 "hello"
string trim(const string& zhi);

// 尝试把字符串解析成数字
// 如果能解析成功返回true，zhi参数得到数值，shi_zhengshu参数告诉你是整数还是小数
// 解析失败返回false
bool try_parse_number(const string& yuanshi, double& zhi, bool& shi_zhengshu);

// 把数字格式化成字符串
// 整数就显示整数，小数会去掉后面多余的0
// 比如 3.14000 显示成 "3.14"，123.0 显示成 "123"
string format_number(double zhi);

// 读取整个文本文件的内容
// 直接按二进制方式读，不会转换换行符
string read_text_file(const string& lujing);

// 把字符串写入文件
// 如果目录不存在会自动创建目录
void write_text_file(const string& lujing, const string& neirong);
