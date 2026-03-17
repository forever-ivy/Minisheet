// ========================================
// 文件存储实现文件
// 支持两种格式：CSV（文本，兼容Excel）和DAT（二进制，我们的私有格式）
// 它位于“文件世界”和“Workbook 内存世界”之间：
// - 负责把磁盘格式翻译成 Workbook
// - 但不自己计算公式，加载完成后会回到 m2::recalculate_all 触发重算
// ========================================

#include "minisheet/m6_storage.h"

#include "minisheet/m1_types.h"  // 基础类型

#include <algorithm>  // max函数
#include <cstdint>    // uint32_t等固定大小整数类型
#include <cstring>    // memcpy
#include <fstream>    // 文件流
#include <iterator>   // istreambuf_iterator
#include <stdexcept>  // 异常

using namespace std;

// DAT文件的魔数（文件头标识）
// 用"MSHT"代表MiniSpreadSHeet
constexpr char kMagic[4] = {'M', 'S', 'H', 'T'};

// DAT文件版本号
// 版本1：只有单元格数据
// 版本2：增加了CSV行列数信息
constexpr uint32_t kVersion = 2;

// 模板函数：把任意类型追加到字节数组
// 用于序列化，直接把内存里的二进制数据拷进去
// 注意：这种方式是平台相关的，不同CPU架构可能不兼容
// 但作为学生作业，这样写最简单
// T可以是int、float、char等基本类型
// zijie_men是目标字节数组
// zhi是要写入的值
// 函数内部用reinterpret_cast把值的内存地址转成char指针
// 然后用insert把字节拷进去
// 比如 append_value<uint32_t>(vec, 123) 会在vec后面加4个字节
// 这4个字节就是123的内存表示（小端序）
template <typename T>
void append_value(vector<char>& zijie_men, T zhi) {
  const char* yuanshi = reinterpret_cast<const char*>(&zhi);
  zijie_men.insert(zijie_men.end(), yuanshi, yuanshi + sizeof(T));
}

// 模板函数：从字节数组读取任意类型
// 用于反序列化
// pianyi是偏移量，读取后会自动增加
// 如果越界会抛异常
template <typename T>
T read_value(const vector<char>& zijie_men, size_t& pianyi) {
  // 检查是否有足够的字节
  if (pianyi + sizeof(T) > zijie_men.size()) {
    throw runtime_error("corrupt dat file");
  }

  T zhi {};  // 零初始化
  memcpy(&zhi, zijie_men.data() + pianyi, sizeof(T));
  pianyi += sizeof(T);  // 偏移量增加
  return zhi;
}

// 解析CSV的一行
// CSV格式比较复杂，要考虑：
// 1. 普通字段：abc,def,ghi
// 2. 带逗号的字段："abc,def",ghi（用引号包裹）
// 3. 带引号的字段："abc""def"（两个引号表示一个引号）
// 4. Windows换行符\r\n，这里要处理\r
vector<string> parse_csv_row(const string& yi_hang) {
  // 先处理Windows换行符，如果行尾有\r就删掉
  string guifanhou = yi_hang;
  if (!guifanhou.empty() && guifanhou.back() == '\r') {
    guifanhou.pop_back();
  }

  vector<string> zhiduan_men;  // 存解析出的字段
  string dangqian_zhiduan;     // 当前正在解析的字段
  bool zai_yinhao_nei = false; // 标记是否在引号内

  // 遍历每个字符
  for (size_t xiabiao = 0; xiabiao < guifanhou.size(); ++xiabiao) {
    char zifu = guifanhou[xiabiao];

    if (zifu == '"') {
      // 遇到引号
      if (zai_yinhao_nei && xiabiao + 1 < guifanhou.size() && guifanhou[xiabiao + 1] == '"') {
        // 在引号内又遇到引号，而且是两个连续的引号
        // 这是转义的引号，表示一个真正的引号字符
        dangqian_zhiduan.push_back('"');
        xiabiao += 1;  // 跳过下一个引号
      } else {
        // 切换引号状态（进入或退出引号模式）
        zai_yinhao_nei = !zai_yinhao_nei;
      }
    } else if (zifu == ',' && !zai_yinhao_nei) {
      // 遇到逗号，而且不在引号内，表示字段结束
      zhiduan_men.push_back(dangqian_zhiduan);
      dangqian_zhiduan.clear();
    } else {
      // 普通字符，加入当前字段
      dangqian_zhiduan.push_back(zifu);
    }
  }

  // 最后一个字段（行尾没有逗号）
  zhiduan_men.push_back(dangqian_zhiduan);
  return zhiduan_men;
}

// 从CSV文件加载工作簿
Workbook load_csv(const string& lujing) {
  ifstream shuru(lujing);
  if (!shuru) {
    throw runtime_error("failed to open csv");
  }

  Workbook gongzuobu;
  string yi_hang;
  int hang = 1;        // 当前行号，从1开始
  int zui_da_lie = 0;  // 记录最大列数

  // 逐行读取
  while (getline(shuru, yi_hang)) {
    // 解析这一行
    vector<string> zhiduan_men = parse_csv_row(yi_hang);

    // 更新最大列数
    zui_da_lie = max(zui_da_lie, static_cast<int>(zhiduan_men.size()));

    // 把每个非空字段写入对应的单元格。
    // 注意这里只是逐格调用 m2::set_cell 写入原始内容，还没有真正做公式依赖计算。
    for (size_t lie = 0; lie < zhiduan_men.size() && lie < static_cast<size_t>(kMaxColumns); ++lie) {
      if (!zhiduan_men[lie].empty()) {
        // 列号要+1，因为CSV的列是从0开始，但我们的单元格ID从1开始
        set_cell(gongzuobu, to_cell_id({hang, static_cast<int>(lie) + 1}), zhiduan_men[lie]);
      }
    }

    hang += 1;
    // 行数太多就停止
    if (hang > kMaxRows) {
      break;
    }
  }

  // 记录原始CSV的尺寸
  gongzuobu.yuan_csv_hang_shu = min(hang - 1, kMaxRows);
  gongzuobu.yuan_csv_lie_shu = min(zui_da_lie, kMaxColumns);

  // 文件内容全部写入后，统一触发一次完整重算。
  // 这会进入 m2 -> m5 -> m4，把 CSV 中出现的公式都算出来。
  recalculate_all(gongzuobu);
  return gongzuobu;
}

// 转义CSV字段
// 如果字段包含逗号、引号、换行，就要用引号包裹
// 字段内的引号要双写（""表示一个引号）
string escape_csv_field(const string& yuanshi) {
  // 先检查是否需要转义
  bool xuyao_yinhao = false;
  for (char zifu : yuanshi) {
    if (zifu == ',' || zifu == '"' || zifu == '\n' || zifu == '\r') {
      xuyao_yinhao = true;
      break;
    }
  }

  // 不需要转义就直接返回
  if (!xuyao_yinhao) {
    return yuanshi;
  }

  // 需要转义，用引号包裹
  string zhuanyi = "\"";
  for (char zifu : yuanshi) {
    if (zifu == '"') {
      // 引号要双写
      zhuanyi += "\"\"";
    } else {
      zhuanyi.push_back(zifu);
    }
  }
  zhuanyi.push_back('"');
  return zhuanyi;
}

// 计算工作簿的尺寸（行数和列数）
// 取原始CSV尺寸和实际单元格的最大值
pair<int, int> workbook_shape(const Workbook& gongzuobu) {
  int hang_shu = gongzuobu.yuan_csv_hang_shu;
  int lie_shu = gongzuobu.yuan_csv_lie_shu;

  // 遍历所有单元格，看最大的行列号
  for (const string& danyuange_id : ordered_cell_ids(gongzuobu)) {
    CellCoord zuobiao = parse_cell_id(danyuange_id);
    hang_shu = max(hang_shu, zuobiao.hang);
    lie_shu = max(lie_shu, zuobiao.lie);
  }

  return {hang_shu, lie_shu};
}

// 把工作簿保存为CSV文件
void save_csv(const string& lujing, const Workbook& gongzuobu) {
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open csv for writing");
  }

  // 获取工作簿尺寸
  pair<int, int> chicun = workbook_shape(gongzuobu);

  // 逐行输出
  for (int hang = 1; hang <= chicun.first; ++hang) {
    for (int lie = 1; lie <= chicun.second; ++lie) {
      // 列之间用逗号分隔
      if (lie > 1) {
        shuchu << ',';
      }

      // 获取单元格的原始内容（不是显示值）
      const CellRecord& danyuange = cell(gongzuobu, to_cell_id({hang, lie}));
      // 转义并输出
      shuchu << escape_csv_field(danyuange.yuanshi);
    }
    // 行尾换行
    shuchu << '\n';
  }
}

// 把工作簿序列化成字节数组（DAT格式）
vector<char> serialize_workbook(const Workbook& gongzuobu) {
  vector<char> zijie_men;

  // 1. 写魔数（4字节）
  zijie_men.insert(zijie_men.end(), kMagic, kMagic + 4);

  // 2. 写版本号（4字节）
  append_value<uint32_t>(zijie_men, kVersion);

  // 3. 写原始CSV行列数（各4字节）
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(max(gongzuobu.yuan_csv_hang_shu, 0)));
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(max(gongzuobu.yuan_csv_lie_shu, 0)));

  // 4. 写单元格数量（4字节）
  vector<string> id_men = ordered_cell_ids(gongzuobu);
  append_value<uint32_t>(zijie_men, static_cast<uint32_t>(id_men.size()));

  // 5. 逐个写单元格
  for (const string& danyuange_id : id_men) {
    const CellRecord& danyuange = cell(gongzuobu, danyuange_id);
    CellCoord zuobiao = parse_cell_id(danyuange_id);

    // 写行号（2字节，uint16_t足够存32767）
    append_value<uint16_t>(zijie_men, static_cast<uint16_t>(zuobiao.hang));
    // 写列号（2字节）
    append_value<uint16_t>(zijie_men, static_cast<uint16_t>(zuobiao.lie));
    // 写类型（1字节）
    append_value<uint8_t>(zijie_men, static_cast<uint8_t>(danyuange.leixing));
    // 写原始内容长度（4字节）
    append_value<uint32_t>(zijie_men, static_cast<uint32_t>(danyuange.yuanshi.size()));
    // 写原始内容（变长）
    zijie_men.insert(zijie_men.end(), danyuange.yuanshi.begin(), danyuange.yuanshi.end());
  }

  return zijie_men;
}

// 从字节数组反序列化工作簿
Workbook deserialize_workbook(const vector<char>& zijie_men) {
  // 检查文件大小（至少要能放下魔数和版本号）
  if (zijie_men.size() < 12) {
    throw runtime_error("dat file too small");
  }

  // 检查魔数
  if (!equal(zijie_men.begin(), zijie_men.begin() + 4, kMagic)) {
    throw runtime_error("invalid dat header");
  }

  size_t pianyi = 4;  // 从魔数后面开始读

  // 读版本号
  uint32_t banben = read_value<uint32_t>(zijie_men, pianyi);
  if (banben != 1 && banben != kVersion) {
    throw runtime_error("unsupported dat version");
  }

  Workbook gongzuobu;

  // 版本2以上才有CSV行列数
  if (banben >= 2) {
    gongzuobu.yuan_csv_hang_shu = static_cast<int>(read_value<uint32_t>(zijie_men, pianyi));
    gongzuobu.yuan_csv_lie_shu = static_cast<int>(read_value<uint32_t>(zijie_men, pianyi));
  }

  // 读单元格数量
  uint32_t ge_shu = read_value<uint32_t>(zijie_men, pianyi);

  // 逐个读单元格
  for (uint32_t xuhao = 0; xuhao < ge_shu; ++xuhao) {
    uint16_t hang = read_value<uint16_t>(zijie_men, pianyi);
    uint16_t lie = read_value<uint16_t>(zijie_men, pianyi);
    uint8_t leixing = read_value<uint8_t>(zijie_men, pianyi);  // 目前没用，但留着以后用
    uint32_t yuanshi_changdu = read_value<uint32_t>(zijie_men, pianyi);

    // 检查长度是否合法
    if (pianyi + yuanshi_changdu > zijie_men.size()) {
      throw runtime_error("corrupt dat record");
    }

    // 读取原始内容
    string yuanshi(zijie_men.data() + pianyi, zijie_men.data() + pianyi + yuanshi_changdu);
    pianyi += yuanshi_changdu;

    (void)leixing;  // 暂时不用类型字段，用set_cell会自动判断

    // 设置单元格
    set_cell(gongzuobu, to_cell_id({static_cast<int>(hang), static_cast<int>(lie)}), yuanshi);
  }

  // 重新计算所有公式
  recalculate_all(gongzuobu);
  return gongzuobu;
}

// 保存DAT文件
void save_dat(const string& lujing, const Workbook& gongzuobu) {
  // 先序列化成字节数组
  vector<char> zijie_men = serialize_workbook(gongzuobu);

  // 写入文件
  ofstream shuchu(lujing, ios::binary);
  if (!shuchu) {
    throw runtime_error("failed to open dat for writing");
  }
  shuchu.write(zijie_men.data(), static_cast<streamsize>(zijie_men.size()));
}

// 加载DAT文件
Workbook load_dat(const string& lujing) {
  // 打开文件
  ifstream shuru(lujing, ios::binary);
  if (!shuru) {
    throw runtime_error("failed to open dat for reading");
  }

  // 读取全部内容到vector
  // 这里用迭代器方式，比较简洁
  vector<char> zijie_men((istreambuf_iterator<char>(shuru)), istreambuf_iterator<char>());

  // 反序列化
  return deserialize_workbook(zijie_men);
}
