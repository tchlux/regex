# Get the version number from the setup file
import os

DIRECTORY = os.path.dirname(os.path.abspath(__file__))
ABOUT_DIR = os.path.join(DIRECTORY, "about")
with open(os.path.join(ABOUT_DIR,"version.txt")) as f:
    __version__ = f.read().strip()

# Get any public module attributes, also set the "__doc__" from there.
from .regex import RegexError, match, __doc__

# When using "from regex import *", only get these:
__all__ = [RegexError, match]
