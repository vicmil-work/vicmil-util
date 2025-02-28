import sys
from pathlib import Path
sys.path.append(str(Path(__file__).resolve().parents[3])) 

from util_cpp import *

cpp_files = [path_traverse_up(__file__, 0) + "/main.cpp"]

build_setup = BuildSetup(
    cpp_file_paths=cpp_files, 
    output_dir=path_traverse_up(__file__, 0) + "/bin",
    browser=True
)

build_setup.build_and_run()


