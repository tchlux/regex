<p align="center">
  <h1 align="center"><code>regex</code></h1>
</p>

<p align="center">
A fast regular expression matcher written in C and accessible from Python3.
</p>


## INSTALLATION:

  Install the latest stable release to Python with:

```bash
pip install https://github.com/tchlux/regex/archive/1.0.0.zip
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
