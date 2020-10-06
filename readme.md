<p align="center">
  <h1 align="center">regex</h1>
</p>

<p align="center">
A fast regular expression matcher written in C and accessible from Python3.
</p>


## INSTALLATION:

  Install the latest stable release with:

```bash
pip install https://github.com/tchlux/regex/archive/1.0.0.zip
```

  In order to install the current files in this repository
  (potentially less stable) use:

```bash
pip install git+https://github.com/tchlux/regex.git
```

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
python3 -m regex "expression" <file_path_1> <file_path_2> ...
```

  Search for the given regular expression in each provided file.

## HOW IT WORKS:

  This regular expression language is dramatically simplified for
  speed. The expression is converted into a set of tokens that must
  be matched, along with jump conditions that point to other tokens
  when a character in the searched string does and doesn't match the
  active token in the regular expression.
