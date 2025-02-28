from typing import List
import platform
import pathlib
import os
import sys

def path_traverse_up(path: str, levels_up: int) -> str:
    """Traverse the provided path upwards

    Parameters
    ----------
        path (str): The path to start from, tips: use __file__ to get path of the current file
        levels_up (int): The number of directories to go upwards

    Returns
    -------
        str: The path after the traversal, eg "/some/file/path"
    """

    parents = pathlib.Path(path).parents
    path_raw = str(parents[levels_up].resolve())
    return path_raw.replace("\\", "/")


class BuildSetup:
    def __init__(self, cpp_file_paths: List[str], output_dir: str, browser = False):
        # When building c++ projects, this is in general the order the flags should be
        self.n1_compiler_path = get_default_compiler_path(browser=browser)
        self.n2_cpp_files = '"' + '" "'.join(cpp_file_paths) + '"'
        self.n3_optimization_level = ""
        self.n4_macros = ""
        self.n5_additional_compiler_settings = ""
        self.n6_include_paths = "-I" + path_traverse_up(__file__, 0)
        self.n7_library_paths = ""
        self.n8_library_files = ""
        self.n9_output_file = output_dir + "/" + get_default_output_file(browser=browser)

        # If the target platform is the browser
        self.browser_flag = browser

    def include_sdl_opengl(self): # Opengl is a cross platform graphics library that also works in the browser(with the right setup)
        add_opengl_flags(self, self.browser_flag)

    def include_socketio_client(self): # socketio client is a cross platform networking library for working with socketio websockets
        add_socketio_client_flags(self)

    def generate_build_command(self):
        arguments = [
            self.n1_compiler_path, 
            self.n2_cpp_files,
            self.n3_optimization_level,
            self.n4_macros,
            self.n5_additional_compiler_settings,
            self.n6_include_paths,
            self.n7_library_paths,
            self.n8_library_files,
            "-o " + '"' + self.n9_output_file + '"',
        ]

        # Reomve arguments with length 0
        arguments = filter(lambda arg: len(arg) > 0, arguments)

        return " ".join(arguments)
    
    def build(self):
        build_command = self.generate_build_command()

        # Remove the output file if it exists already
        if os.path.exists(self.n9_output_file):
            os.remove(self.n9_output_file)

        if not os.path.exists(path_traverse_up(self.n9_output_file, 0)):
            # Create the output directory if it does not exist
            os.makedirs(path_traverse_up(self.n9_output_file, 0), exist_ok=True)

        # Run the build command
        print(build_command)
        run_command(build_command)

    def run(self):
        invoke_file(self.n9_output_file)

    def build_and_run(self):
        self.build()
        self.run()

        


def get_default_output_file(browser = False):
    platform_name = platform.system()

    if not browser:
        if platform_name == "Windows": # Windows
            return "run.exe"

        elif platform_name == "Linux": # Linux
            return "run.out"

        else:
            raise NotImplementedError()
        
    else:
        return "run.html"


def run_command(command: str) -> None:
    """Run a command in the terminal"""
    platform_name = platform.system()
    if platform_name == "Windows": # Windows
        print("running command: ", f'powershell; &{command}')
        if command[0] != '"':
            os.system(f'powershell; {command}')
        else:
            os.system(f'powershell; &{command}')
    else:
        os.system(command)


def invoke_file(file_path: str):
    if not os.path.exists(file_path):
        print(file_path + " does not exist")
        return

    file_extension = file_path.split(".")[-1]

    if file_extension == "html":
        # Create a local python server and open the file in the browser
        launch_html_page(file_path)

    elif file_extension == "exe" or file_extension == "out":
        # Navigate to where the file is located and invoke the file
        file_directory = path_traverse_up(file_path, 0)
        os.chdir(file_directory) # Change active directory
        run_command('"' + file_path + '"')


def launch_html_page(html_file_path: str):
    import webbrowser
    """ Start the webbrowser if not already open and launch the html page

    Parameters
    ----------
        html_file_path (str): The path to the html file that should be shown in the browser

    Returns
    -------
        None
    """
    os.chdir(path_traverse_up(html_file_path, levels_up=0))
    if not (os.path.exists(html_file_path)):
        print("html file does not exist!")
        return
    
    file_name: str = html_file_path.replace("\\", "/").rsplit("/", maxsplit=1)[-1]
    webbrowser.open("http://localhost:8000/" + file_name, new=0, autoraise=True)

    try:
        run_command("python3 -m http.server --bind localhost")
    except Exception as e:
        pass

    run_command("python -m http.server --bind localhost")


# Get the defualt compiler path within vicmil lib
def get_default_compiler_path(browser = False):
    platform_name = platform.system()

    if not browser:
        if platform_name == "Windows": # Windows
            return "g++"
        else:
            return "g++"

    else:
        if platform_name == "Windows": # Windows
            return '"' + path_traverse_up(__file__, 2) + "/emsdk/emsdk/upstream/emscripten/em++.bat" + '"'
        else:
            return '"' + path_traverse_up(__file__, 2) + "/emsdk/emsdk/upstream/emscripten/em++" + '"'


def add_opengl_flags(build_setup: BuildSetup):
    if not build_setup.browser_flag:
        platform_name = platform.system()

        if platform_name == "Windows": # Windows
            dependencies_directory = build_setup.deps_dir

            # SDL
            build_setup.n6_include_paths += ' -I"' + dependencies_directory + "/sdl_mingw/SDL2-2.30.7/x86_64-w64-mingw32/include/SDL2" + '"'
            build_setup.n6_include_paths += ' -I"' + dependencies_directory + "/sdl_mingw/SDL2-2.30.7/x86_64-w64-mingw32/include" + '"'
            build_setup.n7_library_paths += ' -L"' + dependencies_directory + "/sdl_mingw/SDL2-2.30.7/x86_64-w64-mingw32/lib" + '"'

            # SDL_image
            build_setup.n6_include_paths += ' -I"' + dependencies_directory + "/sdl_mingw/SDL2_image-2.8.2/x86_64-w64-mingw32/include" + '"'
            build_setup.n7_library_paths += ' -L"' + dependencies_directory + "/sdl_mingw/SDL2_image-2.8.2/x86_64-w64-mingw32/lib" + '"'

            # Glew
            build_setup.n6_include_paths += ' -I"' + dependencies_directory + "/glew-2.2.0/include" + '"'
            build_setup.n7_library_paths += ' -L"' + dependencies_directory + "/glew-2.2.0/lib/Release/x64" + '"'

            build_setup.n8_library_files += ' -l' + "mingw32"
            build_setup.n8_library_files += ' -l' + "glew32"
            build_setup.n8_library_files += ' -l' + "opengl32"
            build_setup.n8_library_files += ' -l' + "SDL2main"
            build_setup.n8_library_files += ' -l' + "SDL2"
            build_setup.n8_library_files += ' -l' + "SDL2_image"

        elif platform_name == "Linux": # Linux
            build_setup.n6_include_paths += ' -I' + "/usr/include"

            build_setup.n8_library_files += ' -l' + "GLEW"
            build_setup.n8_library_files += ' -l' + "SDL2"
            build_setup.n8_library_files += ' -l' + "SDL2_image"
            build_setup.n8_library_files += ' -l' + "GL"  #(Used for OpenGL on desktops)

        else:
            raise NotImplementedError()

    else:
        build_setup.n5_additional_compiler_settings += " -s USE_SDL=2"
        build_setup.n5_additional_compiler_settings += " -s USE_SDL_IMAGE=2"
        build_setup.n5_additional_compiler_settings += " -s EXTRA_EXPORTED_RUNTIME_METHODS=ccall,cwrap"
        # build_setup.n5_additional_compiler_settings += """ -s SDL2_IMAGE_FORMATS='["png"]'"""
        build_setup.n5_additional_compiler_settings += " -s FULL_ES3=1"


# Asio is used for sockets and network programming
def add_asio_flags(build_setup: BuildSetup):
    if build_setup.browser_flag:
        raise Exception("Asio is not supported for the browser, consider using websockets bindings to javascript(TODO)")

    build_setup.n6_include_paths += ' -I"' + build_setup.deps_dir + "/asio/include" + '"'

    platform_name = platform.system()
    if platform_name == "Windows": # Windows
        build_setup.n8_library_files += " -lws2_32" # Needed to make it compile with mingw compiler

    else:
        print("asio include not implemented for ", platform_name)


def add_socketio_client_flags(build_setup: BuildSetup):
    if build_setup.browser_flag:
        raise Exception("Socketio Client not supported in the browser, consider using websockets bindings to javascript")
    
    build_setup.n2_cpp_files += " "+ build_setup.deps_dir + "/socket.io-client-cpp/src/sio_client.cpp"
    build_setup.n2_cpp_files += " "+ build_setup.deps_dir + "/socket.io-client-cpp/src/sio_socket.cpp"
    build_setup.n2_cpp_files += " "+ build_setup.deps_dir + "/socket.io-client-cpp/src/internal/sio_client_impl.cpp"
    build_setup.n2_cpp_files += " "+ build_setup.deps_dir + "/socket.io-client-cpp/src/internal/sio_packet.cpp"
    
    build_setup.n6_include_paths += " -I" + build_setup.deps_dir + "/socket.io-client-cpp/lib"
    build_setup.n6_include_paths += " -I" + build_setup.deps_dir + "/socket.io-client-cpp/lib/websocketpp"
    build_setup.n6_include_paths += " -I" + build_setup.deps_dir + "/socket.io-client-cpp/lib/asio/asio/include"
    build_setup.n6_include_paths += " -I" + build_setup.deps_dir + "/socket.io-client-cpp/lib/rapidjson/include"

    # These will force ASIO to compile without Boost
    build_setup.n4_macros += " -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -DASIO_STANDALONE"
        
    # These will force sioclient to compile with C++11
    build_setup.n4_macros += " -D_WEBSOCKETPP_CPP11_STL_ -D_WEBSOCKETPP_CPP11_FUNCTIONAL_ -D_WEBSOCKETPP_CPP11_TYPE_TRAITS_ -D_WEBSOCKETPP_CPP11_CHRONO_"

    # Disable sockeio logging
    build_setup.n4_macros += " -DSIO_DISABLE_LOGGING"


def add_miniz_flags(build_setup: BuildSetup):
    build_setup.n2_cpp_files += " "+ build_setup.deps_dir + "/miniz/miniz.c"


def convert_file_to_header(input_file: str, output_header: str=None):
    if not output_header:
        output_header = input_file + ".hpp"
    # Check if input file exists
    if not os.path.exists(input_file):
        print(f"Error: The file '{input_file}' does not exist.")
        sys.exit(1)

    # Open the input file for reading
    with open(input_file, 'rb') as f:
        file_data = f.read()

    var_name = output_header.split("/")[-1].upper().replace('.', '_').replace('/', '_').replace('\\', '_').replace('-', '_')
    # Start creating the header file
    with open(output_header, 'w') as header_file:
        # Write the C++ array header
        header_file.write(f"#ifndef {var_name}_H\n")
        header_file.write(f"#define {var_name}_H\n\n")
        header_file.write(f"unsigned char {var_name}_data[] = {{\n")

        # Write the contents of the file as a C++ array
        for i in range(0, len(file_data), 12):  # 12 bytes per line (adjust as needed)
            # Format each line with the appropriate number of bytes
            header_file.write("    " + ", ".join(f"0x{byte:02X}" for byte in file_data[i:i+12]))
            header_file.write(",\n")

        # End the array
        header_file.write("};\n\n")
        header_file.write(f"unsigned int {var_name}_size = {len(file_data)};\n")
        header_file.write(f"#endif // {var_name}_H\n")

    print(f"Header file '{output_header}' has been created successfully.")


import subprocess
def invoke_python_file_using_subprocess(python_env_path: str, file_path: str, logfile_path: str = None) -> subprocess.Popen:
    if not os.path.exists(python_env_path):
        print(f"invalid path: {python_env_path}")

    if not os.path.exists(file_path):
        print(f"invalid path: {file_path}")

    current_directory = str(pathlib.Path(file_path).parents[0].resolve()).replace("\\", "/")
    os.chdir(current_directory) # Set active directory to the current directory

    command = ""
    my_os = platform.system()
    if logfile_path:
        if my_os == "Windows":
            command = f'powershell; &"{python_env_path}/Scripts/python" -u "{file_path}" > "{logfile_path}"'
        else:
            command = f'"{python_env_path}/bin/python" -u "{file_path}" > "{logfile_path}"'
    else:
        if my_os == "Windows":
            command = f'powershell; &"{python_env_path}/Scripts/python" -u "{file_path}"'
        else:
            command = f'"{python_env_path}/bin/python" -u "{file_path}"'

    new_process = subprocess.Popen(command, shell=True)
    return new_process