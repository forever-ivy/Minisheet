#pragma once

/**
 * @file m6_storage.h
 * @brief 存储持久化模块
 *
 * 本文件提供工作簿的导入导出功能：
 * - CSV 格式导入（兼容 Excel）
 * - 二进制 DAT 格式序列化/反序列化
 *
 * DAT 格式比 CSV 更快且保留更多信息。
 */

#include "minisheet/m2_workbook.h"

#include <string>
#include <vector>

namespace minisheet {

// ============================================================================
// CSV 导入
// ============================================================================

/**
 * @brief 从 CSV 文件加载工作簿
 *
 * 解析 CSV 文件并创建工作簿，支持引号包裹的字段和转义。
 *
 * @param lujing CSV 文件路径
 * @return 加载后的工作簿
 * @throw std::runtime_error 如果文件无法打开
 */
Workbook load_csv(const std::string& lujing);

// ============================================================================
// DAT 二进制格式
// ============================================================================

/**
 * @brief 将工作簿序列化为二进制数据
 *
 * DAT 文件格式：
 * - 4 bytes: 魔数 "MSHT"
 * - 4 bytes: 版本号 (uint32)
 * - 4 bytes: 单元格数量 (uint32)
 * - 每个单元格：
 *   - 2 bytes: 行号 (uint16)
 *   - 2 bytes: 列号 (uint16)
 *   - 1 byte:  类型 (uint8)
 *   - 4 bytes: 原始内容长度 (uint32)
 *   - N bytes: 原始内容
 *
 * @param gongzuobu 要序列化的工作簿
 * @return 二进制数据
 */
std::vector<char> serialize_workbook(const Workbook& gongzuobu);

/**
 * @brief 从二进制数据反序列化工作簿
 *
 * @param zijie_men 二进制数据
 * @return 反序列化后的工作簿
 * @throw std::runtime_error 如果数据格式无效
 */
Workbook deserialize_workbook(const std::vector<char>& zijie_men);

/**
 * @brief 保存工作簿为 DAT 文件
 * @param lujing 输出文件路径
 * @param gongzuobu 要保存的工作簿
 */
void save_dat(const std::string& lujing, const Workbook& gongzuobu);

/**
 * @brief 从 DAT 文件加载工作簿
 * @param lujing DAT 文件路径
 * @return 加载后的工作簿
 * @throw std::runtime_error 如果文件无法打开或格式无效
 */
Workbook load_dat(const std::string& lujing);

}  // namespace minisheet
