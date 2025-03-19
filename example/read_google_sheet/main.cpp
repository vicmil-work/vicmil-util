#include <iostream>
#include <vector>
#include "util_std.hpp"
#include "util_js.hpp"

class GoogleSheet {
public:
    bool sheet_loaded = false; // Initialize with false, since the sheet has not been loaded yet
    std::vector<std::vector<std::string>> sheet_data = {};
    GoogleSheet() {}
    GoogleSheet(std::string sheet_link) {
        // Read the sheet from the link
        std::string sheet_id = _extract_sheet_id_from_link(sheet_link);
        _read_sheet(sheet_id);
    }
    std::string _extract_sheet_id_from_link(std::string sheet_link) {
        return ""; // TODO
    }
    void _read_sheet(std::string sheet_id) {
        // Use Emscripten to invoke JavaScript to read file
        EM_ASM({
            const url = "https://docs.google.com/spreadsheets/d/1snfaCuSoKFVpsMUSrEP76OrerWAATDLbzviLm-cc1c8/gviz/tq?tqx=out:csv";

            fetch(url)
                .then(response => response.text())
                .then(csvText => {
                    // Invoke a c++ function to handle the data... TODO

                    // For now, just print to console
                    console.log(csvText);
                })
                .catch(error => console.error("Error loading data:", error));
        });
    }
};

int main() {
    std::cout << "Starting to read sheet" << std::endl;
    GoogleSheet my_sheet = GoogleSheet("https://docs.google.com/spreadsheets/d/1snfaCuSoKFVpsMUSrEP76OrerWAATDLbzviLm-cc1c8/edit?usp=drive_link");
    while(true) {
        vicmil::sleep_s(0.1);
        if(my_sheet.sheet_loaded) {
            std::cout << my_sheet.sheet_data[0][0]; // Print the first element
            break;
        }
    }
    return 0;
}