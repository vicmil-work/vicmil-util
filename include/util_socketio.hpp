#pragma once
/*
This file contains some utilities for handling socketio
The idea is to be cross platform, and also to support the web
*/

#include "util_js.hpp"

#ifndef __EMSCRIPTEN__
#include "../deps/socket.io-client-cpp/src/sio_client.h"
#else
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#endif

namespace vicmil {

#ifndef __EMSCRIPTEN__
//#ifndef Print
class SocketIOClient {
private:
    sio::client client;
public:
    bool _successfull_connection = false;
    bool _failed_connection = false;
    class Data {
    public:
        sio::message::ptr _data;
        Data() {
            _data = sio::object_message::create();
        }
        std::string read_str(std::string key) {
            std::string str = _data->get_map()[key]->get_string();
            return str;
        }
        void write_str(std::string key, std::string str) {
            _data->get_map()[key] = sio::string_message::create(str);
        }
        std::vector<unsigned char> read_bytes(std::string key) {
            std::string base64_bytes = _data->get_map()[key]->get_string();
            std::vector<unsigned char> raw_bytes_vec = base64_decode(base64_bytes);
            return raw_bytes_vec;
        }
        void write_bytes(std::string key, std::vector<unsigned char> bytes_data) {
            std::string base64_data = to_base64(bytes_data);
            _data->get_map()[key] = sio::string_message::create(base64_data);
        }
        void write_array(std::string key, std::vector<int> array_vals) {
            sio::message::ptr xy_array = sio::array_message::create();
            for(int i = 0; i < array_vals.size(); i++) {
                xy_array->get_vector().push_back(sio::int_message::create(array_vals[i]));
            }
            _data->get_map()[key] = xy_array;
        }
        int size_in_bytes() const {
            size_t total_size = 0;

            auto& message_map = _data->get_map();
            for (const auto& pair : message_map) {
                // Add the size of the key (assuming key is a string)
                total_size += pair.first.size();
                
                // Add the size of the value (if it's a string)
                if (pair.second->get_flag() == pair.second->flag_string) {
                    total_size += pair.second->get_string().size();
                }
                // Handle other types (e.g., number, object) if necessary
            }
            return total_size;
        }
    };

    // Interface class for handling when data is recieved
    class OnDataRecieved {
    public:
        virtual void on_data(Data) = 0;
    };

    bool successfull_connection() {
        return _successfull_connection;
    }
    bool failed_connection() {
        return _failed_connection;
    }

    SocketIOClient() {}
    // When recieving an event with the event name, invoke the function
    void add_OnDataRecieved(std::string event_name, OnDataRecieved* on_data_recieved) {
        auto lambda = [this, on_data_recieved](sio::event& event) {  // Capture args by value
            SocketIOClient::Data data;
            data._data = event.get_message();
            on_data_recieved->on_data(data);
        };
        client.socket()->on(event_name, lambda);
    }
    void connect(const std::string &uri) {
        client.connect(uri);
        auto lambda = [this]() {  // Capture args by value
            this->_successfull_connection = true;
        };
        client.set_open_listener(lambda);
    }
    void close() {
        client.sync_close();
        client.clear_con_listeners();
    }
    // Send more complicated data in a map: TODO
    void emit_data(const std::string &event_name, const SocketIOClient::Data& data) {
        // Note! There is a hard limit of sending 1_000_000 bytes, split it up if you need to send more
        Print("data size: " << std::to_string(data.size_in_bytes()));
        client.socket()->emit(event_name, data._data);
    }
};

#else

class SocketIOClient {
private:
bool connection_attempt = false;
std::string socketID = "";
public:
    bool _successfull_connection = false;
    bool _failed_connection = false;
    class Data: public JSData {};

    // Interface class for handling when data is recieved
    class OnDataRecieved {
    public:
        virtual void on_data(Data) = 0;
    };

    class _OnDataRecieved: public JsFunc {
    public:
        OnDataRecieved* to_invoke; // Invoke class whenever data is recived
        void on_data(emscripten::val raw_data) {
            Data data;
            data._payload = raw_data;
            to_invoke->on_data(data);
        }
    };
    class _OnConnection: public JsFunc {
    public:
        SocketIOClient* socket_io;
        void on_data(emscripten::val) {
            socket_io->_successfull_connection = true;
            Print("_OnConnection");

            // Setup all the events for handling incomming data
            // TODO
        }
    };
    bool successfull_connection() {
        return _successfull_connection;
    }
    bool failed_connection() {
        return _failed_connection;
    }
    _OnConnection _on_connection = _OnConnection();
    std::map<std::string, _OnDataRecieved> _js_on_data = {};
    void add_OnDataRecieved(std::string event_name, OnDataRecieved* on_data_recieved) {
        if(!connection_attempt) {
            ThrowError("add_OnDataRecieved invoked before connection attempt");
        }
        _js_on_data[event_name] = _OnDataRecieved();
        _js_on_data[event_name].to_invoke = on_data_recieved;
        vicmil::JsFuncManager::add_js_func(&_js_on_data[event_name]); // Expose class instance to javascript

        // OnConnection invoked when a connection has been made
        EM_ASM({
            // Listen for messages from the server
            window.socket.on(UTF8ToString($0), function(data) {
                console.log('Received data from socket');
                Module.JsFuncManager(data, UTF8ToString($1));
            });
        }, event_name.c_str(), _js_on_data[event_name].key.c_str());
    }
    void emit_data(std::string event_name, Data data) {
        if(!_successfull_connection) {
            ThrowError("send_data invoked without successfull socket connection");
        }
        emscripten::val socket = emscripten::val::global("socket"); // Get the JavaScript 'socket' object
        socket.call<void>("emit", emscripten::val(event_name.c_str()), data._payload);
    }
    void connect(const std::string &uri) {
        if(connection_attempt) {
            ThrowError("socket connection_attempt has already been made!");
        }
        _on_connection.socket_io = this;
        vicmil::JsFuncManager::add_js_func(&_on_connection); // Expose class instance to javascript
        EM_ASM({
            // Load socket.io if not already loaded
            if (typeof io === 'undefined') {
                var script = document.createElement('script');
                script.src = 'https://cdn.socket.io/4.5.4/socket.io.min.js';
                script.onload = function() {
                    console.log("Socket.io loaded.");
                    startSocket();
                };
                document.head.appendChild(script);
            } else {
                startSocket();
            }

            function startSocket() {
                // Connect to the Socket.io server
                window.socket = io(UTF8ToString($0)); // Change to your server

                window.socket.on('connect', function() {
                    console.log('Connected to server');
                    var data = {"connection": "success"};
                    Module.JsFuncManager(data, UTF8ToString($1));
                });
            }
        }, uri.c_str(), _on_connection.key.c_str());
        connection_attempt = true; // Connection attempt made
    }
    void close() { // Close the connection to the server
        // TODO
    }
};

/*class SocketIOClient {
private:
    emscripten::val socket;
    emscripten::val io;
    void wait_for_socketio() { // If socketIO is not loaded yet, wait for it to load...
        if (io.isUndefined() || io.isNull()) {
            // Dynamically load Socket.IO and establish a connection
            EM_ASM({
                // Check if 'io' is already defined
                if (typeof io === 'undefined') {
                    console.log("Loading Socket.IO...");
                    var script = document.createElement('script');
                    script.src = "https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.5.4/socket.io.js";
                    document.head.appendChild(script);
                }
            });

            Print("Waiting for socketio to load...");
            // Wait for socketio to load...
            while(io.isUndefined() || io.isNull()) {
                vicmil::sleep_s(1);
                Print("trying again...");
                io = emscripten::val::global("io");
            }
            Print("Socketio loaded!");
        }
    }

public:
    std::string socket_name = "default"; // Optionally set a name of the socket, for the listener functions

    SocketIOClient() {
        io = emscripten::val::global("io");
    }

    

    typedef void (*socket_func_ptr)(SocketIOClient*);
    typedef void (*socket_msg_func_ptr)(SocketIOClient*, std::string);
    typedef void (*socket_data_func_ptr)(SocketIOClient*, SocketIOClient::Data);

private:
    std::vector<socket_func_ptr> open_listeners = {};
    std::vector<socket_func_ptr> fail_listeners = {};
    std::vector<socket_func_ptr> close_listeners = {};
public:

    void connect(const std::string &uri) {
        wait_for_socketio();
        if (!(socket.isUndefined() || socket.isNull())) {
            ThrowError("socket connection already established!");
            return; // Socket conection already established!
        }

        socket = io.call<emscripten::val>("connect", emscripten::val(uri));
        if (socket.isUndefined() || socket.isNull()) {
            std::cerr << "Error: Failed to connect socket." << std::endl;
            for(int i = 0; i < fail_listeners.size(); i++) {
                fail_listeners[i](this);
            }
            return;
        }
        else {
            Print("socket connection successfull!");
            for(int i = 0; i < open_listeners.size(); i++) {
                open_listeners[i](this);
            }
        }
    }

    void set_close_listener(socket_func_ptr func) {
        close_listeners.push_back(func);
    }

    void set_open_listener(socket_func_ptr func) {
        open_listeners.push_back(func);
    }

    void set_fail_listener(socket_func_ptr func) {
        fail_listeners.push_back(func);
    }

    // When recieving an event with the event name, invoke the function
    void set_on_msg_listener(const std::string &event_name, socket_msg_func_ptr func) {
        // Capture additional data in a lambda

        int a = 10;

        socket.call<void>("on", emscripten::val("response"), emscripten::val([a](emscripten::val event_data) {
            // Now you have access to the socket and event_data
            // You can call the original on_response handler, or modify the behavior
            // js_callback.call<void>("call", event_data, socket);
        }));
    }
    // When recieving an event with the event name, invoke the function
    void set_on_data_listener(const std::string &event_name, socket_data_func_ptr func) {
        // Define as static variables so we can pass it into lambda function(hacky workaround)
        static socket_data_func_ptr static_func;
        static_func = func;
        static SocketIOClient* static_client;
        static_client = this;

        typedef void(*EventListenerFunc)(emscripten::val);

        EventListenerFunc onEventListener = [](emscripten::val data) {
            std::cout << "Event received" << std::endl;
            static socket_data_func_ptr lambda_func;
            static SocketIOClient* lambda_client;
            static bool inited = false;
            if(!inited) {
                Print("Init static variables");
                inited = true;
                lambda_func = static_func;
                lambda_client = static_client;
                return;
            }
            Print("Transmit payload");
            Data data_;
            data_._payload = data;
            lambda_func(lambda_client, data_);
        };

        // Init the lambda statatic parameters
        emscripten::val tmp_data;
        onEventListener(tmp_data); 
        Print("Lambda function should be inited!");

        // Register callback
        emscripten::function("_on_event_listener", onEventListener);

        auto js_callback = emscripten::val::module_property("_on_event_listener");
        socket.call<void>("on", emscripten::val(event_name), js_callback);
    }
    void close() {
        // Closing the socket
        socket.call<void>("disconnect");
        for(int i = 0; i < close_listeners.size(); i++) {
            close_listeners[i](this);
        }
    }
    // Send a message
    void emit_msg(const std::string &event_name, const std::string &message) {
        // Emit a message to the server
        socket.call<void>("emit", emscripten::val(event_name), emscripten::val(message));
    }
    // Send more complicated data in a map
    void emit_data(const std::string &event_name, const SocketIOClient::Data& data) {
        // Send the payload using socket.emit
        if (!socket.isUndefined() && !socket.isNull()) {
            socket.call<void>("emit", emscripten::val(event_name), data._payload);
        } else {
            std::cerr << "Error: Socket is not connected!" << std::endl;
            for(int i = 0; i < fail_listeners.size(); i++) {
                fail_listeners[i](this);
            }
        }
    }
};*/

// Expose the on_message function to JavaScript
//EMSCRIPTEN_BINDINGS(my_module) {
    //emscripten::function("_on_event_listener", &_on_event_listener);
    //emscripten::function("on_response", &on_response);
    //emscripten::function("on_image_modified", &on_image_modified);
//}

#endif

}
