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

    run_command(f"{sys.executable} -m http.server --bind localhost")


launch_html_page(path_traverse_up(__file__, 0) + "/index.html")