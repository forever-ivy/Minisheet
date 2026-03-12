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

int parse_port_text(const char* wenben) {
  if (wenben == nullptr) {
    return 0;
  }

  try {
    int duankou = stoi(wenben);
    if (duankou > 0 && duankou <= 65535) {
      return duankou;
    }
  } catch (...) {
  }

  return 0;
}

void set_json_error(httplib::Response& xiangying, int zhuangtai, const string& cuowu) {
  xiangying.status = zhuangtai;
  xiangying.set_content(nlohmann::json({{"error", cuowu}}).dump(), "application/json");
}

void set_json_response(httplib::Response& xiangying, const string& neirong) {
  xiangying.set_content(neirong, "application/json");
}

int read_port(int argc, char** argv) {
  if (argc >= 2) {
    int duankou = parse_port_text(argv[1]);
    if (duankou != 0) {
      return duankou;
    }
  }

  int duankou = parse_port_text(getenv("MINISHEET_PORT"));
  if (duankou != 0) {
    return duankou;
  }

  return 8080;
}

void handle_options(const httplib::Request&, httplib::Response& xiangying) {
  xiangying.status = 204;
}

void handle_snapshot(Workbook& gongzuobu, const httplib::Request&, httplib::Response& xiangying) {
  try {
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 500, cuowu.what());
  }
}

void handle_cell(Workbook& gongzuobu,
                 const httplib::Request& qingqiu,
                 httplib::Response& xiangying) {
  try {
    nlohmann::json qingqiu_ti = nlohmann::json::parse(qingqiu.body);
    set_cell(gongzuobu, qingqiu_ti.at("cellId").get<string>(), qingqiu_ti.at("raw").get<string>());
    recalculate_all(gongzuobu);
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());
  }
}

void handle_import_csv(Workbook& gongzuobu,
                       const httplib::Request& qingqiu,
                       httplib::Response& xiangying) {
  try {
    if (!qingqiu.form.has_file("csv")) {
      set_json_error(xiangying, 400, "missing csv file");
      return;
    }

    auto wenjian = qingqiu.form.get_file("csv");
    filesystem::path linshi_lujing = filesystem::temp_directory_path() / "minisheet_upload.csv";
    write_text_file(linshi_lujing.string(), wenjian.content);
    gongzuobu = load_csv(linshi_lujing.string());
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());
  }
}

void handle_load_dat(Workbook& gongzuobu,
                     const httplib::Request& qingqiu,
                     httplib::Response& xiangying) {
  try {
    if (!qingqiu.form.has_file("dat")) {
      set_json_error(xiangying, 400, "missing dat file");
      return;
    }

    auto wenjian = qingqiu.form.get_file("dat");
    vector<char> zijie_men(wenjian.content.begin(), wenjian.content.end());
    gongzuobu = deserialize_workbook(zijie_men);
    set_json_response(xiangying, workbook_snapshot_json(gongzuobu));
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 400, cuowu.what());
  }
}

void handle_save_dat(Workbook& gongzuobu, const httplib::Request&, httplib::Response& xiangying) {
  try {
    vector<char> zijie_men = serialize_workbook(gongzuobu);
    xiangying.set_header("Content-Disposition", "attachment; filename=\"workbook.dat\"");
    xiangying.set_content(string(zijie_men.begin(), zijie_men.end()), "application/octet-stream");
  } catch (const exception& cuowu) {
    set_json_error(xiangying, 500, cuowu.what());
  }
}

int main(int argc, char** argv) {
  Workbook gongzuobu;
  httplib::Server fuwuqi;
  int duankou = read_port(argc, argv);

  fuwuqi.set_default_headers({
      {"Access-Control-Allow-Origin", "*"},
      {"Access-Control-Allow-Headers", "Content-Type"},
      {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
  });

  fuwuqi.Options(R"(.*)", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_options(qingqiu, xiangying);
  });

  fuwuqi.Get("/api/snapshot", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_snapshot(gongzuobu, qingqiu, xiangying);
  });

  fuwuqi.Post("/api/cell", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_cell(gongzuobu, qingqiu, xiangying);
  });

  fuwuqi.Post("/api/import-csv",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_import_csv(gongzuobu, qingqiu, xiangying);
  });

  fuwuqi.Post("/api/load-dat",
              [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_load_dat(gongzuobu, qingqiu, xiangying);
  });

  fuwuqi.Post("/api/save-dat", [&](const httplib::Request& qingqiu, httplib::Response& xiangying) {
    handle_save_dat(gongzuobu, qingqiu, xiangying);
  });

  cout << "minisheet_server listening on http://127.0.0.1:" << duankou << "\n";
  fuwuqi.listen("127.0.0.1", duankou);
  return 0;
}
