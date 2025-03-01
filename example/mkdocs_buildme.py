import sys
from pathlib import Path
sys.path.append(str(Path(__file__).resolve().parents[2])) # you may need to update the include path if run from somewhere else

from vicmil_mkdocs import *

compile_mkdocs(get_directory(__file__) + "/mkdocs")


