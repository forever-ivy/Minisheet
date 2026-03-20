#pragma once

// ========================================
// 文件存储模块
// 负责把电子表格保存到文件或从文件加载
// 支持CSV格式（文本，兼容Excel）和DAT格式（二进制，我们自己的压缩格式）
// 它是典型的“边界模块”：
// - 上游：CLI 和 Server 都会直接调用它
// - 下游：加载时会调用 m2::set_cell 把数据写入 Workbook，
//        然后调用 m2::recalculate_all 触发 m5 -> m4 完成公式计算
// 所以导入文件并不只是读文件，还会顺带跑完整的重算流程
// ========================================

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

using namespace std;

// 从CSV文件加载工作簿
// lujing是文件路径
// 返回加载好的工作簿对象
// 会自动解析CSV格式，处理逗号分隔和引号包裹的字段
Workbook load_csv(const string& lujing);

// 把工作簿保存为CSV文件
// 会把所有单元格按行列顺序写入CSV
// 空单元格会输出为空字段
void save_csv(const string& lujing, const Workbook& gongzuobu);

// 把工作簿序列化成字节数组
// 这是DAT格式的内部表示，用于网络传输或保存二进制文件
// 当前格式是版本3：魔数(4字节) + 版本号(4字节) + 原始长度(4字节) + zlib压缩数据体
vector<char> serialize_workbook(const Workbook& gongzuobu);

// 从字节数组反序列化工作簿
// 是上面那个函数的反向操作
Workbook deserialize_workbook(const vector<char>& zijie_men);

// 把工作簿保存为DAT二进制文件
// 内部调用serialize_workbook然后写入文件
void save_dat(const string& lujing, const Workbook& gongzuobu);

// 从DAT二进制文件加载工作簿
// 内部读取文件然后调用deserialize_workbook
Workbook load_dat(const string& lujing);
