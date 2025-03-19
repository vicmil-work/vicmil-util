#include "../../include/util_google.hpp"

int main() {
    std::cout << "Starting to read sheet" << std::endl;
    GoogleSheet my_sheet = GoogleSheet("https://docs.google.com/spreadsheets/d/1snfaCuSoKFVpsMUSrEP76OrerWAATDLbzviLm-cc1c8/edit?usp=drive_link");
    while(true) {
        vicmil::sleep_s(0.5);
        Print("Waiting for sheet to load...");
        if(my_sheet.sheet_loaded()) {
            Print("Sheet loaded successfully!");
            Print("raw csv: \n" << my_sheet.raw_csv_string());
            Print(my_sheet.rows_and_columns()[0][0]); // Print the first element
            break;
        }
    }
    return 0;
}