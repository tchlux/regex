'''--------------------------------------------------------------------
                            regex.py
 
 A fast regular expression matching code for Python, built on top of
 the 'regex.c' library. The main function provided here is:

   match(regex, string) -> (start, end) or None or RegexError,

 Documentation for 'regex.c' follows.

''' 

import os, ctypes

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
# 
def match(regex, string):
    greedy = None
    # Do substitutions that make the underlying regex implementation
    # behave more like common existing regex packages.
    if (len(regex) > 0):
        # Add a ".*" to the front of the regex if the beginning of the
        # string was not explicitly desired in the match pattern.
        if (regex[0] == "^"): regex = regex[1:]
        elif ((regex[0] != ".") or (regex[1] != '*')): regex = ".*" + regex
        # Replace any "**" with greedy matches that incorporate a
        # negation after the preceding token.
        if ("**" in regex):
            i = regex.index("**")
            greedy = regex[i-1]
            if ((i > 0) and (greedy not in {')', ']', '}'})):
                regex = regex[:i] + f"*{{{greedy}}}" + regex[i+2:]
            print("regex:", str([regex])[1:-1])

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

    # TODO: only allow the last token to be greedy
    # TODO: remove the extra character that may have been added by "greedy"

    # Return appropriately.
    if (start >= 0): return (start, end)
    elif (start < 0):
        if (end == 0): return None # no match found
        elif (end == -1): return (0, 0) # empty regular expression
        elif ((start == -1) and (end == -5)): return None  # empty string
        elif (end < -1): # error code provided by C
            err = f"Invalid regular expression (code {-end} {start})"
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

# Main for verification that it is working.
if __name__ == "__main__":

    # regex  = "^[*][*]*{[*]}"
    # string = "*** test"

    # # Find any "**" patterns.
    # regex  = "{[*]}[*][*]{[*]}"
    # string = "ab*  **"

    # Find any "**" patterns.
    regex  = "^aa**"   # <- find most 'a' at beginning
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
