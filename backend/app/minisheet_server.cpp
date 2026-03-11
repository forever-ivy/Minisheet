/**
 * @file minisheet_server.cpp
 * @brief HTTP API 服务器入口
 *
 * 提供 RESTful API 用于前端交互：
 * - GET  /api/snapshot   - 获取工作簿快照
 * - POST /api/cell       - 设置单元格内容
 * - POST /api/import-csv - 导入 CSV 文件
 * - POST /api/load-dat   - 加载 DAT 文件
 * - POST /api/save-dat   - 保存为 DAT 文件
 *
 * 端口优先级：命令行参数 > MINISHEET_PORT 环境变量 > 默认 8080
 */

#include "minisheet/m6_storage.h"
#include "minisheet/m7_api.h"

#include "../../vendor/httplib.h"
#include "../../vendor/json.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace {

// ----------------------------------------------------------------------------
// 解析端口号字符串
// 返回：有效端口号（1-65535）或 0（无效）
// ----------------------------------------------------------------------------
int parse_port_value(const string& wenben) {
  try {
    int duankou = stoi(wenben);
    if (duankou > 0 && duankou <= 65535) {
      return duankou;
    }
  } catch (...) {
  }

  return 0;
}

// ----------------------------------------------------------------------------
// 读取服务器端口
// 优先级：命令行参数 > 环境变量 MINISHEET_PORT > 默认 8080
// ----------------------------------------------------------------------------
int read_port(int argc, char** argv) {
  if (argc >= 2) {
    int duankou = parse_port_value(argv[1]);
    if (duankou != 0) {
      return duankou;
    }
  }

  const char* huanjing_duankou = getenv("MINISHEET_PORT");
  if (huanjing_duankou != nullptr) {
    int duankou = parse_port_value(huanjing_duankou);
    if (duankou != 0) {
      return duankou;
    }
  }

  return 8080;
}

}  // namespace

// ----------------------------------------------------------------------------
// HTTP 服务器主函数
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
  minisheet::Workbook gongzuobu;
  httplib::Server fuwuqi;
  int duankou = read_port(argc, argv);

  // 设置 CORS 头，允许前端跨域访问
  fuwuqi.set_default_headers({
      {"Access-Control-Allow-Origin", "*"},
      {"Access-Control-Allow-Headers", "Content-Type"},
      {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
  });

  // 处理预检请求
  fuwuqi.Options(R"(.*)", [&](const httplib::Request&, httplib::Response& xiangying) {
    xiangying.status = 204;
  });

  // ----------------------------------------------------------------------------
  // GET /api/snapshot - 获取工作簿完整快照
  // 返回：包含所有单元格和元数据的 JSON
  // ----------------------------------------------------------------------------
  fuwuqi.Get("/api/snapshot", [&](const httplib::Request&, httplib::Response& xiangying) {
    try {
      xiangying.set_content(minisheet::workbook_snapshot_json(gongzuobu), "application/json");
    } catch (const std::exception& cuowu) {
      xiangying.status = 500;
      xiangying.set_content(nlohmann::json({{"error", cuowu.what()}}).dump(), "application/json");
    }
  });

  // ----------------------------------------------------------------------------
  // POST /api/cell - 设置单元格内容
  // 请求体：{"cellId": "A1", "raw": "=B1+C1"}
  // 返回：更新后的工作簿快照
  // ----------------------------------------------------------------------------
  fuwuqi.Post("/api/cell", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    try {
      nlohmann::json qingqiu_ti = nlohmann::json::parse(qingqiu.body);
      gongzuobu.set_cell(qingqiu_ti.at("cellId").get<std::string>(),
                         qingqiu_ti.at("raw").get<std::string>());
      gongzuobu.recalculate_all();
      xiangying.set_content(minisheet::workbook_snapshot_json(gongzuobu), "application/json");
    } catch (const std::exception& cuowu) {
      xiangying.status = 400;
      xiangying.set_content(nlohmann::json({{"error", cuowu.what()}}).dump(), "application/json");
    }
  });

  // ----------------------------------------------------------------------------
  // POST /api/import-csv - 导入 CSV 文件
  // 上传文件字段名：csv
  // ----------------------------------------------------------------------------
  fuwuqi.Post("/api/import-csv",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    try {
      if (!qingqiu.form.has_file("csv")) {
        xiangying.status = 400;
        xiangying.set_content(R"({"error":"missing csv file"})", "application/json");
        return;
      }

      auto wenjian = qingqiu.form.get_file("csv");
      std::filesystem::path linshi_lujing =
          std::filesystem::temp_directory_path() / "minisheet_upload.csv";
      minisheet::write_text_file(linshi_lujing.string(), wenjian.content);
      gongzuobu = minisheet::load_csv(linshi_lujing.string());
      xiangying.set_content(minisheet::workbook_snapshot_json(gongzuobu), "application/json");
    } catch (const std::exception& cuowu) {
      xiangying.status = 400;
      xiangying.set_content(nlohmann::json({{"error", cuowu.what()}}).dump(), "application/json");
    }
  });

  // ----------------------------------------------------------------------------
  // POST /api/load-dat - 加载 DAT 文件
  // 上传文件字段名：dat
  // ----------------------------------------------------------------------------
  fuwuqi.Post("/api/load-dat",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    try {
      if (!qingqiu.form.has_file("dat")) {
        xiangying.status = 400;
        xiangying.set_content(R"({"error":"missing dat file"})", "application/json");
        return;
      }

      auto wenjian = qingqiu.form.get_file("dat");
      vector<char> zijie_men(wenjian.content.begin(), wenjian.content.end());
      gongzuobu = minisheet::deserialize_workbook(zijie_men);
      xiangying.set_content(minisheet::workbook_snapshot_json(gongzuobu), "application/json");
    } catch (const std::exception& cuowu) {
      xiangying.status = 400;
      xiangying.set_content(nlohmann::json({{"error", cuowu.what()}}).dump(), "application/json");
    }
  });

  // ----------------------------------------------------------------------------
  // POST /api/save-dat - 保存为 DAT 文件
  // 返回：二进制 DAT 文件（下载）
  // ----------------------------------------------------------------------------
  fuwuqi.Post("/api/save-dat", [&](const httplib::Request&, httplib::Response& xiangying) {
    try {
      vector<char> zijie_men = minisheet::serialize_workbook(gongzuobu);
      xiangying.set_header("Content-Disposition", "attachment; filename=\"workbook.dat\"");
      xiangying.set_content(string(zijie_men.begin(), zijie_men.end()), "application/octet-stream");
    } catch (const std::exception& cuowu) {
      xiangying.status = 500;
      xiangying.set_content(nlohmann::json({{"error", cuowu.what()}}).dump(), "application/json");
    }
  });

  cout << "minisheet_server listening on http://127.0.0.1:" << duankou << "\n";
  fuwuqi.listen("127.0.0.1", duankou);
  return 0;
}
