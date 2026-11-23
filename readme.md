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

### C: bounded aggregate matching

The library remains a single C file, `regex/regex.c`. Compile it alongside
an application, or include it directly in one translation unit.

`matchan` finds aggregate matches in a byte slice **without making a
NUL-terminated copy**. It is useful for editor lines and other views into a
larger text buffer:

```c
#include <stddef.h>
#include <stdlib.h>

void matchan(const char *regex, const char *text, size_t len,
             int *n, int **starts, int **ends);

/* No terminator required, and the trailing '!' is outside the slice. */
const char line[] = {'a', 'b', 'a', '!'};
int n = 16; /* maximum matches; <= 0 selects the default cap of 1024 */
int *starts = NULL, *ends = NULL;
matchan("a", line, 3, &n, &starts, &ends);
/* On success: n == 2, spans [0, 1) and [2, 3). */
/* Handle n < 0 before reading match spans. */
free(starts); /* ends points into the same allocation: never free it */
```

Contract:

- `len` is a maximum readable byte count, not a Unicode character count.
  No byte at or after `text[len]` is read. An earlier NUL still terminates
  matching. This is a text-slice API, not a binary-data regex API.
- The results equal `matcha` on a NUL-terminated copy of those bytes. The
  slice beginning is the start-anchor position and its bound is a virtual
  NUL/end of text. Existing `matcha` behavior is unchanged.
- Matches can overlap. If the same start is discovered again, its longest
  discovered end is retained. This is not a nonoverlapping search iterator
  or a guarantee of conventional leftmost-longest matching.
- Offsets are `int`, measured from the slice beginning; ends are exclusive.
  As in `matcha`, a negated token such as `{a}` can consume the terminating
  NUL and produce an end one beyond the text length. **Clamp spans to the
  actual text length before indexing the input.** This behavior is preserved
  for compatibility, including for zero-length slices.
- `text` may be NULL only when `len == 0`. A zero-length slice is still
  matched, so nullable patterns can return empty matches. `regex` must be
  NUL-terminated; `regex`, `n`, `starts`, and `ends` must be non-NULL.
- `len` must be at most `INT_MAX - 1`. This reserves space for the legacy
  terminating-NUL offset. Oversized lengths are rejected before reading
  text or allocating: `n == -7` (`REGEX_LENGTH_ERROR`) and both arrays NULL.
- On success, `n >= 0` is the number of returned matches. `n == 0` means no
  match. On syntax errors, `n == -1`, `starts[0]` is the negative error
  position and `ends[0]` is the existing regex error code. Allocation failure
  returns `n == -6` (`REGEX_MEMORY_ERROR`) and NULL arrays.
- Always free `starts` after a call, including syntax errors; `free(NULL)`
  is safe. Never free `ends` separately. Input is not modified or retained.

The legacy `matcha` interface still accepts NUL-terminated strings and
shares the same matcher. `matchan` is an additive C API; Python and WebAssembly
wrappers retain their existing interfaces.

Known existing engine limitation: repeating a nullable group, such as
`(a?)*`, can fail to terminate. Avoid those expressions. A bounded input
limits memory reads, not matcher execution time; this helper does not change
that behavior.

### C tests

Run all C suites from the repository root:

```sh
sh test.sh
```

The suites cover existing engine behavior, literal-set search regressions
across string/file APIs, bounded slices, anchors, overlaps, nullable patterns,
embedded NULs, high bytes, match caps, allocation failures, and 117,150
slice-versus-copy comparisons. The bounded tests track allocation balance
and inject failure at every allocation point in representative paths.

For address and undefined-behavior checks (where supported):

```sh
CFLAGS='-std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' sh test.sh
```

On hosts where LeakSanitizer cannot run under a tracer, set
`ASAN_OPTIONS=detect_leaks=0`; this disables only leak detection, not the
address/undefined-behavior checks. The bounded suite's allocation accounting
still checks its success and injected-failure paths.

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
// ___________________________________________________________________
//                            regex.c
// 
// DESCRIPTION
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
//  The regular expression matcher is accessible through the functions:
// 
//   void match(regex, string, start, end)
//     (const char *) regex -- A null-terminated simple regular expression.
//     (const char *) string -- A null-terminated character array.
//     (int *) start -- The (pass by reference) start of the first match.
//                      If there is no match this value is -1.
//     (int *) end -- The (pass by reference) end (noninclusive) of the
//                    first match. If *start is negative, contains an
//                    error code (0 for no match).
//
//   int label(regex, string, labels, groups, group_spans)
//     Label each byte in an exact match with its compiled token and group index.
//     Caller must free returned "labels", "groups", and "group_spans" pointers.
//
//   void matcha(regex, string, n, starts, ends)
//     Find aggregate matches in a null-terminated string.
//     Set *n > 0 before calling to cap returned matches; *n <= 0 uses 1024.
//     Caller must free "starts"; do not free "ends".
//
//   void fmatcha(regex, path, n, starts, ends, lines, min_ascii_ratio)
//     Find aggregate matches in a file at a given path.
//     Set *n > 0 before calling to cap returned matches; *n <= 0 uses 1024.
//     Caller must free "starts"; do not free "ends" or "lines".
// 
//
// BEHAVIOR
//  match, matcha, and fmatcha search past leading text. Begin a regex
//  with "{.}" to match only from the start. match() is first-discovered, not
//  greedy longest. matcha() and fmatcha() may return overlapping starts; when
//  the same start is discovered again, the longest discovered end is kept.
//  The "|" operator applies to the neighboring token (group) unless
//  explicit groups are used. NUL terminates string APIs. Nullable
//  matcha patterns may return zero-length and overlapping-looking
//  spans. fmatcha line numbers report where the match completes.
// 
// 
// ERROR CODES
//  Regex errors are returned in "end" when "start<0" for match,
//  and in "starts[0]" / "ends[0]" when "n == -1" for matcha and fmatcha.
//   0   Successful execution, no match found.
//  -1   Regex error, no tokens.
//  -2   Regex error, a group starting with '(', '[', or '{' is
//        not terminated.
//  -3   Regex error, bad syntax (i.e., starts with ')', ']', '}',
//        '*', '?', or '|', empty second argument to '|', or bad token
//        preceding '*' or '?').
//  -4   Regex error, empty group, contains "()", "[]", or "{}".
//  -5   No exact match found by label.
//  -6   Failed memory allocation.
//
//  fmatcha returns "n == -2" for file errors and "n == -3" when the
//  sampled ASCII ratio is below "min_ascii_ratio".
// 
//
// COMPILATION
//  Compile shared object (importable by Python) with:
//    cc -O3 -fPIC -shared -o regex.so regex.c
// 
//  Compile and run test (including debugging print statements) with:
//    cc -o test_regex test_regex.c && ./test_regex
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
//    ^     include "{.}" at the beginning of the regex
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
// 
// ___________________________________________________________________
```
