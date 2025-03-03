import sys
from pathlib import Path
sys.path.append(str(Path(__file__).resolve().parents[3])) # you may need to update the include path if run from somewhere else

from vicmil_pip.util_mkdocs import *

compile_mkdocs(get_directory(__file__) + "/docs")


