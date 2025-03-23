import sys
from pathlib import Path
sys.path.append(str(Path(__file__).resolve().parents[3])) 

from util_cpp import *

cpp_files = [path_traverse_up(__file__, 0) + "/main.cpp"]

build_setup = BuildSetup(
    cpp_file_paths=cpp_files, 
    output_dir=path_traverse_up(__file__, 0) + "/bin",
    browser=True,
    include_vicmil_pip_packages=True
)

convert_file_to_header(path_traverse_up(__file__, 3) + "/noto_sans_mono_jp/Noto Sans Mono CJK JP Regular.otf", path_traverse_up(__file__, 0) + "/noto_sans_mono_jp.hpp")

build_setup.build_and_run()


