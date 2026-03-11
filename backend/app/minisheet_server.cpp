#include "minisheet/m6_storage.h"
#include "minisheet/m7_api.h"

#include "../../vendor/httplib.h"
#include "../../vendor/json.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  minisheet::Workbook workbook;
  httplib::Server server;
  server.set_default_headers({
      {"Access-Control-Allow-Origin", "*"},
      {"Access-Control-Allow-Headers", "Content-Type"},
      {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
  });

  server.Options(R"(.*)", [&](const httplib::Request&, httplib::Response& response) {
    response.status = 204;
  });

  server.Get("/api/snapshot", [&](const httplib::Request&, httplib::Response& response) {
    try {
      response.set_content(minisheet::workbook_snapshot_json(workbook), "application/json");
    } catch (const std::exception& error) {
      response.status = 500;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  server.Post("/api/cell", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      nlohmann::json body = nlohmann::json::parse(request.body);
      workbook.set_cell(body.at("cellId").get<std::string>(), body.at("raw").get<std::string>());
      workbook.recalculate_from(body.at("cellId").get<std::string>());
      response.set_content(minisheet::workbook_snapshot_json(workbook), "application/json");
    } catch (const std::exception& error) {
      response.status = 400;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  server.Post("/api/import-csv", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      if (!request.form.has_file("csv")) {
        response.status = 400;
        response.set_content(R"({"error":"missing csv file"})", "application/json");
        return;
      }

      auto file = request.form.get_file("csv");
      std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "minisheet_upload.csv";
      minisheet::write_text_file(temp_path.string(), file.content);
      workbook = minisheet::load_csv(temp_path.string());
      response.set_content(minisheet::workbook_snapshot_json(workbook), "application/json");
    } catch (const std::exception& error) {
      response.status = 400;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  server.Post("/api/restore-browser-draft", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      nlohmann::json body = nlohmann::json::parse(request.body);
      std::unordered_map<std::string, std::string> cells;

      if (body.contains("cells") && body.at("cells").is_object()) {
        for (auto it = body.at("cells").begin(); it != body.at("cells").end(); ++it) {
          if (!it.value().is_string()) {
            throw std::runtime_error("draft cells must be strings");
          }
          cells[it.key()] = it.value().get<std::string>();
        }
      }

      minisheet::restore_workbook_from_browser_draft(workbook, cells);
      response.set_content(minisheet::workbook_snapshot_json(workbook), "application/json");
    } catch (const std::exception& error) {
      response.status = 400;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  server.Post("/api/load-dat", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      if (!request.form.has_file("dat")) {
        response.status = 400;
        response.set_content(R"({"error":"missing dat file"})", "application/json");
        return;
      }

      auto file = request.form.get_file("dat");
      std::vector<char> bytes(file.content.begin(), file.content.end());
      workbook = minisheet::deserialize_workbook(bytes);
      response.set_content(minisheet::workbook_snapshot_json(workbook), "application/json");
    } catch (const std::exception& error) {
      response.status = 400;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  server.Post("/api/save-dat", [&](const httplib::Request&, httplib::Response& response) {
    try {
      std::vector<char> bytes = minisheet::serialize_workbook(workbook);
      response.set_header("Content-Disposition", "attachment; filename=\"workbook.dat\"");
      response.set_content(std::string(bytes.begin(), bytes.end()), "application/octet-stream");
    } catch (const std::exception& error) {
      response.status = 500;
      response.set_content(nlohmann::json({{"error", error.what()}}).dump(), "application/json");
    }
  });

  std::cout << "minisheet_server listening on http://127.0.0.1:8080\n";
  server.listen("127.0.0.1", 8080);
  return 0;
}
