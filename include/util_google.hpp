#pragma once

#include "util_std.hpp"
#include "util_js.hpp"

class GoogleSheet {
public:
    GoogleSheet() {}
    GoogleSheet(std::string sheet_link) {
        // Read the sheet from the link
        std::string sheet_id = _extract_sheet_id_from_url(sheet_link);
        if(sheet_id.size() > 0) {
            _read_sheet(sheet_id);
        }
        else {
            Print("Warning: The link to the google sheet was in an invalid format!");
        }
    }
    std::string _extract_sheet_id_from_url(const std::string& url) {
        std::string prefix = "/d/";
        
        size_t start = url.find(prefix);
        if (start == std::string::npos) return ""; // Not found
        start += prefix.length(); // Move past "/d/"
        
        return url.substr(start, url.find('/', start) - start);
    }
    class _OnSheetLoaded: public vicmil::JsFunc {
        public:
        bool file_loaded = false;
        std::string raw_csv_string;
        std::vector<std::vector<std::string>> rows_and_cols; 

        std::vector<std::vector<std::string>> _parse_csv(const std::string& csvText) {
            std::vector<std::vector<std::string>> rows_and_cols;
            std::string line;
            std::stringstream ss(csvText);
            
            while (std::getline(ss, line)) {
                std::vector<std::string> row;
                std::string cell;
                std::stringstream lineStream(line);
                
                bool inside_quotes = false;
                
                for (size_t i = 0; i < line.size(); i++) {
                    char c = line[i];

                    if (c == '"') {
                        inside_quotes = !inside_quotes;  // Toggle the inside_quotes flag
                    } else if (c == ',' && !inside_quotes) {
                        row.push_back(cell);  // Save the current cell and clear it for the next one
                        cell.clear();
                    } else {
                        cell.push_back(c);  // Add the character to the current cell
                    }
                }

                // Add the last cell in the row
                if (!cell.empty()) {
                    row.push_back(cell);
                }

                rows_and_cols.push_back(row);
            }
            return rows_and_cols;
        }
        
        void on_data(emscripten::val data) override {
            Print("_OnSheetLoaded::on_data");
            vicmil::JSData js_data;
            js_data._payload = data;
            file_loaded = true;
            raw_csv_string = js_data.read_str("csvText");

            // Extract raw_csv_string into rows and columns
            rows_and_cols = _parse_csv(raw_csv_string);

            rows_and_cols.push_back({});
            rows_and_cols[0].push_back("first column and row element");
        }
    };
    _OnSheetLoaded _on_sheet_loaded; // The loaded file will be stored in this file object

    bool sheet_loaded() {
        return _on_sheet_loaded.file_loaded;
    }

    const std::vector<std::vector<std::string>>& rows_and_columns() {
        return _on_sheet_loaded.rows_and_cols; // Return a reference to where you can read the rows and columns
    }

    const std::string& raw_csv_string() {
        // The data is passed as a csv string, use this to view it
        return _on_sheet_loaded.raw_csv_string;
    }

    void _read_sheet(std::string sheet_id) {
        // Use Emscripten to invoke JavaScript to read file

        // Expose c++ function to javascript to handle callback
        _on_sheet_loaded = _OnSheetLoaded();
        vicmil::JsFuncManager::add_js_func(&_on_sheet_loaded);

        // Call javascript to download the sheet contents
        EM_ASM({
            const sheet_id = UTF8ToString($0);
            const url = "https://docs.google.com/spreadsheets/d/" + sheet_id + "/gviz/tq?tqx=out:csv";

            fetch(url)
                .then(response => response.text())
                .then(csvText => {
                    // Invoke a c++ function to handle the data
                    console.log("reading google sheet finished successfully");
                    var data = {"csvText": csvText};
                    Module.JsFuncManager(data, UTF8ToString($1));
                })
                .catch(error => console.error("Error loading data:", error));
        }, sheet_id.c_str(), _on_sheet_loaded.key.c_str());
    }
};