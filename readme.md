<p align="center">
  <h1 align="center"><code>regex</code></h1>
</p>

<p align="center">
A fast regular expression matcher written in C and accessible from Python3.
</p>


## INSTALLATION:

  Install the latest stable release to Python with:

```bash
pip install https://github.com/tchlux/regex/archive/1.1.0.zip
```

  In order to install the current files in this repository
  (potentially less stable) use:

```bash
pip install git+https://github.com/tchlux/regex.git
```

  This module can also be installed by simply copying out the `regex`
  subdirectory and placing it anywhere in the Python path.

## USAGE:

### Python

```python
import regex
reg = "rx"

string = "find the symbol 'rx' in this string"
print(regex.match(reg, string))

string = "the symbol is missing here"
print(regex.match(reg, string))
```

  Descriptions of the `match` function is contained in the `help`
  documentation. Descriptions of the full list of allowed regular
  expression syntax can be seen with `import regex; help(regex)`.

### Command line

```bash
python3 -m regex "<search-pattern>" "<path-pattern-1>" ["<path-pattern-2>"] [...]
```

  Or

```bash
python3 regex.py "<search-pattern>" "<path-pattern-1>" ["<path-pattern-2>"] [...]
```

  Search for the given regular expression in any files that match
  the path pattern regular expression. This will recurse through the
  subdirectory tree from the current directory.

## HOW IT WORKS:

  This regular expression language is dramatically simplified for
  speed. The expression is converted into a set of tokens that must
  be matched, along with jump conditions that point to other tokens
  when a character in the searched string does and doesn't match the
  active token in the regular expression.

## LANGUAGE SPECIFICATION FROM [`regex.c`](regex/regex.c)

```C
//  This file provides code for very fast regular expression matching
//  over character arrays for regular expressions. Only accepts
//  regular expressions of a reduced form, with syntax:
//
//     .   any character
//     *   0 or more repetitions of the preceding token (group)
//     ?   0 or 1 occurrence of the preceding token (group)
//     |   logical OR, either match the preceding token (group) or 
//         match the next token (group)
//     ()  define a token group (designate order of operations)
//     []  define a token set of character literals (single token
//         observed as any character from a collection), use this to
//         capture any (special) characters except ']'
//     {}  logical NOT, reverse behavior of successful and failed matches
//
//  The regular expression matcher is accessible through the function:
// 
//   void match(regex, string, start, end)
//     (const char *) regex -- A null-terminated simple regular expression.
//     (const char *) string -- A null-terminated character array.
//     (int *) start -- The (pass by reference) start of the first match.
//                      If there is no match this value is -1.
//     (int *) end -- The (pass by reference) end (noninclusive) of the
//                    first match. If *start is -1, contains error code.
// 
// 
// ERROR CODES
//  These codes are returned in "end" when "start<0".
//   0  Successful execution, no match found.
//   1  Regex error, no tokens.
//   2  Regex error, unterminated token set of character literals.
//   3  Regex error, bad syntax (i.e., starts with ')', '*', '?',
//        or '|', empty second argument to '|', or bad token
//        preceding '*' or '?').
//   4  Regex error, empty group, contains "()", "[]", or "{}".
//   5  Regex error, a group starting with '(', '[', or '{' is
//        not terminated.
// 
//
// COMPILATION
//  Compile shared object (importable by Python) with:
//    cc -O3 -fPIC -shared -o regex.so regex.c
// 
//  Compile and run test (including debugging print statements) with:
//    cc -o test_regex regex.c && ./test_regex
// 
//
// EXAMPLES:
//  Match any text contained within square brackets.
//    ".*[[].*].*" 
//
//  Match any dates of the form YYYY-MM-DD or YYYY/MM/DD.
//    ".*[0123456789][0123456789][0123456789][0123456789][-/][0123456789][0123456789][-/][0123456789][0123456789].*"
//
//  Match any comments that only have horizontal whitespace before them in a C code file.
//    ".*\n[ \t\r]*//.*"
// 
//  Match this entire documentation block in this C file.
//    "/.*\n{/}"
// 
//  Match "$TEXT$" where TEXT does not contain "\n\n" (two sequential new lines).
//    "$(({\n}\n?)|(\n?{\n}))*$"
//
//
// NOTES:
// 
//  A more verbose format is required to match common character
//  collections. These substitutions can easily be placed in regular
//  expression pre-processing code. Some substitution examples:
// 
//    +     replace with one explicit copy of preceding token (group),
//            followed by a copy with '*'
//    ^     this is implicitly at the beginning of all regex'es,
//            disable by including ".*" at the beginning of the regex
//    $     include "{.}" at the end of the regex
//    [~ab] replace with {[ab]}
//    \d    replace with "[0123456789]"
//    \D    replace with "{[0123456789]}"
//    \s    replace with "[ \t\n\r]"
//    {n}   replace with n occurrences of the preceding group
//    {n,}  replace with n occurrences of the preceding group,
//            followed by a copy with '*'
//    {n,m} replace with n occurrences of the preceding group, then
//            m-n repetitions of the group all with '?'
```

