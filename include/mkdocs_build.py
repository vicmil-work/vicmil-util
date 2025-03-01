import pathlib
import os
import platform
import importlib
import sys

def get_directory(file_path: str):
    return str(pathlib.Path(file_path).parents[0].resolve()).replace("\\", "/")


def _python_virtual_environment(env_directory_path):
    # Setup a python virtual environmet
    os.makedirs(env_directory_path, exist_ok=True) # Ensure directory exists
    my_os = platform.system()
    if my_os == "Windows":
        os.system(f'python -m venv "{env_directory_path}"')
    else:
        os.system(f'python3 -m venv "{env_directory_path}"')


def _pip_install_packages_in_virtual_environment(env_directory_path, packages):
    if not os.path.exists(env_directory_path):
        print("Invalid path")
        raise Exception("Invalid path")

    my_os = platform.system()
    for package in packages:
        if my_os == "Windows":
            os.system(f'powershell; &"{env_directory_path}/Scripts/pip" install {package}')
        else:
            os.system(f'"{env_directory_path}/bin/pip" install {package}')


def _get_site_packages_path(venv_path):
    """Returns the site-packages path for a given virtual environment."""
    python_version = f"python{sys.version_info.major}.{sys.version_info.minor}"
    
    # Construct the expected site-packages path
    if os.name == "nt":  # Windows
        site_packages_path = os.path.join(venv_path, "Lib", "site-packages")
    else:  # macOS/Linux
        site_packages_path = os.path.join(venv_path, "lib", python_version, "site-packages")

    return site_packages_path if os.path.exists(site_packages_path) else None


def _use_other_venv_if_missing(package_name, other_venv_path, silent=False):
    try:
        importlib.import_module(package_name)
        if not silent:
            print(f"{package_name} is already installed in the current environment.")
    except ImportError:
        print(f"{package_name} not found. Using the package from the other environment.")
        other_venv_path = _get_site_packages_path(other_venv_path)
        print(other_venv_path)
        sys.path.insert(0, other_venv_path)  # Add other venv's site-packages to sys.path
        

def setup():
    virtual_env_path = get_directory(__file__) + "/venv"
    if not os.path.exists(virtual_env_path):
        _python_virtual_environment(virtual_env_path)
        _pip_install_packages_in_virtual_environment(
            env_directory_path=virtual_env_path,
            packages=["mkdocs", "mkdocs-material", "pymdown-extensions"]
        )

    _use_other_venv_if_missing("mkdocs", virtual_env_path, silent=True)
    _use_other_venv_if_missing("mkdocs-material", virtual_env_path, silent=True)
    _use_other_venv_if_missing("pymdown-extensions", virtual_env_path, silent=True)


def mkdocs_new(docs_path: str):
    setup()
    from mkdocs.utils import write_file
    # Create the project directory and docs directory
    os.makedirs(docs_path+"/docs", exist_ok=True)

    # Write default mkdocs.yml file
    config_content = \
"""site_name: My Docs

theme:
  name: material

  features:
    - content.code.copy

  palette:
  # Palette toggle for light mode
    - media: "(prefers-color-scheme: light)"
      scheme: default
      toggle:
        icon: material/toggle-switch-off-outline
        name: Switch to dark mode

    # Palette toggle for dark mode
    - media: "(prefers-color-scheme: dark)"
      scheme: slate
      toggle:
        icon: material/toggle-switch
        name: Switch to light mode

nav:
  - Home: index.md
markdown_extensions:
  - codehilite:
      guess_lang: false  # Ensures correct language highlighting
  - fenced_code
  - pymdownx.superfences
  - pymdownx.tabbed
  - attr_list
"""
    write_file(config_content.encode('utf-8'), docs_path + "/mkdocs.yml")

    # Write default index.md file
    index_content = "# Welcome to MkDocs\n\nThis is your homepage!"
    write_file(index_content.encode('utf-8'), docs_path + "/docs/index.md")

    print(f"New MkDocs project created in: {docs_path}")


def serve_mkdocs_project(docs_path, host="127.0.0.1", port=8000):
    """
    Serve an MkDocs project locally.

    Args:
        config_file (str): Path to the mkdocs.yml configuration file.
        host (str): Host address to bind the server (default: 127.0.0.1).
        port (int): Port number to serve the site (default: 8000).
    """
    setup()
    config_file = docs_path + "/mkdocs.yml"
    from mkdocs.commands.serve import serve
    try:
        # Start the MkDocs development server directly with the configuration file
        print(f"Serving MkDocs at http://{host}:{port}")
        serve(config_file, host=host, port=port, livereload=True, watch_theme=True)
    except KeyboardInterrupt:
        print("\nServer stopped.")


def build_mkdocs_documentation(docs_path):
    setup()
    from mkdocs.config import load_config
    from mkdocs.commands.build import build
    """
    Build MkDocs documentation into a static site.

    Args:
        config_file (str): Path to the mkdocs.yml configuration file.
        output_dir (str): Optional. Path to the output directory for the built site.
    """
    config_file = docs_path + "/mkdocs.yml"
    output_dir = docs_path + "/site"
    # Load the MkDocs configuration
    config = load_config(config_file)
    
    # Set a custom output directory if provided
    if output_dir:
        config['site_dir'] = os.path.abspath(output_dir)
    
    try:
        print(f"Building documentation using config: {config_file}")
        build(config)
        print(f"Documentation successfully built in: {config['site_dir']}")
    except Exception as e:
        print(f"Error while building documentation: {e}")


def go_to_url(url: str):
    import webbrowser
    webbrowser.open(url, new=0, autoraise=True)


def compile_mkdocs(docs_path: str):
    if not os.path.exists(docs_path):
        mkdocs_new(docs_path)
    build_mkdocs_documentation(docs_path)
    # TODO: check if build was successfull
    go_to_url("http://127.0.0.1:8000")
    serve_mkdocs_project(docs_path)



