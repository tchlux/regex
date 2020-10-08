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
    # Configure for the compilation for the C code.
    c_compiler = "cc"
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
    global ctypes, clib
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

# Given a path to a file, search for all nonoverlapping matches in the
# file and return the count of number of matches and a summary string.
def match_file(path, regex):
    import ctypes
    # Initialize the current line, the number of matches found, and 
    # a summary string of all matches.
    lines = 1
    matches = 0
    summary = ""
    # Make sure the file exists.
    if (not os.path.exists(path)): return path, matches, summary
    # Open the file.
    with open(path) as f:
        # Try reading the file, if it fails then skip it.
        try: file_string = f.read()
        except: return path, matches, summary
        # Get the location of any matches in the file.
        loc = match(regex, file_string)
        if (loc is None): return path, matches, summary
        summary += "______________________________________________________________________\n"
        summary += " "*int(70/2 - len(path)/2) + path
        while (loc is not None):
            matches += 1
            # Process the strings to get the match.
            start, end = loc
            file_start = file_string[:start]
            match_string = file_string[start:end]
            file_end = file_string[end:]
            # Get the first new line before and after the match.
            nearest_new_line = ("\n"+file_start)[::-1].index("\n")
            next_new_line = (file_end+"\n").index("\n")
            match_line_string = file_string[start-nearest_new_line-1:end+next_new_line]
            # Print the line number, accumulate line count, look for next match.
            lines += file_start.count("\n")
            match_line_string = str([match_line_string.strip()])[2:-2]
            if (len(match_line_string) > 0):
                summary += f"\n{lines}: " + match_line_string
            lines += match_string.count("\n")
            file_string = file_end
            loc = match(regex, file_string)
    # Return the number of matches and the  summary string.
    return path, matches, summary

# Do a fast regular expression search over files that match a given
# pattern. Find all nonoverlapping matches in the files and print
# all matching patterns, their files, and their locations.
def frex(regex, *path_patterns, curdir=".", matches=None, recursion_level=0):
    # Initialize the dictionary of matches found
    if (matches is None): matches = {}
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
        # Initialize this path (to be searched later).
        if (os.path.isfile(path) and (path not in matches)):
            matches[path] = 0
    # Recurse into all directories.
    for path in directories:
        frex(regex, *path_patterns, curdir=path, matches=matches,
             recursion_level=recursion_level+1)
    # If this is the top-level caller, do the file searching.
    if (recursion_level == 0):
        # Use parallelism if there are lots of tiles to search.
        if (len(matches) > 100):
            from regex import match_file as pmatch_file
            from regex.parallel import map as pmap
            match_iterator = pmap(pmatch_file, matches, args=(regex,))
        else:
            match_iterator = map(lambda p: match_file(p,regex), matches)
        # Cycle over all matches and print the summaries.
        for p,n,s in match_iterator:
            matches[p] = n
            if (n > 0): print(s)
        # Return the dictionary containing all paths and matches.
        return matches

# Append the documentation from "regex.c" to the documentation for this module.
with open(os.path.join(os.path.dirname(__file__), "regex.c")) as f:
    source_file = f.read()
    start, end = match("^/.*\n{/}", source_file)
    __doc__ += source_file[start+3:end].replace("\n//", "\n")
    del(source_file, start, end)
del(f)

# When using "from regex import *", only get these variables:
__all__ = [RegexError, match, frex]

# Main for when this is executed as a program.
if __name__ == "__main__":
    import sys
    if (len(sys.argv) < 2):
        print()
        print("ERROR: Only",len(sys.argv),"command line argument(s) provided.")
        print()
        print("Expected at least 3 after python:")
        print("  python3 regex.py \"<search-pattern>\" [\"<path-pattern-1>\"] [\"<path-pattern-2>\"] [...]")
        print()
        print("If no path patterns are given, all files are searched.")
        print("Documentation for this module follows.")
        print()
        print(__doc__)
        exit()
    regex = sys.argv[1]
    print("Using regex:",str([regex])[1:-1])
    path_patterns = sys.argv[2:]
    # Set the default path pattern to match all paths.
    if (len(path_patterns) == 0): path_patterns = [""]
    curdir = os.path.abspath(os.path.curdir)
    # Do a fast regular expression search.
    matches = frex(regex, *path_patterns, curdir)
    total_matches = sum(matches.values())
    if (total_matches > 0):
        print(f"\n found {total_matches} matches across {len(matches)} files")
    else:
        print("\n no matches found")


# 2020-10-06 20:40:46
# 
        ############################################################
        # # Use parallelism if there are a lot of files to search. #
        # from multiprocessing import Pool                         #
        # thread_pool = Pool()                                     #
        ############################################################


# 2020-10-06 20:41:13
# 
        ##############################################
        # # Close the thread pool if it was created. #
        # if (len(matches) > 100):                   #
        #     thread_pool.close()                    #
        ##############################################
