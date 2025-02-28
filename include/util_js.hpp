#pragma once
/*
When using emscripten and compiling to webassembly, it is possible to call javascript functions from c++
Here is some wrapper functions to do what you want with javascript, from c++!
*/

//#ifdef Print
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <iostream>
#include <string>

namespace vicmil {
// Creates one central javascript function to invoke, where you can pass in as argument which other function you then want to call
// Reduces the complexity of creating and managing javascript functions, and having complex states be passed along with it, now solved using an interface class.
class JsFunc {
    public:
    std::string key;
    virtual void on_data(emscripten::val) = 0;
};

std::map<std::string, JsFunc*> js_funcs;
class JsFuncManager {
public:
    static void on_data(const emscripten::val data, const std::string& func_name) {
        Print("JsFuncManager::on_data");
        if(js_funcs.count(func_name) != 0) {
            js_funcs[func_name]->on_data(data);
        }
    }
    static std::string add_js_func(JsFunc* js_func){
        static bool inited = false;
        if(!inited) {
            inited = true;
            js_funcs = {};
            // Add javascript function instance
            emscripten::function("JsFuncManager", on_data);
        }

        static int js_func_counter = 1;

        std::string key = "JsFunc_" + std::to_string(js_func_counter);
        js_func_counter += 1;
        // Assign the function
        js_funcs[key] = js_func;
        js_func->key = key;
        return key;
    }
    static void remove_js_func(std::string func_name) {
        js_funcs.erase(func_name);
    }
};

class JSData {
    public:
        emscripten::val _payload;
        JSData() {
            _payload = emscripten::val::object();
        }
        std::string read_str(std::string key) {
            std::string str = _payload[key].as<std::string>();
            return str;
        }
        void write_str(std::string key, std::string str) {
            _payload.set(key, emscripten::val(str));
        }
        std::vector<unsigned char> read_bytes(std::string key) {
            std::string base64_bytes = _payload[key].as<std::string>();
            std::vector<unsigned char> raw_bytes_vec = base64_decode(base64_bytes);
            return raw_bytes_vec;
        }
        std::vector<unsigned char> read_Uint8Array(std::string key) {
            // Extract Uint8Array from the JavaScript object
            emscripten::val file_content = _payload[key];

            // Get length of the Uint8Array
            size_t length = file_content["length"].as<size_t>();

            // Create a std::vector<unsigned char> and resize it to fit the data
            std::vector<unsigned char> raw_file_content(length);

            // Copy data from Uint8Array to std::vector
            emscripten::val memoryView = emscripten::val::global("Uint8Array").new_(emscripten::val::module_property("HEAPU8")["buffer"],
                                                                                    file_content["byteOffset"],
                                                                                    length);
            for (size_t i = 0; i < length; i++) {
                raw_file_content[i] = memoryView[i].as<unsigned char>();
            }

            return raw_file_content;
        }
        void write_bytes(std::string key, std::vector<unsigned char> bytes_data) {
            std::string base64_data = to_base64(bytes_data);
            _payload.set(key, emscripten::val(base64_data));
        }
        void write_array(std::string key, std::vector<int> array_vals) {
            emscripten::val em_array = emscripten::val::array();

            for(int i = 0; i < array_vals.size(); i++) {
                em_array.set(i, emscripten::val(array_vals[i]));
            }

            _payload.set(key, em_array);
        }
    };
/*
Make the opengl window fullscreen when compiling with emscripten
*/
void setup_fullscreen_canvas() {
    EM_ASM({
        const canvas = document.getElementById('canvas');
        const terminal = document.getElementById('output');

        // Make the canvas fullscreen
        canvas.style.position = 'absolute';
        canvas.style.top = '0';
        canvas.style.left = '0';
        canvas.style.width = '100%';
        canvas.style.height = '100%';
        canvas.style.zIndex = '10';

        // Hide the terminal
        if (terminal) {
            terminal.style.display = 'none';
        }
    });
}

std::string get_mime_type(const std::string& filename, bool base64 = false) {
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".txt", "text/plain"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"}
    };
    
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return std::string("data:application/octet-stream") + (base64 ? ";base64" : "");
    }
    
    std::string extension = filename.substr(dot_pos);
    auto it = mime_types.find(extension);
    std::string mime_type = (it != mime_types.end()) ? it->second : "application/octet-stream";
    
    return "data:" + mime_type + (base64 ? ";base64" : "");
}

// Function to download a zip file (raw zip bytes)
void download_file(std::string file_name, std::vector<unsigned char> raw_data) {
    // Convert raw zip data to a base64 string
    std::string mimetype = get_mime_type(file_name, true);
    Print("Downloading file " << file_name << ":" << mimetype);
    std::string base64_data = mimetype + "," + to_base64(raw_data);

    // Use Emscripten to invoke JavaScript for file download
    EM_ASM({
        var link = document.createElement('a');
        link.href = UTF8ToString($0);
        link.download = UTF8ToString($1);
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }, base64_data.c_str(), file_name.c_str());
}

class FileInputRequest {
public:
    class _OnFileLoaded: public JsFunc {
        public:
        bool file_loaded = false;
        std::string filename = "";
        std::vector<unsigned char> raw_file_content;
        void on_data(emscripten::val data) override {
            Print("_OnFileLoaded::on_data");
            JSData js_data;
            js_data._payload = data;
            raw_file_content = js_data.read_bytes("file_content");
            filename = "default"; // Set filename to something default; TODO, get the actual filename
            Print("First few bytes: " << (int)raw_file_content[0] << " " 
                          << (int)raw_file_content[1] << " "
                          << (int)raw_file_content[2] << " "
                          << (int)raw_file_content[3]);
            file_loaded = true;
        }
    };
    class _OnFileError: public JsFunc {
        public:
        bool error_detected;
        void on_data(emscripten::val) override {
            Print("_OnFileError::on_data");
        }
    };
    _OnFileLoaded _on_file_loaded;
    _OnFileError _on_file_error;

    FileInputRequest() {}
    FileInputRequest(std::vector<std::string> allowed_filetypes) {
        _on_file_loaded = _OnFileLoaded();
        _on_file_error = _OnFileError();
        JsFuncManager::add_js_func(&_on_file_loaded);
        JsFuncManager::add_js_func(&_on_file_error);
        _request_file_input(allowed_filetypes);
    }

    bool file_loaded() {
        return _on_file_loaded.file_loaded;
    }

    bool file_load_failed() {
        return false; // TODO: Add better error handling
    }

    std::string get_filename() {
        return _on_file_loaded.filename;
    }

    std::vector<unsigned char> get_file_content() {
        if(!file_loaded()) {
            return {};
        }
        return _on_file_loaded.raw_file_content;
    }

    void _request_file_input(std::vector<std::string> allowed_filetypes) {
        std::string filetypes_str = "";
        for (const auto& type : allowed_filetypes) {
            filetypes_str += type + ",";
        }
        if (!filetypes_str.empty()) {
            filetypes_str.pop_back();
        }

        // Use EM_ASM to interact with the JavaScript File API
        EM_ASM({
            var input = document.createElement('input');
            input.type = 'file';
            input.accept = UTF8ToString($0);
            input.onchange = function(event) {
                var file = event.target.files[0];
                if (file) {
                    var reader = new FileReader();
                    reader.onload = function() {
                        console.log("recieved lots of file data...");
                        var base64String = reader.result.split(',')[1]; // Extract base64 data
                        var data = {"file_content": base64String};
                        Module.JsFuncManager(data, UTF8ToString($1));
                    };
                    reader.readAsDataURL(file); // Read file as a Base64 string
                }
            };
            input.click();
        }, filetypes_str.c_str(), _on_file_loaded.key.c_str(), this);
    }
};

// Expose the on_message function to JavaScript
//EMSCRIPTEN_BINDINGS(my_module) {
    //emscripten::function("on_file_loaded", &on_file_loaded);
    //emscripten::function("on_response", &on_response);
    //emscripten::function("on_image_modified", &on_image_modified);
//}

}

#else
namespace vicmil {
    // If not using emscripten, do nothing
    void setup_fullscreen_canvas() {}
    void download_file(std::string file_name, const std::vector<unsigned char>& raw_data) {
        std::string base_name = file_name;
        std::string extension;
        size_t dot_pos = file_name.find_last_of('.');
        if (dot_pos != std::string::npos) {
            base_name = file_name.substr(0, dot_pos);
            extension = file_name.substr(dot_pos);
        }
        
        int count = 1;
        while (std::filesystem::exists(file_name)) {
            std::ostringstream oss;
            oss << base_name << "(" << count << ")" << extension;
            file_name = oss.str();
            count++;
        }
        
        std::ofstream file(file_name, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not open file " << file_name << " for writing." << std::endl;
            return;
        }
        
        file.write(reinterpret_cast<const char*>(raw_data.data()), raw_data.size());
        
        if (!file) {
            std::cerr << "Error: Failed to write data to file " << file_name << std::endl;
        }
        
        file.close();
        std::cout << "File " << file_name << " downloaded successfully." << std::endl;
    }

class FileInputRequest {
public:
    FileInputRequest() {}
    
    FileInputRequest(std::vector<std::string> allowed_filetypes) {}

    bool file_loaded() {
        return false;
    }

    bool file_load_failed() {
        return false;
    }

    std::string get_filename() {
        return "";
    }

    std::vector<unsigned char> get_file_content() {
        return {}; // TODO: Add better implementation
    }
};
}
#endif