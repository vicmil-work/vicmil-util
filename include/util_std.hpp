/*
Contains documentation for how to do some things using the standard library

Everything should be supported in C++11 and newer versions across most major platforms(Windows, Linux, Browser using emscripten)
*/


// ============================================================
//                           Include
// ============================================================

/**
 * These are a list of **most** cross-platform c++11 headers
*/
#pragma once
#include <iostream>     // For cout, cin and in other ways interracting with the terminal
#include <fstream>
#include <sstream>
#include <filesystem>   // For reading and writing files

#include <string>       // Contains std::string
#include <cstring>      // Contains std::memcpy
#include <set>          // Contains std::set
#include <map>          // contains std::map
#include <unordered_map>
#include <vector>       // Contains std::vector
#include <list>         // Contains std::list

#include <math.h>       // Includes basic math operations such as sinus, cosinus etc.
#include <cassert>      // For assering values during runtime
#include <regex>        // Includes functions for regex
#include <chrono>       // Includes functions related to time 
#include <thread>       // Includes functions for handling multi-thread applications
#include <mutex>        // Support library for using threads, includes locks
#include <future>       // Support library for using threads, used for asynchronous retrieval of values
#include <complex.h>    // For supporting complex number operations

#include <typeindex>    // For getting the index of different types, and other type information
#include <random>       // For generating random numbers

#ifdef __EMSCRIPTEN__
#include "emscripten.h" // Include emscripten if available, needed to set the main loop function in case of emscripten
#endif

#ifdef __unix__                             /* __unix__ is usually defined by compilers targeting Unix systems */
    #define OS_Linux
#elif defined(_WIN32) || defined(WIN32)     /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
    #define OS_Windows
#endif

// ============================================================
//                      Debug
// ============================================================

namespace vicmil {
/**
 * Assert that some expression is true, and throw an error if not
*/
#define Assert(x) if((x) == false) {Print("Assert failed! \n" << #x); throw 0;}

/**
 * Print an error message and then throw an error
*/
#define ThrowError(x) Print(x); throw 0;


/**
 * Assert that two numerical values are equal to each other
 * @arg v1: The first numerical value
 * @arg v2: The second numerical value
 * @arg deviance: the deviance allowed between v1 and v2
*/
#define AssertEq(v1, v2, deviance) Assert(abs(v1 - v2) < deviance)


/**
 * Pad the string with spaces if it is too short
 * @arg str: The string to operate on
 * @arg length: How long the string should be after padding
*/
std::string pad_str(std::string str, int length) {
    if(str.size() < length) {
        str.resize(length, ' ');
    }
    return str;
}


inline std::vector<std::string> split_string(std::string str, char separator) {
    if(str.length() == 0) {
        return std::vector<std::string>();
    }
    
    std::vector<std::string> strings;

    int startIndex = 0, endIndex = 0;
    for (int i = 0; i <= str.size(); i++) {
        // If we reached the end of the word or the end of the input.
        if (str[i] == separator || i == str.size()) {
            endIndex = i;
            std::string temp;
            temp.append(str, startIndex, endIndex - startIndex);
            strings.push_back(temp);
            startIndex = endIndex + 1;
        }
    }

    return strings;
}

/**
 * Print the values with some extra nice decorator like 
 * - line number
 * - file name
 * - etc.
*/
#define Print(x) { \
    const std::string line_info = vicmil::pad_str(vicmil::split_string(__FILE__, '/').back(), 20) + ":" + vicmil::pad_str(std::to_string(__LINE__), 4) + ":" + vicmil::pad_str(__func__, 20); \
    std::cout << line_info << x << "\n"; \
}

/**
 * Print the values with some extra nice decorator like 
 * - line number
 * - file name
 * - the content that is printed
 * - etc.
*/
#define PrintExpr(x) { \
    const std::string line_info = vicmil::pad_str(vicmil::split_string(__FILE__, '/').back(), 20) + ":" + vicmil::pad_str(std::to_string(__LINE__), 4) + ":" + vicmil::pad_str(__func__, 20); \
    std::cout << line_info << ": " << #x << ":" << x << "\n"; \
}


// ============================================================
//                          Tests
// ============================================================

typedef void (*void_function_type)();

typedef std::map<std::string, struct TestClass*> __test_map_type__;
static __test_map_type__* __test_map__ = nullptr;

struct TestClass {
    std::string _id_long;
    virtual void test() {}
    virtual ~TestClass() {}
    TestClass(std::string id, std::string id_long) {
        if ( !__test_map__ ) {
            __test_map__ = new __test_map_type__();
        }
        (*__test_map__)[id] = this;
        _id_long = id_long;
    }
    static void run_all_tests(std::vector<std::string> test_keywords = {}) {
        if(!__test_map__) {
            std::cout << "No tests detected!" << std::endl;
            return;
        }
        __test_map_type__::iterator it = __test_map__->begin();
        while(it != __test_map__->end()) {
            std::pair<const std::string, TestClass*> val = *it;
            it++;
            std::string test_name = val.first;
            std::cout << "<<<<<<< run test: " << test_name << ">>>>>>>";
                try {
                    val.second->test();
                }
                catch (const std::exception& e) {
                    std::cout << "caught error" << std::endl;
                    std::cout << e.what(); // information from error printed
                    exit(1);
                }
                catch(...) {
                    std::cout << "caught unknown error" << std::endl;
                    exit(1);
                } 
            std::cout << "test passed!" << std::endl;
        }
        std::cout << "All tests passed!" << std::endl;
    }
};

#define TestWrapper(test_name, func) \
namespace test_class { \
    struct test_name : vicmil::TestClass { \
        test_name() : vicmil::TestClass( \
            pad_str(split_string(__FILE__, '/').back(), 20) + ":" + pad_str(std::to_string(__LINE__), 4) + ":" + pad_str(__func__, 20), \
            std::string(__FILE__) + ":" + pad_str(std::to_string(__LINE__), 4) + ":" + pad_str(__func__, 20)) {} \
        func \
    }; \
} \
namespace test_factory { \
    test_class::test_name test_name = test_class::test_name(); \
}

/**
 * Add a test to be executed
*/
#define AddTest(test_name) \
TestWrapper(test_name ## _, \
void test() { \
    test_name(); \
} \
);

int add_(int x, int y) {
    return x + y;
}

// Add a test for the function
void TEST_add_() {
    assert(add_(1, 2) == 3);
}
AddTest(TEST_add_);

// Run all tests
//int main() {
//    vicmil::TestClass::run_all_tests();
//    return 0;
//}

// ============================================================
//                      String operations
// ============================================================

/*
Convert a vector of something into a string
example: "{123.321, 314.0, 42.0}"
*/
template<class T>
std::string vec_to_str(const std::vector<T>& vec) {
    std::string out_str;
    out_str += "{ ";
    for(int i = 0; i < vec.size(); i++) {
        if(i != 0) {
            out_str += ", ";
        }
        out_str += std::to_string(vec[i]);
    }
    out_str += " }";
    return out_str;
}

/*
Convert a vector of strings into a single string
["a", "b"] -> "{'a', 'b'}"
*/
std::string vec_to_str(const std::vector<std::string>& vec) {
    std::string return_str = "{";
    for(int i = 0; i < vec.size(); i++) {
        if(i != 0) {
            return_str += ", ";
        }
        return_str += "'" + vec[i] + "'";
    }
    return_str += " }";
    return return_str;
}

/**
 * Takes an arbitrary type and converts it to binary, eg string of 1:s and 0:es
*/
template<class T>
std::string to_binary_str(T& value) {
    int size_in_bytes = sizeof(T);
    char* bytes = (char*)&value;
    std::string return_str;
    for(int i = 0; i < size_in_bytes; i++) {
        if(i != 0) {
            return_str += " ";
        }
        for(int j = 0; j < 8; j++) {
            if((bytes[i] & (1<<j)) == 0) {
                return_str += "0";
            }
            else {
                return_str += "1";
            }
        }
    }
    return return_str;
}

/**
 * Replaces each instance of a str_from to str_to inside str
 * @param str the main string
 * @param str_from the pattern string we would like to replace
 * @param str_to what we want to replace str_from with
*/
inline std::string string_replace(const std::string& str, const std::string& str_from, const std::string& str_to) {
    std::string remaining_string = str;
    std::string new_string = "";
    while(true) {
        auto next_occurence = remaining_string.find(str_from);
        if(next_occurence == std::string::npos) {
            return new_string + remaining_string;
        }
        new_string = new_string + remaining_string.substr(0, next_occurence) + str_to;
        remaining_string = remaining_string.substr(next_occurence + str_from.size(), std::string::npos);
    }
}

std::vector<std::string> regex_find_all(std::string str, std::string regex_expr) {
    // Wrap regular expression in c++ type
    std::regex r = std::regex(regex_expr);

    // Iterate to find all matches of regex expression
    std::vector<std::string> tokens = std::vector<std::string>();
    for(std::sregex_iterator i = std::sregex_iterator(str.begin(), str.end(), r);
                            i != std::sregex_iterator();
                            ++i )
    {
        std::smatch m = *i;
        //std::cout << m.str() << " at position " << m.position() << '\n';
        tokens.push_back(m.str());
    }
    return tokens;
}

bool regex_match_expr(std::string str, std::string regex_expr) {
    return std::regex_match(str, std::regex(regex_expr));
}

inline std::string cut_off_after_find(std::string str, std::string delimiter) {
    // Find first occurrence
    size_t found_index = str.find(delimiter);

    // Take substring before first occurance
    if(found_index != std::string::npos) {
        return str.substr(0, found_index);
    }
    return str;
}

inline std::string cut_off_after_rfind(std::string str, std::string delimiter) {
    // Find first occurrence
    size_t found_index = str.rfind(delimiter);

    // Take substring before first occurance
    if(found_index != std::string::npos) {
        return str.substr(0, found_index);
    }
    return str;
}

/** UTF8 is compatible with ascii, and can be stored in a string(of chars)
 * Since ascii is only 7 bytes, we can have one bit represent if it is a regular ascii
 *  or if it is a unicode character. Unicode characters can be one, two, three or four bytes
 * 
 * See https://en.wikipedia.org/wiki/UTF-8 for more info
**/ 
bool is_utf8_ascii_char(char char_) {
    return ((char)(1<<7) & char_) == 0;
}

// Function to convert UTF-8 to a vector of integers representing Unicode code points
std::vector<int> utf8ToUnicodeCodePoints(const std::string& utf8String) {
    std::vector<int> codePoints;
    size_t i = 0;
    while (i < utf8String.size()) {
        unsigned char c = utf8String[i++];
        int codePoint;
        if ((c & 0x80) == 0) {
            codePoint = c;
        } else if ((c & 0xE0) == 0xC0) {
            codePoint = ((c & 0x1F) << 6);
            codePoint |= (utf8String[i++] & 0x3F);
        } else if ((c & 0xF0) == 0xE0) {
            codePoint = ((c & 0x0F) << 12);
            codePoint |= ((utf8String[i++] & 0x3F) << 6);
            codePoint |= (utf8String[i++] & 0x3F);
        } else if ((c & 0xF8) == 0xF0) {
            codePoint = ((c & 0x07) << 18);
            codePoint |= ((utf8String[i++] & 0x3F) << 12);
            codePoint |= ((utf8String[i++] & 0x3F) << 6);
            codePoint |= (utf8String[i++] & 0x3F);
        }
        codePoints.push_back(codePoint);
    }
    return codePoints;
}

// Function to convert a vector of Unicode code points back into a UTF-8 string
std::string unicodeToUtf8(const std::vector<int>& codePoints) {
    std::string result;
    for (int codePoint : codePoints) {
        if (codePoint <= 0x7F) {
            result += static_cast<char>(codePoint);  // 1 byte
        } else if (codePoint <= 0x7FF) {
            result += static_cast<char>(0xC0 | (codePoint >> 6));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));  // 2 bytes
        } else if (codePoint <= 0xFFFF) {
            result += static_cast<char>(0xE0 | (codePoint >> 12));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));  // 3 bytes
        } else {
            result += static_cast<char>(0xF0 | (codePoint >> 18));
            result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));  // 4 bytes
        }
    }
    return result;
}

// Function to convert raw data to a base64 string
std::string to_base64(const std::vector<unsigned char>& data) {
    static const char* const base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string ret;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            ret.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        ret.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (ret.size() % 4) {
        ret.push_back('=');
    }
    return ret;
}

// Base64 decode function
std::vector<unsigned char> base64_decode(const std::string& in) {
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = in.size();
    size_t padding = 0;

    if (len % 4 != 0) {
        throw std::invalid_argument("Input base64 string has incorrect length");
    }

    if (in[len - 1] == '=') padding++;
    if (in[len - 2] == '=') padding++;

    size_t decoded_len = (len * 3) / 4 - padding;
    std::vector<unsigned char> decoded_data(decoded_len);
    
    size_t i = 0;
    size_t j = 0;
    while (i < len) {
        uint32_t a = (in[i] == '=' ? 0 & i++ : base64_chars.find(in[i++])) << 18;
        uint32_t b = (in[i] == '=' ? 0 & i++ : base64_chars.find(in[i++])) << 12;
        uint32_t c = (in[i] == '=' ? 0 & i++ : base64_chars.find(in[i++])) << 6;
        uint32_t d = (in[i] == '=' ? 0 & i++ : base64_chars.find(in[i++]));
        uint32_t triple = a | b | c | d;
        decoded_data[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_len) decoded_data[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_len) decoded_data[j++] = triple & 0xFF;
    }

    return decoded_data;
}

// ============================================================
//                      File read/write
// ============================================================

/**
 * Check if file exists
*/
bool file_exists(const std::string& file_name) {
    std::ifstream file(file_name);
    return file.good();
}

/**
 * Read all the contents of a file and return it as a string
*/
std::string read_file_contents(const std::string& filename) {
    std::ifstream file(filename);
    std::string contents;

    if (file.is_open()) {
        // Read the file content into the 'contents' string.
        std::string line;
        while (std::getline(file, line)) {
            contents += line + "\n"; // Add each line to the contents with a newline.
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
    }

    return contents;
}

/**
 * Read all file contents line by line, and return it as a vector where each index represents a line
*/
std::vector<std::string> read_file_contents_line_by_line(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> contents;

    if (file.is_open()) {
        // Read the file content into the 'contents' string.
        std::string line;
        while (std::getline(file, line)) {
            contents.push_back(line); // Add each line to the contents with a newline.
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
    }

    return contents;
}

/**
 * General file manager for easy file management
 * - Contains a lot of utility functions to make your life easier
*/
class FileManager {
    std::string filename;
public:
    std::fstream file; // Read & Write
    //std::ifstream file; // Read only
    //std::ofstream file; // Write only
    FileManager(std::string filename_, bool create_file=false) { // class constructor
        filename = filename_;
        if(create_file) {
            file = std::fstream(filename, std::ios::app | std::ios::binary);
        }
        else {
            file = std::fstream(filename, std::ios::in | std::ios::binary | std::ios::out);
        }
  
        if(!file_is_open()) {
            std::cout << "file could not open!" << std::endl;
            std::cout << filename << std::endl;
            throw;
        }
        else {
            //std::cout << "file opened successfully" << std::endl;
        }
        }
    bool file_is_open() {
        return file.is_open();
    }

    void set_read_write_position(unsigned int index) {
        file.seekg(index);
    }
    unsigned int get_read_write_position() {
        // get current read position
        std::streampos read_pos = file.tellg();
        return read_pos;
    }

    std::vector<char> read_bytes(unsigned int read_size_in_bytes) { // read bytes as binary and write into &output
        std::vector<char> output = std::vector<char>();
        output.resize(read_size_in_bytes);
        file.read(&output[0], read_size_in_bytes); // read this many bytes from read_write_position in file, to &output[0] in memory.
        return output;
    }
    std::vector<unsigned char> read_bytes_uchar(unsigned int read_size_in_bytes) { // read bytes as binary and write into &output
        std::vector<unsigned char> output = std::vector<unsigned char>();
        output.resize(read_size_in_bytes);
        file.read((char*)(&output[0]), read_size_in_bytes); // read this many bytes from read_write_position in file, to &output[0] in memory.
        return output;
    }
    std::vector<char> read_entire_file() {
        unsigned int file_size = get_file_size();
        set_read_write_position(0);
        return read_bytes(file_size);
    }
    std::vector<unsigned char> read_entire_file_uchar() {
        unsigned int file_size = get_file_size();
        set_read_write_position(0);
        return read_bytes_uchar(file_size);
    }
    std::string read_entire_file_str() {
        unsigned int file_size = get_file_size();
        set_read_write_position(0);
        return read_str(file_size);
    }
    void write_bytes(const unsigned char* data, int size_in_bytes) {
        file.write((char*)(void*)data, size_in_bytes);
    }
    void write_bytes(const std::vector<char>& input) {
        file.write(&input[0], input.size());
        // write this many bytes from &input[0] in memory, to read_write_position in file.
        // also increments read_write_position by the same amount of bytes.
    }
    void write_bytes(const std::vector<unsigned char>& input) {
        write_bytes(&input[0], input.size());
    }

    std::string read_str(unsigned int read_size_in_bytes) {
        std::string output = std::string();
        output.resize(read_size_in_bytes);
        file.read(&output[0], read_size_in_bytes);
        return output;
    }
    void write_str(std::string& str) {
        file.write(&str[0], str.length());
    }

    void write_int32(int val) {
        int* val_pointer = &val;
        char* char_pointer = reinterpret_cast<char*>(val_pointer); // convert int* to char* without changing position
        std::vector<char> bytes = {char_pointer[0], char_pointer[1], char_pointer[2], char_pointer[3]};
        write_bytes(bytes);
    }
    int read_int32() {
        std::vector<char> bytes = read_bytes(4);
        char* char_pointer = &bytes[0];
        int* val_pointer = reinterpret_cast<int*>(char_pointer);
        return *val_pointer;
    }

    unsigned int read_uint32() {
        std::vector<char> bytes = read_bytes(4);
        char* char_pointer = &bytes[0];
        unsigned int* val_pointer = reinterpret_cast<unsigned int*>(char_pointer);
        return *val_pointer;
    }

    unsigned char read_uint8() {
        std::vector<char> bytes = read_bytes(1);
        char* char_pointer = &bytes[0];
        unsigned char* val_pointer = reinterpret_cast<unsigned char*>(char_pointer);
        return *val_pointer;
    }

    char read_int8() {
        std::vector<char> bytes = read_bytes(1);
        char* char_pointer = &bytes[0];
        char* val_pointer = reinterpret_cast<char*>(char_pointer);
        return *val_pointer;
    }

    std::string read_word() {
        std::string output = std::string();
        file >> output;
        return output;
    }
    std::string read_next_line() {
        std::string output = std::string();
        std::getline(file, output, '\n');
        return output;
    }
    bool end_of_file() {
        return file.eof();
    }
    void erase_file_contents() {
        file.close();
        file.open(filename, std::ios::trunc | std::ios::out | std::ios::in | std::ios::binary);
        if(!file_is_open()) {
            std::cout << "file could not open!" << std::endl;
            throw;
        }
        else {
            //std::cout << "file opened successfully" << std::endl;
        }
    }
    // NOTE! This will move the read/write position
    unsigned int get_file_size() {
        file.seekg( 0, std::ios::end );
        return get_read_write_position();
    }
};

// ============================================================
//                           Time
// ============================================================

/**
 * Returns the time since epoch in seconds, 
 * NOTE! Different behaviour on different devices
 * on some devices epoch refers to January 1: 1970, 
 * on other devices epoch might refer to time since last boot
*/
double get_time_since_epoch_s() {
    using namespace std::chrono;
    auto time = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch());
    double nano_secs = time.count();
    return nano_secs / (1000.0 * 1000.0 * 1000.0);
}
double get_time_since_epoch_ms() {
    return get_time_since_epoch_s() * 1000;
}

void sleep_s(double sleep_time_s) {
    #ifndef __EMSCRIPTEN__
    double time_ms = sleep_time_s * 1000;
    std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)time_ms));
    #else
    emscripten_sleep(sleep_time_s*1000); // requires setting -s ASYNCIFY=1 in the compiler options. Otherwise it sleeps indefinately
    #endif
}

void TEST_sleep() {
    double start_time = get_time_since_epoch_s();
    sleep_s(0.7);
    double end_time = get_time_since_epoch_s();
    double duration = end_time - start_time;
    AssertEq(duration, 0.7, 0.1);
}


// ============================================================
//                           Math
// ============================================================

const double PI  = 3.141592653589793238463;

inline bool is_power_of_two(unsigned int x) {
    return !(x == 0) && !(x & (x - 1));
}
inline bool is_power_of_two(int x) {
    return !(x == 0) && !(x & (x - 1));
}

unsigned int upper_power_of_two(unsigned int x) {
    int power = 1;
    while(power < x) {
        power*=2;
    }
    return power;
}

double modulo(double val, double mod) {
    if(val > 0) {
        return val - ((int)(val / mod)) * mod;
    }
    else {
        return val - ((int)((val-0.0000001) / mod) - 1) * mod;
    }
}

double degrees_to_radians(const double deg) {
    const double PI  = 3.141592653589793238463;
    return deg * 2.0 * PI / 360.0;
}

double radians_to_degrees(const double rad) {
    const double PI  = 3.141592653589793238463;
    return rad * 360.0 / (PI * 2.0);
}

/**
 * Determine if a value is in range
 * Returns true if min_v <= v <= max_v
*/
template<class T>
inline bool in_range(T v, T min_v, T max_v) {
    if(v < min_v) {
        return false;
    }
    if(v > max_v) {
        return false;
    }
    return true;
}

// ============================================================
//                     Vector operations
// ============================================================

/**
 * Determine if a value exists somewhere in a vector
 * @param val The value to look for
 * @param vec The vector to look in
 * @return Returns true if value is somewhere in vector, otherwise returns false
*/
template<class T>
bool in_vector(T val, std::vector<T>& vec) {
    for(int i = 0; i < vec.size(); i++) {
        if(val == vec[i]) {
            return true;
        }
    }
    return false;
}

double get_min_in_vector(std::vector<double> vec) {
    double min_val = vec[0];
    for(int i = 0; i < vec.size(); i++) {
        if(vec[i] < min_val) {
            min_val = vec[i];
        }
    }
    return min_val;
}

double get_max_in_vector(std::vector<double> vec) {
    double max_val = vec[0];
    for(int i = 0; i < vec.size(); i++) {
        if(vec[i] > max_val) {
            max_val = vec[i];
        }
    }
    return max_val;
}

template <class T>
T vec_sum(const std::vector<T>& vec, T zero_element) {
    T sum = zero_element;
    for(int i = 0; i < vec.size(); i++) {
        sum += vec[i];
    }
    return sum;
}

template <class T>
T vec_sum(const std::vector<T>& vec) {
    return vec_sum(vec, (T)0);
}

template <class T>
void vec_sort_ascend(std::vector<T>& vec) {
    std::sort(vec.begin(), 
        vec.end(), 
        [](const T& lhs, const T& rhs) {
            return lhs < rhs;
    });
}

template <class T>
void vec_sort_descend(std::vector<T>& vec) {
    std::sort(vec.begin(), 
        vec.end(), 
        [](const T& lhs, const T& rhs) {
            return lhs > rhs;
    });
}

template <class T>
std::vector<std::pair<T, int>> vec_to_pair_with_indecies(const std::vector<T>& vec) {
    std::vector<std::pair<T, int>> return_vec = {};
    for(int i = 0; i < vec.size(); i++) {
        std::pair<T, int> pair_;
        pair_.first = vec[i];
        pair_.second = i;
        return_vec.push_back(pair_);
    }
    return return_vec;
}
template <class T>
std::vector<std::pair<T, int>> vec_sort_ascend_and_get_indecies(const std::vector<T>& vec) {
    std::vector<std::pair<T, int>> return_vec = vec_to_pair_with_indecies(vec);
    std::sort(return_vec.begin(), 
        return_vec.end(), 
        [](const std::pair<T, int>& lhs, const std::pair<T, int>& rhs) {
            return lhs.first < rhs.first;
    });
    return return_vec;
}
template <class T>
std::vector<std::pair<T, int>> vec_sort_descend_and_get_indecies(const std::vector<T>& vec) {
    std::vector<std::pair<T, int>> return_vec = vec_to_pair_with_indecies(vec);
    std::sort(return_vec.begin(), 
        return_vec.end(), 
        [](const std::pair<T, int>& lhs, const std::pair<T, int>& rhs) {
            return lhs.first > rhs.first;
    });
    return return_vec;
}

template <class T>
void vec_remove(std::vector<T>& vec, std::size_t pos)
{
    auto it = vec.begin();
    std::advance(it, pos);
    vec.erase(it);
}

/**
Extend one vector with another(can also be referred to as vector adding or concatenation)
extend_vec({1, 2}, {3, 4, 5}) -> {1, 2, 3, 4, 5}
@arg vec: the first vector
@arg add_vec: the vector to add to vec
*/
template <class T>
void vec_extend(std::vector<T>& vec, const std::vector<T>& vec_add){
    vec.insert(vec.end(), vec_add.begin(), vec_add.end());
}

/**
Extend one vector with another(can also be referred to as vector adding or concatenation)
extend_vec({1, 2}, {3, 4, 5}) -> {1, 2, 3, 4, 5}
@arg vec: the first vector
@arg add_vec: the vector to add to vec
*/
template <class T>
void vec_insert(std::vector<T>& vec,int index, T val){
    vec.insert(vec.begin()+index, val);
}

// ============================================================
//                    Emscripten support
// ============================================================

// Some functions to make working with emscripten easier, should also work without emscripten to make it cross platform
static vicmil::void_function_type update_func_ptr = nullptr;
static vicmil::void_function_type init_func_ptr = nullptr;
void main_app_update() {
    static bool inited = false;
    if(!inited) {
        inited = true;
        if(init_func_ptr != nullptr) {
            init_func_ptr();
        }
    }
    if(update_func_ptr != nullptr) {
        update_func_ptr();
    }
}
void set_app_update(vicmil::void_function_type func_ptr_) {
    update_func_ptr = func_ptr_;
}
void set_app_init(vicmil::void_function_type func_ptr_) {
    init_func_ptr = func_ptr_;
}
void app_start() {
    #ifdef EMSCRIPTEN
        emscripten_set_main_loop(main_app_update, 0, 1);
    #else
        while(true) {
            main_app_update();
        }
    #endif
}

// ============================================================
//                           Typing
// ============================================================
template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::shared_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

template<class T>
std::type_index _get_type_index() {
    return std::type_index(typeid(T));
}
template<class T>
std::string type_to_str() {
    std::type_index typ = _get_type_index<T>();
    std::string name = typ.name();
    return name;
}
template<class T>
int64_t type_to_int() {
    std::type_index typ = _get_type_index<T>();
    std::size_t code = typ.hash_code();
    return (int64_t)code;
}
template<class T>
int64_t type_to_int(T* _) {
    return type_to_int<T>();
}
template<class T>
T* null_if_type_missmatch(T* v, int64_t type_int) {
    if(type_to_int<T>() == type_int) {
        return v;
    }
    return nullptr;
}
}