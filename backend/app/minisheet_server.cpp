// ========================================
// MiniSheet HTTP服务器程序
// 这是一个基于httplib的简易HTTP服务器
// 提供REST API给前端调用
// 功能包括：获取表格快照、修改单元格、导入导出文件等
// ========================================

#include "../../vendor/httplib.h"  // 第三方HTTP库
#include "../../vendor/json.hpp"   // 第三方JSON库

#include "minisheet/m6_storage.h"  // 文件存储功能
#include "minisheet/m7_api.h"      // JSON API

#include <cstdlib>     // getenv, atoi等
#include <exception>   // 异常处理
#include <filesystem>  // 文件系统操作
#include <iostream>    // 输入输出
#include <string>      // 字符串
#include <vector>      // 动态数组

using namespace std;

// 解析端口字符串
// 检查字符串是否是合法的端口号（1-65535）
// 合法返回端口号，不合法返回0
int parse_port_text(const char* wenben) {
  if (wenben == nullptr) {
    return 0;  // 空指针不合法
  }

  try {
    int duankou = stoi(wenben);  // 转成整数
    if (duankou > 0 && duankou <= 65535) {
      return duankou;  // 在有效范围内
    }
  } catch (...) {
    // 转换失败（比如包含非数字字符）
  }

  return 0;  // 不合法
}

// 设置JSON格式的错误响应
// zhuangtai是HTTP状态码（如400、500）
// cuowu是错误信息字符串
void set_json_error(httplib::Response& xiangying, int zhuangtai, const string& cuowu) {
  xiangying.status = zhuangtai;
  xiangying.set_content(nlohmann::json({{"error", cuowu}}).dump(), "application/json");
}

// 设置JSON格式的成功响应
void set_json_response(httplib::Response& xiangying, const string& neirong) {
  xiangying.set_content(neirong, "application/json");
}

// 读取服务器要监听的端口号
// 优先级：命令行参数 > 环境变量MINISHEET_PORT > 默认值8080
int read_port(int argc, char** argv) {
  // 先看命令行参数
  if (argc >= 2) {
    int duankou = parse_port_text(argv[1]);
    if (duankou != 0) {
      return duankou;
    }
  }

  // 再看环境变量
  int duankou = parse_port_text(getenv("MINISHEET_PORT"));
  if (duankou != 0) {
    return duankou;
  }

  // 都用默认的8080
  return 8080;
}

// 处理OPTIONS请求
// 这是CORS预检请求，返回允许的跨域设置
void handle_options(const httplib::Request&, httplib::Response& xiangying) {
  xiangying.status = 204;  // No Content
}

// 处理GET /api/snapshot 请求
// 返回整个工作簿的当前状态（JSON格式）
void handle_snapshot(Workbook& gongzuobu, const httplib::Request&, httplib::Response& xiangying) {
  try {
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 500, cuowu.what());  // 服务器内部错误
  }
}

// 处理POST /api/cell 请求
// 修改某个单元格的内容
// 请求体应该是JSON格式：{"cellId": "A1", "raw": "=B1+1"}
void handle_cell(Workbook& gongzuobu,
                 const httplib::Request& qingqiu,
                 httplib::Response& xiangying) {
  try {
    // 解析请求JSON
    nlohmann::json qingqiu_ti = nlohmann::json::parse(qingqiu.body);

    // 设置单元格（at会检查字段是否存在，不存在会抛异常）
    set_cell(gongzuobu, qingqiu_ti.at("cellId").get<string>(), qingqiu_ti.at("raw").get<string>());

    // 重新计算所有公式
    recalculate_all(gongzuobu);

    // 返回新的工作簿状态
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());  // 客户端错误（参数不对）
  }
}

// 处理POST /api/import-csv 请求
// 导入CSV文件（multipart/form-data格式）
void handle_import_csv(Workbook& gongzuobu,
                       const httplib::Request& qingqiu,
                       httplib::Response& xiangying) {
  try {
    // 检查是否有csv文件字段
    if (!qingqiu.form.has_file("csv")) {
      set_json_error(xiangying, 400, "missing csv file");
      return;
    }

    // 获取上传的文件
    auto wenjian = qingqiu.form.get_file("csv");

    // 保存到临时文件
    filesystem::path linshi_lujing = filesystem::temp_directory_path() / "minisheet_upload.csv";
    write_text_file(linshi_lujing.string(), wenjian.content);

    // 加载CSV（会清空原有内容）
    gongzuobu = load_csv(linshi_lujing.string());

    // 返回新的工作簿状态
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());
  }
}

// 处理POST /api/load-dat 请求
// 加载二进制DAT文件
void handle_load_dat(Workbook& gongzuobu,
                     const httplib::Request& qingqiu,
                     httplib::Response& xiangying) {
  try {
    // 检查是否有dat文件字段
    if (!qingqiu.form.has_file("dat")) {
      set_json_error(xiangying, 400, "missing dat file");
      return;
    }

    // 获取上传的文件
    auto wenjian = qingqiu.form.get_file("dat");

    // 转成字节数组
    vector<char> zijie_men(wenjian.content.begin(), wenjian.content.end());

    // 反序列化（会清空原有内容）
    gongzuobu = deserialize_workbook(zijie_men);

    // 返回新的工作簿状态
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());
  }
}

// 处理POST /api/save-dat 请求
// 下载当前工作簿为DAT文件
void handle_save_dat(Workbook& gongzuobu, const httplib::Request&, httplib::Response& xiangying) {
  try {
    // 序列化成字节数组
    vector<char> zijie_men = serialize_workbook(gongzuobu);

    // 设置下载头
    xiangying.set_header("Content-Disposition", "attachment; filename=\"workbook.dat\"");

    // 返回二进制内容
    xiangying.set_content(string(zijie_men.begin(), zijie_men.end()), "application/octet-stream");
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 500, cuowu.what());
  }
}

// 主函数
int main(int argc, char** argv) {
  Workbook gongzuobu;        // 工作簿对象，程序运行期间一直存在
  httplib::Server fuwuqi;    // HTTP服务器对象
  int duankou = read_port(argc, argv);  // 读取端口号

  // 设置CORS（跨域）头，允许前端从任何域名访问
  fuwuqi.set_default_headers({
      {"Access-Control-Allow-Origin", "*"},           // 允许任何来源
      {"Access-Control-Allow-Headers", "Content-Type"},  // 允许Content-Type头
      {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},  // 允许的方法
  });

  // 注册OPTIONS路由（处理CORS预检）
  fuwuqi.Options(R"(.*)", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_options(qingqiu, xiangying);
  });

  // 注册GET /api/snapshot 路由
  fuwuqi.Get("/api/snapshot", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_snapshot(gongzuobu, qingqiu, xiangying);
  });

  // 注册POST /api/cell 路由
  fuwuqi.Post("/api/cell", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_cell(gongzuobu, qingqiu, xiangying);
  });

  // 注册POST /api/import-csv 路由
  fuwuqi.Post("/api/import-csv",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_import_csv(gongzuobu, qingqiu, xiangying);
  });

  // 注册POST /api/load-dat 路由
  fuwuqi.Post("/api/load-dat",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_load_dat(gongzuobu, qingqiu, xiangying);
  });

  // 注册POST /api/save-dat 路由
  fuwuqi.Post("/api/save-dat", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_save_dat(gongzuobu, qingqiu, xiangying);
  });

  // 打印启动信息
  cout << "minisheet_server listening on http://127.0.0.1:" << duankou << "\n";

  // 启动服务器（阻塞调用，一直运行直到程序结束）
  fuwuqi.listen("127.0.0.1", duankou);
  return 0;
}
