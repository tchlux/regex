'''--------------------------------------------------------------------
                            regex.py
 
 A fast regular expression matching code for Python, built on top of
 the 'regex.c' library. The main function provided here is:

   match(regex, string) -> (start, end) or None or RegexError,

 Documentation for 'regex.c' follows.

''' 

# Get the version number from the setup file
import os

DIRECTORY = os.path.dirname(os.path.abspath(__file__))
ABOUT_DIR = os.path.join(DIRECTORY, "about")
with open(os.path.join(ABOUT_DIR,"version.txt")) as f:
    __version__ = f.read().strip()

# Import ctypes for loading the underlying C regex library.
import ctypes

# --------------------------------------------------------------------
clib_bin = os.path.join(os.path.dirname(os.path.abspath(__file__)), "regex.so")
clib_source = os.path.join(os.path.dirname(os.path.abspath(__file__)), "regex.c")
# Import or compile the C file.
try:
    clib = ctypes.CDLL(clib_bin)
except:
    # Get the name of the local C compiler used by Python.
    def get_c_compiler():
        from distutils.ccompiler import new_compiler
        from distutils.sysconfig import customize_compiler
        compiler = new_compiler()
        customize_compiler(compiler)
        return compiler.compiler[0]
    # Configure for the compilation for the C code.
    c_compiler = get_c_compiler()
    compile_command = f"{c_compiler} -O3 -fPIC -shared -o '{clib_bin}' '{clib_source}'"
    # Compile and import.
    os.system(compile_command)
    clib = ctypes.CDLL(clib_bin)
    # Clean up "global" variables.
    del(c_compiler, compile_command)
# --------------------------------------------------------------------

# Exception to raise when errors are reported by the regex library.
class RegexError(Exception): pass

# Find a match for the given regex in string.
# 
#   match(regex, string) -> (start, end) or None or RegexError,
# 
# where "regex" is a string defining a regular expression and "string"
# is a string to be searched against. The function will return None
# if there is no match found. If a match is found, a tuple with
# integers "start" (inclusive index) and "end" (exclusive index) will
# be returned. If there is a problem with the regular expression,
# a RegexError will be raised.
#
# Some substitutions are made before passing the regular expressions
# "regex" to the "match" function in 'regex.c'. These substitutions
# include:
#
#  - If "^" is at the beginning of "regex", it will be removed (because
#    the underlying library implicitly assumes beginning-of-string.
#  - If no "^" is placed at the beginning of "regex", then ".*" will
#    be appened to the beginning of "regex" to behave like other 
#    regular expression libraries.
#  - If "$" is the last character of "regex", it will be substituted
#    with "{.}", the appropriate pattern for end-of-string matches.
# 
def match(regex, string):
    # Do substitutions that make the underlying regex implementation
    # behave more like common existing regex packages.
    if (len(regex) > 0):
        # Add a ".*" to the front of the regex if the beginning of the
        # string was not explicitly desired in the match pattern.
        if (regex[0] == "^"): regex = regex[1:]
        elif ((regex[0] != ".") or (regex[1] != '*')): regex = ".*" + regex
        # Add a "{.}" to the end of the regex if the end of the string
        # was explicitly requested in the pattern.
        if (regex[-1] == "$"): regex = regex[:-1] + "{.}"

    # Call the C utillity.
    #   initialize memory storage for the start and end of a match
    start = ctypes.pointer(ctypes.c_int())
    end = ctypes.pointer(ctypes.c_int())
    #   convert strings into character arrays
    c_regex = ctypes.c_char_p(regex.encode("utf-8"));
    c_string = ctypes.c_char_p(string.encode("utf-8"));
    #   execute the C function
    clib.match(c_regex, c_string, start, end)
    #   extract the integers assigned to 'start' and 'end'
    start = start.contents.value
    end = end.contents.value

    # Return appropriately (handling error flags).
    if (start >= 0): return (start, end)
    elif (start < 0):
        if (end == 0): return None # no match found
        elif (end == -1): return (0, 0) # empty regular expression
        elif ((start == -1) and (end == -5)): return None  # empty string
        elif (end < -1): # error code provided by C
            err = f"Invalid regular expression (code {-end})"
            if (start < -1):
                start -= sum(1 for c in regex if c in "\n\t\r\0")
                err += f", error at position {-start-1}.\n"
                err += f"  {str([regex])[1:-1]}\n"
                err += f"  {(-start-1)*' '}^"
            else: err += "."
            raise(RegexError(err))

# Append the documentation from "regex.c" to the documentation for this module.
with open(os.path.join(os.path.dirname(__file__),"regex.c")) as f:
    source_file = f.read()
    start, end = match("^/.*\n{/}", source_file)
    __doc__ += source_file[start+3:end].replace("\n//", "\n")
    del(source_file, start, end)
del(f)


# Function for running hand-crafted tests.
def test():
    # regex  = "^[*][*]*{[*]}"
    # string = "*** test"

    # # Find any "**" patterns.
    # regex  = "{[*]}[*][*]{[*]}"
    # string = "ab*  **"

    # Find any "**" patterns.
    regex  = "^aa*{a}"   # <- find most 'a' at beginning
    # regex  = "{a}aa**" # <- find most 'a' in middle
    string = "aaa  "

    # regex  = "^((\r\n)|\r|\n)"
    # string = ""
    # string = ''' Testing 
    # long multiplie line string,
    # does it work? '''

    # regex = "\n.*{[}]}abc" # <- this should not match the regex on this file
    # with open(__file__) as f:
    #     string = f.read()
    # abc <- this will match the test regular expression

    # Run the regex on the string.
    # string = string[4280:4280+150]
    result = match(regex, string)
    if (result is not None):
        start, end = result
        print(f"Match at {start} -> {end}: {str([string[start:end]])[1:-1]}")
    else:
        print("No match.")


# When using "from regex import *", only get these variables:
__all__ = [RegexError, match]

# Do a fast regular expression search over files that match a given
# pattern. Find all nonoverlapping matches in the files and print
# all matching patterns, their files, and their locations.
def frex(regex, path_patterns, curdir=".", skip=None):
    # Initialize the list of paths to skip..
    if (skip is None): skip = set()
    # Initialize a list of directories to recurse into.
    directories = []
    # Check all paths in the current directory.
    for path in os.listdir(curdir):
        path = os.path.join(curdir, path)
        # If this is a directory, mark it to be searched later.
        if os.path.isdir(path):
            directories.append(path)
            continue
        # Look to see if this file path matches a path pattern.
        for pattern in path_patterns:
            if (match(pattern, path) is not None): break
        # If no matches were found, skip this path.
        else: continue
        # Otherwise, if this is a file then search it!
        if (os.path.isfile(path) and (path not in skip)):
            skip.add(path) # mark this path to not be checked again
            with open(path) as f:
                lines = 1
                file_string = f.read()
                loc = match(regex, file_string)
                if (loc is None): continue
                print("_"*(len(path)+4))
                print(" ",path)
                print()
                while (loc is not None):
                    # Process the strings to get the match.
                    start, end = loc
                    file_start = file_string[:start-1]
                    match_string = file_string[start:end]
                    file_end = file_string[end:]
                    # Get the first new line before and after the match.
                    nearest_new_line = ("\n"+file_start)[::-1].index("\n")
                    next_new_line = (file_end+"\n").index("\n")
                    match_line_string = file_string[start-nearest_new_line-1:end+next_new_line]
                    # Print the line number, accumulate line count, look for next match.
                    lines += file_start.count("\n")
                    print("Line", lines)
                    print("",match_line_string)
                    print()
                    lines += match_string.count("\n")
                    file_string = file_end
                    loc = match(regex, file_string)
    # Recurse into all directories.
    for path in directories:
        frex(regex, path_patterns, curdir=path, skip=skip)

    pass

# Main for when this is executed as a program.
if __name__ == "__main__":
    import sys
    regex = sys.argv[1]
    print()
    print("Using regex:",str([regex])[1:-1])
    path_patterns = sys.argv[2:]
    curdir = os.path.abspath(os.path.curdir)
    # Do a fast regular expression search.
    frex(regex, path_patterns, curdir)

