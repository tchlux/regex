// cc -o test_regex test_regex.c && ./test_regex * ; rm -f ./test_regex

#define DEBUG

#ifdef DEBUG
#include <stdio.h>  // EOF
// Define a global flag for determining if interior prints should be done.
int DO_PRINT = 0;
// Define a global character array for safely printing escaped characters.
char CHAR3[3];
// Define a function for safely printing a character (escapes whitespace).
char* SAFE_CHAR(const char c) {
  CHAR3[0] = '\\';
  CHAR3[1] = '\0';
  CHAR3[2] = '\0';
  if (c == '\n') CHAR3[1] = 'n';
  else if (c == '\t') CHAR3[1] = 't';
  else if (c == '\r') CHAR3[1] = 'r';
  else if (c == '\0') CHAR3[1] = '0';
  else if (c == EOF)  CHAR3[1] = 'X';
  else CHAR3[0] = c;
  return (char *) CHAR3;
}
#endif

// Include the source code for regex here in the tests.
#include "regex.c"


// ===================================================================
//                  BEGIN   T E S T I N G   CODE
// ===================================================================


#ifdef DEBUG
int run_tests(); // <- actually declared later
// For testing purposes.
int main(int argc, char * argv[]) {
  // =================================================================
  // Manual test of `match`.. (use "if (1)" to run, "if (0)" to skip)
  if (0) {
    // ------------------------------------------
    // char * regex = "((\r\n)|\r|\n)";
    // char * string = "\r\n**** \n";
    // ------------------------------------------
    // char * regex = "$(({\n}\n?)|(\n?{\n}))*$";
    // char * string = "$\n  testing \n$";
    // ------------------------------------------
    DO_PRINT = 1;
    char * regex = ".*st{.}";
    char * string = "| test";
    int start, end;
    match(regex, string, &start, &end);
    printf("==================================================\n\n");
    // Handle errors.
    if (start < 0) {
      if (end < 0) {
      printf("\nERROR: invalid regular expression, code %d", -end);
      if (start < -1) {
        printf(" error at position %d.\n", -start-1);
        printf("  %s\n", regex);
        printf("  %*c\n", -start, '^');
      } else {
        printf(".\n");
      }
        // Mark the failure in the match with return code.
        return (1);
      // No matches found in the search string.
      } else {
      printf("no match found\n");
      }
      // Matched.
    } else {
      printf("match at (%d -> %d)\n", start, end);
    }
    // Print out the matched expression.
    if (start >= 0) {
      printf("\n\"");
      for (int j = start; j < end; j++)
      printf("%c",string[j]);
      printf("\"\n");
    }
    return 0;
  // =================================================================
  // Manual test of `fmatcha`.. (use "if (1)" to run, "if (0)" to skip)
  } else if (0) {
    DO_PRINT = 1;
    char * regex = ".*hehe";
    // char * path = "regex.so";
    char * path = "test.txt";
    int n_matches;
    int * starts;
    int * ends;
    int * lines;
    float min_ascii_ratio = 0.5;
    fmatcha(regex, path, &n_matches, &starts, &ends, &lines, min_ascii_ratio);
    printf("==================================================\n\n");
    // Handle errors.
    if (n_matches == -3) {
      printf("\nERROR: too many non-ASCII characters in file");
      return(3);
    } else if (n_matches == -2) {
      printf("\nERROR: failed to load file");
      return(2);
    } else if (n_matches == -1) {
      if (starts[0] > 0) {
        if (ends[0] > 0) {
          printf("\nERROR: invalid regular expression, code %d", ends[0]);
          if ((long) starts[0] > 1) {
            printf(" error at position %d.\n", starts[0]-1);
            printf("  ");
            for (int i = 0; regex[i] != '\0'; i++)
              printf("%s", SAFE_CHAR(regex[i]));
            printf("\n");
            printf("  %*c\n", starts[0], '^');
          } else {
            printf(".\n");
          }
          // Mark the failure in the match with return code.
          return (1);
        // No matches found in the search string.
        } else {
          printf("ERROR: unexpected execution flow, (n_matches = -1) and match at (%d -> %d)\n", starts[0], ends[0]);
          return(4);
        }
      }
      printf("ERROR: unexpected execution flow, (n_matches = -1) and match at (%ld -> %ld)\n", (long) starts, (long) ends);
      return(4);
    } else {
      // Print out the matched expression.
      printf("\n");
      for (int i = 0; i < n_matches; i++) {
        printf("  %d -> %d\n", starts[i], ends[i]);
      }
      return 0;
    }
  // =================================================================
  } else {
    return(run_tests());
  }
}

int run_tests() {

  // Custom type for test cases.
  typedef struct {
    char *regex;
    int n_tokens;
    int n_groups;
    char *tokens;
    int *jumps;
    int *jumpf;
    char *jumpi;
    char *string;
    int match_start;
    int match_end;
  } regex_test_case;

  // Test cases for the regular expression engine.
  // 
  //  {
  //     regex,           number-of-tokens,  number-of-groups,
  //     tokens-prefixed, jump-on-match,     jump-on-mismatch,  jump-immediate-for-OR,
  //     string,          match-start-index, match-end-index
  //   }
  //
  regex_test_case test_cases[] = {
    // Invalid regular expressions.
    { "*abc", -1, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -1, REGEX_SYNTAX_ERROR },

    { "?abc", -1, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -1, REGEX_SYNTAX_ERROR },

    { "|abc", -1, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -1, REGEX_SYNTAX_ERROR },

    { ")abc", -1,    REGEX_SYNTAX_ERROR,
      "",     NULL,     NULL,     NULL,
      " ",    -1,    REGEX_SYNTAX_ERROR },

    { "}abc", -1, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -1, REGEX_SYNTAX_ERROR },

    { "]abc", -1, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -1, REGEX_SYNTAX_ERROR },

    { "abc|", -4, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -4, REGEX_SYNTAX_ERROR },

    { "abc|*", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc|?", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc|)", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc|]", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc|}", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "(|a)", -2, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -2, REGEX_SYNTAX_ERROR },

    { "a||b", -3, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -3, REGEX_SYNTAX_ERROR },

    { "abc**", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc*?", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc?*", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc??", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc(*", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc(?", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc{*", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc{?", -5, REGEX_SYNTAX_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_SYNTAX_ERROR },

    { "abc(", -5, REGEX_UNCLOSED_GROUP_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -5, REGEX_UNCLOSED_GROUP_ERROR },

    { "abc{", -5, REGEX_UNCLOSED_GROUP_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -5, REGEX_UNCLOSED_GROUP_ERROR },

    { "abc()", -5, REGEX_EMPTY_GROUP_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_EMPTY_GROUP_ERROR },

    { "abc{}", -5, REGEX_EMPTY_GROUP_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_EMPTY_GROUP_ERROR },

    { "abc[]", -5, REGEX_EMPTY_GROUP_ERROR,
      "",      NULL, NULL, NULL,
      " ",     -5, REGEX_EMPTY_GROUP_ERROR },

    { "(a}", -3, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -3, REGEX_SYNTAX_ERROR },

    { "{a)", -3, REGEX_SYNTAX_ERROR,
      "",     NULL, NULL, NULL,
      " ",    -3, REGEX_SYNTAX_ERROR },

    { "a]", -2, REGEX_SYNTAX_ERROR,
      "",    NULL, NULL, NULL,
      " ",   -2, REGEX_SYNTAX_ERROR },

    // Valid regular expressions.

    { ".",    1, 0,
      ".",    (int[]){1}, (int[]){-1}, (char[]){0},
      " abc", 0, 1 },

    { ".*", 2, 0,
      "*.", (int[]){1,0}, (int[]){2,-1}, (char[]){0,0},
      ".*", 0, 0 },

    { "..", 2, 0,
      "..", (int[]){1,2}, (int[]){-1,-1}, (char[]){0,0},
      "..", 0, 2 },

    { " (.|.)*d", 6, 1,
      " *|..d",   (int[]){1,2,3,1,1,6}, (int[]){-1,5,4,-1,-1,-1}, (char[]){0,0,0,0,0,0},
      " (.|.)*d", 0, 8 },

    { ".* .*ad", 7, 0,
      "*. *.ad", (int[]){1,0,3,4,3,6,7}, (int[]){2,-1,-1,5,-1,-1,-1}, (char[]){0,0,0,0,0,0,0},
      ".* .*ad", 0, 7 },

    { "abc",  3, 0,
      "abc",  (int[]){1,2,3}, (int[]){-1,-1,-1}, (char[]){0,0,0},
      " abc", 1, 4 },

    { ".*abc", 5, 0,
      "*.abc", (int[]){1,0,3,4,5}, (int[]){2,-1,-1,-1,-1}, (char[]){0,0,0,0,0},
      "      abc", 0, 9 },

    { ".((a*)|(b*))*.", 8, 3,
      ".*|*a*b.",       (int[]){1,2,3,4,3,6,5,8}, (int[]){-1,7,5,7,-1,1,-1,-1}, (char[]){0,0,0,0,0,0,0,0},
      " aabbb ",        0, 2 },

    { "(abc)", 3, 1,
      "abc",   (int[]){1,2,3}, (int[]){-1,-1,-1}, (char[]){0,0,0},
      "abc",   0, 3 },

    { "[abc]", 3, 1,
      "abc",   (int[]){3,3,3}, (int[]){1,2,-1}, (char[]){1,1,2},
      "c",     0, 1 },

    { "{abc}", 3, 1,
      "abc",   (int[]){-1,-1,-1}, (int[]){1,2,3}, (char[]){0,0,0},
      "ddd",   0, 3 },

    { "{[abc]}", 3, 2,
      "abc",     (int[]){-1,-1,-1}, (int[]){1,2,3}, (char[]){1,1,2},
      "d",       0, 1 },

    { "{{[abc]}}", 3, 3,
      "abc",       (int[]){3,3,3}, (int[]){1,2,-1}, (char[]){1,1,2},
      "c",         0, 1 },

    { "[ab][ab]", 4, 2,
      "abab",     (int[]){2,2,4,4}, (int[]){1,-1,3,-1}, (char[]){1,2,1,2},
      "ba",       0, 2 },

    { "{[ab][ab]}", 4, 3,
      "abab",       (int[]){-1,-1,-1,-1}, (int[]){1,2,3,4}, (char[]){1,2,1,2},
      "cd",         0, 2 },

    { "a*bc", 4, 0,
      "*abc", (int[]){1,0,3,4}, (int[]){2,-1,-1,-1}, (char[]){0,0,0,0},
      "aabc", 0, 4 },

    { "(ab)*c", 4, 1,
      "*abc",   (int[]){1,2,0,4}, (int[]){3,-1,-1,-1}, (char[]){0,0,0,0},
      "ababc",  0, 5 },

    { "[ab]*c", 4, 1,
      "*abc",   (int[]){1,0,0,4}, (int[]){3,2,-1,-1}, (char[]){0,1,2,0},
      "baabc",  0, 5 },

    { "{ab}*c", 4, 1,
      "*abc",   (int[]){1,-1,-1,4}, (int[]){3,2,0,-1}, (char[]){0,0,0,0},
      "zzdc",   -1, 0 },

    { "[a][b]*{[c]}", 4, 4,
      "a*bc",         (int[]){1,2,1,-1}, (int[]){-1,3,-1,4}, (char[]){2,0,2,2},
      "ad",           0, 2 },

    { "{{a}[bcd]}", 4, 3,
      "abcd",       (int[]){1,-1,-1,-1}, (int[]){-1,2,3,4}, (char[]){0,1,1,2},
      "azw",        0, 2 },

    { "a{[bcd]}e", 5, 2,
      "abcde",     (int[]){1,-1,-1,-1,5}, (int[]){-1,2,3,4,-1}, (char[]){0,1,1,2,0},
      "afe",       0, 3 },

    { "{{a}[bcd]{e}}", 5, 4,
      "abcde",         (int[]){1,-1,-1,-1,5}, (int[]){-1,2,3,4,-1}, (char[]){0,1,1,2,0},
      "age",           0, 3 },

    { "(a(bc)?)*(d)", 6, 3,
      "*a?bcd",       (int[]){1,2,3,4,0,6}, (int[]){5,-1,0,-1,-1,-1}, (char[]){0,0,0,0,0,0},
      "abcabcd",      0, 7 },

    { "(a(bc*)?)|d", 7, 2,
      "|a?b*cd",     (int[]){1,2,3,4,5,4,7}, (int[]){6,-1,7,-1,7,-1,-1}, (char[]){0,0,0,0,0,0,0},
      "d",           0, 1 },

    { "{a(bc*)?}|d", 7, 2,
      "|a?b*cd",     (int[]){1,-1,3,-1,5,-1,7}, (int[]){6,2,7,4,7,4,-1}, (char[]){0,0,0,0,0,0,0},
      "zdb",         0, 1 },

    { "{(a(bc*)?)}|d", 7, 3,
      "|a?b*cd",       (int[]){1,-1,3,-1,5,-1,7}, (int[]){6,2,7,4,7,4,-1}, (char[]){0,0,0,0,0,0,0},
      "d",             0, 1 },

    { "(a(bc)?)|(de)", 7, 3,
      "|a?bcde",       (int[]){1,2,3,4,7,6,7}, (int[]){5,-1,7,-1,-1,-1,-1}, (char[]){0,0,0,0,0,0,0},
      "abc",           0, 1 },

    { "a( |(_))[bc]", 6, 3,
      "a| _bc",       (int[]){1,2,4,4,6,6}, (int[]){-1,3,-1,-1,5,-1}, (char[]){0,0,0,0,1,2},
      "a b",          0, 3 },

    { "(a(z.)*)[bc]*d*", 9, 3,
      "a*z.*bc*d",       (int[]){1,2,3,1,5,4,4,8,7}, (int[]){-1,4,-1,-1,7,6,-1,9,-1}, (char[]){0,0,0,0,0,1,2,0,0},
      "az.bcd",          0, 1 },

    { "(a(z.)*)[bc]*d*{e}f?g", 13, 4,
      "a*z.*bc*de?fg",         (int[]){1,2,3,1,5,4,4,8,7,-1,11,12,13}, (int[]){-1,4,-1,-1,7,6,-1,9,-1,10,12,-1,-1}, (char[]){0,0,0,0,0,1,2,0,0,0,0,0,0},
      "aztzsbcdfg",            0, 10 },

    { "(a(z.)*)[bc]*d*{e}f?g|h", 15, 4,
      "a*z.*bc*de?f|gh",         (int[]){1,2,3,1,5,4,4,8,7,-1,11,12,13,15,15}, (int[]){-1,4,-1,-1,7,6,-1,9,-1,10,12,-1,14,-1,-1}, (char[]){0,0,0,0,0,1,2,0,0,0,0,0,0,0,0},
      "aztzsbcdh",               0, 9 },

    { "({({ab}c?)*d}|(e(fg)?))", 11, 6,
      "|*ab?cde?fg",             (int[]){1,2,3,4,5,-1,-1,8,9,10,11}, (int[]){7,6,-1,-1,1,1,11,-1,11,-1,-1}, (char[]){0,0,0,0,0,0,0,0,0,0,0},
      "abdabc",                  0, 1 },

    { "({({[ab]}c?)*d}|(e(fg)?))", 11, 7,
      "|*ab?cde?fg",               (int[]){1,2,4,4,5,-1,-1,8,9,10,11}, (int[]){7,6,3,-1,1,1,11,-1,11,-1,-1}, (char[]){0,0,1,2,0,0,0,0,0,0,0},
      "efg",                       0, 1 },

    { "({(a)({[bc]}d?e)*(f)}|g(hi)?)", 13, 8,
      "|a*bc?defg?hi",                 (int[]){1,-1,3,5,5,6,-1,-1,-1,10,11,12,13}, (int[]){9,2,8,4,-1,7,7,2,10,-1,13,-1,-1}, (char[]){0,0,0,1,2,0,0,0,0,0,0,0,0},
      "gf",                            0, 1 },

    { "[*][*]*{[*]}", 4, 4,
      "****",         (int[]){1,2,1,-1}, (int[]){-1,3,-1,4}, (char[]){2,0,2,2},
      "*** test",     0, 4 },

    { "[[][[]",  2, 2,
      "[[",      (int[]){1,2}, (int[]){-1,-1}, (char[]){2,2},
      "[[ test", 0, 2 },

    { ".*[)][)]", 4, 2,
      "*.))",     (int[]){1,0,3,4}, (int[]){2,-1,-1,-1}, (char[]){0,0,2,2},
      "test ))",  0, 7 },

    { ".*end{.}",          6, 1,
      "*.end.",            (int[]){1,0,3,4,5,-1}, (int[]){2,-1,-1,-1,-1,6}, (char[]){0,0,0,0,0,0},
      " does it ever end", 0, 17 },

    { "[|]",    1, 1,
      "|",      (int[]){1}, (int[]){-1}, (char[]){2},
      "| test", 0, 1 },

    { "{[.]}*{.}", 3, 3,
      "*..",       (int[]){1,-1,-1}, (int[]){2,0,3}, (char[]){0,2,0},
      "anything",  0, 8 },

    { "[a]*{[a]}", 3, 3,
      "*aa",      (int[]){1,0,-1}, (int[]){2,-1,3}, (char[]){0,2,2},
      "abba",     0, 2 },

    { "[*]*{[*]}", 3, 3,
      "****",      (int[]){1,0,-1}, (int[]){2,-1,3}, (char[]){0,2,2},
      "*paths*",   0, 2 },

    { "[*]*{[*]}{[*]}", 4, 5,
      "****",           (int[]){1,0,-1,-1}, (int[]){2,-1,3,4}, (char[]){0,2,2,2},
      "*paths*",        0, 3 },

    { "a*", 2, 0,
      "*a", (int[]){1,0}, (int[]){2,-1}, (char[]){0,0},
      "",   0, 0 },

    { "a", 1, 0,
      "a", (int[]){1}, (int[]){-1}, (char[]){0},
      "",  -1, 0 },

    // Last test regular expression must be empty!

    { "", 0, 0,
      "", NULL, NULL, NULL,
      " ", -1, REGEX_NO_TOKENS_ERROR  }

  };


  int done = 0;
  int i = -1;  // test index
  while (done == 0) {
    i++; // increment test index counter
    regex_test_case *t = &test_cases[i];

    // ===============================================================
    //                          _count     
    // 
    // Count the number of tokens and groups in this regular expression.
    int n_tokens, n_groups;
    _count(t->regex, &n_tokens, &n_groups);

    // Verify the number of tokens and the number of groups..
    if (n_tokens != t->n_tokens) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
      printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n\n");
      printf("ERROR: Wrong number of tokens returned by _count.\n");
      printf(" expected %d\n", t->n_tokens);
      printf(" received %d\n", n_tokens);
      return(1);
    } else if (n_groups != t->n_groups) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
      printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n\n");
      printf("ERROR: Wrong number of groups returned by _count.\n");
      printf(" expected %d\n", t->n_groups);
      printf(" received %d\n", n_groups);
      return(2);
    }
    // ---------------------------------------------------------------

    if (n_tokens > 0) {
    // ===============================================================
    //                          _set_jump     
    // 
    // Initialize storage for tracking the current active tokens and
    // where to jump based on the string being parsed.
    const int mem_bytes = (2*n_tokens*sizeof(int) + 2*(n_tokens+1)*sizeof(char));
    int * jumps = malloc(mem_bytes); // jump-to location after success
    int * jumpf = jumps + n_tokens; // jump-to location after failure
    char * tokens = (char*) (jumpf + n_tokens); // regex index of each token (character)
    char * jumpi = tokens + n_tokens + 1; // immediately check next on failure
    // Terminate the two character arrays with the null character.
    tokens[n_tokens] = '\0';
    jumpi[n_tokens] = '\0';

    // Determine the jump-to tokens upon successful match and failed
    // match at each token in the regular expression.
    _set_jump(t->regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);

    // Verify the the tokens, jumps, jumpf, and jumpi..
    for (int j = 0; j < n_tokens; j++) {
      if (tokens[j] != t->tokens[j]) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
        printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n\n");
      // Re-run the code with debug printing enabled.
      DO_PRINT = 1;
      _set_jump(t->regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
      printf("\n");
      printf("ERROR: Wrong TOKEN returned by _set_jump.\n");
      printf(" expected '%s' as token %d\n", SAFE_CHAR(t->tokens[j]), j);
      printf(" received '%s'\n", SAFE_CHAR(tokens[j]));
      return(3);
      } else if (jumps[j] != t->jumps[j]) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
        printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n");
      // Re-run the code with debug printing enabled.
      DO_PRINT = 1;
      _set_jump(t->regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
      printf("\n");
      printf("ERROR: Wrong JUMP S returned by _set_jump.\n");
      printf(" expected %d in col 0, row %d\n", t->jumps[j], j);
      printf(" received %d\n", jumps[j]);
      return(4);
      } else if (jumpf[j] != t->jumpf[j]) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
        printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n");
      // Re-run the code with debug printing enabled.
      DO_PRINT = 1;
      _set_jump(t->regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
      printf("\n");
      printf("ERROR: Wrong JUMP F returned by _set_jump.\n");
      printf(" expected %d in col 1, row %d\n", t->jumpf[j], j);
      printf(" received %d\n", jumpf[j]);
      return(5);
      } else if (jumpi[j] != t->jumpi[j]) {
      printf("\nRegex: '");
      for (int j = 0; t->regex[j] != '\0'; j++) {
        printf("%s", SAFE_CHAR(t->regex[j]));
      }
      printf("'\n");
      // Re-run the code with debug printing enabled.
      DO_PRINT = 1;
      _set_jump(t->regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
      printf("\n");
      printf("ERROR: Wrong JUMP I returned by _set_jump.\n");
      printf(" expected %d in col 2, row %d\n", t->jumpi[j], j);
      printf(" received %d\n", jumpi[j]);
      return(6);
      }
    }
    free(jumps); // free the allocated memory
    }
    // -------------------------------------------------------------


    // =============================================================
    //                          match     
    // 
    int start;
    int end;
    match(t->regex, t->string, &start, &end);

    if (start != t->match_start) {
          DO_PRINT = 1;
          match(t->regex, t->string, &start, &end);
          printf("\nString: '");
          for (int j = 0; t->string[j] != '\0'; j++) {
            printf("%s", SAFE_CHAR(t->string[j]));
          }
      printf("'\n\n");
      printf("ERROR: Bad match START returned by match.\n");
      printf(" expected %d\n", t->match_start);
      printf(" received %d\n", start);
          return(7);
    } else if (end != t->match_end) {
          DO_PRINT = 1;
          match(t->regex, t->string, &start, &end);      
          printf("\nString: '");
          for (int j = 0; t->string[j] != '\0'; j++) {
            printf("%s", SAFE_CHAR(t->string[j]));
          }
      printf("'\n\n");
      printf("ERROR: Bad match END returned by match.\n");
      printf(" expected %d\n", t->match_end);
      printf(" received %d\n", end);
          return(8);
    }

    // -------------------------------------------------------------

    // Exit once the empty regex has been verified.
    if (t->regex[0] == '\0') done++;
  }

  int n_matches;
  int * starts;
  int * ends;
  int * labels;
  int * group_labels;
  int * group_spans;

  int start;
  int end;
  match("abc", "xxabc", &start, &end);
  if ((start != 2) || (end != 5)) {
    printf("ERROR: Bad unanchored match returned by match.\n");
    return(32);
  }

  match("{.}abc", "abc", &start, &end);
  if ((start != 0) || (end != 3)) {
    printf("ERROR: Bad start-anchored match returned by match.\n");
    return(33);
  }

  match("{.}abc", "xxabc", &start, &end);
  if ((start != -1) || (end != 0)) {
    printf("ERROR: Bad start-anchored no-match returned by match.\n");
    return(34);
  }

  match("abc{.}", "xxabc", &start, &end);
  if ((start != 2) || (end != 5)) {
    printf("ERROR: Bad end-anchored match returned by match.\n");
    return(35);
  }

  match("({.}|[ \t\n\v\f\r])Try", "Try", &start, &end);
  if ((start != 0) || (end != 3)) {
    printf("ERROR: Bad branch start-anchor match returned by match.\n");
    return(50);
  }

  match("({.}|[ \t\n\v\f\r])Try", " Try", &start, &end);
  if ((start != 0) || (end != 4)) {
    printf("ERROR: Bad branch whitespace match returned by match.\n");
    return(51);
  }

  match("({.}|[ \t\n\v\f\r])Try", "xTry", &start, &end);
  if ((start != -1) || (end != 0)) {
    printf("ERROR: Bad branch start-anchor no-match returned by match.\n");
    return(52);
  }

  match("({.}|[ \t\n\v\f\r])Try", "x Try", &start, &end);
  if ((start != 1) || (end != 5)) {
    printf("ERROR: Bad restarted branch whitespace match returned by match.\n");
    return(53);
  }

  match("abc{.}", "abcxx", &start, &end);
  if ((start != -1) || (end != 0)) {
    printf("ERROR: Bad end-anchored no-match returned by match.\n");
    return(36);
  }

  match("{.}abc{", "", &start, &end);
  if ((start != -8) || (end != REGEX_UNCLOSED_GROUP_ERROR)) {
    printf("ERROR: Bad anchored regex error position returned by match.\n");
    return(37);
  }

  matcha("a*", "", &n_matches, &starts, &ends);
  if ((n_matches != 1) || (starts[0] != 0) || (ends[0] != 0)) {
    printf("ERROR: Bad empty-string match returned by matcha.\n");
    return(9);
  }
  free(starts);

  if (label("rx", "rx", &labels, &group_labels, &group_spans) != 2 ||
      labels[0] != 0 || labels[1] != 1 ||
      group_labels[0] != -1 || group_labels[1] != -1 || group_spans != NULL) {
    printf("ERROR: Bad literal labels returned by label.\n");
    return(10);
  }
  free(labels);
  free(group_labels);

  if (label("{.}abc", "abc", &labels, &group_labels, &group_spans) != 3 ||
      labels[0] != 0 || labels[1] != 1 || labels[2] != 2 ||
      group_labels[0] != -1 || group_labels[1] != -1 ||
      group_labels[2] != -1 || group_spans != NULL) {
    printf("ERROR: Bad start-anchored labels returned by label.\n");
    return(38);
  }
  free(labels);
  free(group_labels);

  if (label("{.}abc", "xxabc", &labels, &group_labels, &group_spans) != LABEL_NO_MATCH_ERROR ||
      labels != NULL || group_labels != NULL || group_spans != NULL) {
    printf("ERROR: Bad start-anchored label no-match returned by label.\n");
    return(39);
  }

  if (label("abc{.}", "abc", &labels, &group_labels, &group_spans) != 3 ||
      labels[0] != 0 || labels[1] != 1 || labels[2] != 2 ||
      group_labels[0] != -1 || group_labels[1] != -1 ||
      group_labels[2] != -1 || group_spans == NULL) {
    printf("ERROR: Bad end-anchored labels returned by label.\n");
    return(40);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("a*", "aaa", &labels, &group_labels, &group_spans) != 3 || labels[0] != 1 ||
      labels[1] != 1 || labels[2] != 1 || group_labels[0] != -1 ||
      group_labels[1] != -1 || group_labels[2] != -1 || group_spans != NULL) {
    printf("ERROR: Bad repeated-token labels returned by label.\n");
    return(11);
  }
  free(labels);
  free(group_labels);

  if (label(".*rx", "find the symbol 'rx", &labels, &group_labels, &group_spans) != 19) {
    printf("ERROR: Bad wildcard label length returned by label.\n");
    return(12);
  }
  for (int index = 0; index < 17; index++) {
    if ((labels[index] != 1) || (group_labels[index] != -1)) {
      printf("ERROR: Bad wildcard prefix labels returned by label.\n");
      return(13);
    }
  }
  if (labels[17] != 2 || labels[18] != 3 ||
      group_labels[17] != -1 || group_labels[18] != -1) {
    printf("ERROR: Bad wildcard suffix labels returned by label.\n");
    return(14);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("[ab]", "b", &labels, &group_labels, &group_spans) != 1 ||
      labels[0] != 1 || group_labels[0] != 0 ||
      group_spans[0] != 0 || group_spans[1] != 0) {
    printf("ERROR: Bad token-set labels returned by label.\n");
    return(15);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("{[ab]}", "c", &labels, &group_labels, &group_spans) != 1 ||
      labels[0] != 1 || group_labels[0] != 1 ||
      group_spans[0] != 0 || group_spans[1] != 1 ||
      group_spans[2] != 1 || group_spans[3] != 1) {
    printf("ERROR: Bad negated token-set labels returned by label.\n");
    return(16);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("(rx)", "rx", &labels, &group_labels, &group_spans) != 2 ||
      group_labels[0] != 0 || group_labels[1] != 0 ||
      group_spans[0] != 0 || group_spans[1] != 0) {
    printf("ERROR: Bad parenthesized group labels returned by label.\n");
    return(17);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("a([bc])", "ab", &labels, &group_labels, &group_spans) != 2 ||
      group_labels[0] != -1 || group_labels[1] != 1 ||
      group_spans[0] != 0 || group_spans[1] != 1 ||
      group_spans[2] != 1 || group_spans[3] != 1) {
    printf("ERROR: Bad nested group labels returned by label.\n");
    return(18);
  }
  free(labels);
  free(group_labels);
  free(group_spans);

  if (label("aa", "aaa", &labels, &group_labels, &group_spans) != LABEL_NO_MATCH_ERROR ||
      labels != NULL || group_labels != NULL || group_spans != NULL) {
    printf("ERROR: Bad exact-window no-match returned by label.\n");
    return(19);
  }

  if (label("a*", "", &labels, &group_labels, &group_spans) != 0 ||
      labels != NULL || group_labels != NULL || group_spans != NULL) {
    printf("ERROR: Bad empty-match labels returned by label.\n");
    return(20);
  }

  matcha("a", "", &n_matches, &starts, &ends);
  if (n_matches != 0) {
    printf("ERROR: Bad empty-string no-match returned by matcha.\n");
    return(21);
  }

  matcha("abc", "xxabc", &n_matches, &starts, &ends);
  if ((n_matches != 1) || (starts[0] != 2) || (ends[0] != 5)) {
    printf("ERROR: Bad unanchored matches returned by matcha.\n");
    return(41);
  }
  free(starts);

  matcha("{.}abc", "abc abc", &n_matches, &starts, &ends);
  if ((n_matches != 1) || (starts[0] != 0) || (ends[0] != 3)) {
    printf("ERROR: Bad start-anchored matches returned by matcha.\n");
    return(42);
  }
  free(starts);

  matcha("{.}abc", "xxabc", &n_matches, &starts, &ends);
  if (n_matches != 0) {
    printf("ERROR: Bad start-anchored no-match returned by matcha.\n");
    return(43);
  }

  matcha("abc{.}", "xxabc", &n_matches, &starts, &ends);
  if ((n_matches != 1) || (starts[0] != 2) || (ends[0] != 5)) {
    printf("ERROR: Bad end-anchored matches returned by matcha.\n");
    return(44);
  }
  free(starts);

  matcha("abc{.}", "abcxx", &n_matches, &starts, &ends);
  if (n_matches != 0) {
    printf("ERROR: Bad end-anchored no-match returned by matcha.\n");
    return(45);
  }

  matcha("a|a", "baa", &n_matches, &starts, &ends);
  if ((n_matches != 2) || (starts[0] != 1) || (ends[0] != 2) ||
      (starts[1] != 2) || (ends[1] != 3)) {
    printf("ERROR: Bad restarted match returned by matcha.\n");
    return(20);
  }
  free(starts);

  matcha("aa", "aaaa", &n_matches, &starts, &ends);
  if ((n_matches != 2) || (starts[0] != 0) || (ends[0] != 2) ||
      (starts[1] != 2) || (ends[1] != 4)) {
    printf("ERROR: Bad nonoverlapping matches returned by matcha.\n");
    return(21);
  }
  free(starts);

  matcha("a?", "a", &n_matches, &starts, &ends);
  if ((n_matches != 3) || (starts[0] != 0) || (ends[0] != 0) ||
      (starts[1] != 0) || (ends[1] != 1) ||
      (starts[2] != 1) || (ends[2] != 1)) {
    printf("ERROR: Bad deduplicated matches returned by matcha.\n");
    return(22);
  }
  free(starts);

  matcha("a( |(_))[bc]", "a b|a_c|a x", &n_matches, &starts, &ends);
  if ((n_matches != 2) || (starts[0] != 0) || (ends[0] != 3) ||
      (starts[1] != 4) || (ends[1] != 7)) {
    printf("ERROR: Bad grouped branch matches returned by matcha.\n");
    return(54);
  }
  free(starts);

  int * lines;
  fmatcha("hello", "regex/test.txt", &n_matches, &starts, &ends, &lines, 0.5);
  if ((n_matches != 3) || (starts[0] != 13) || (ends[0] != 18) ||
      (lines[0] != 3) || (starts[1] != 69) || (ends[1] != 74) ||
      (lines[1] != 8) || (starts[2] != 75) || (ends[2] != 80) ||
      (lines[2] != 9)) {
    printf("ERROR: Bad nonoverlapping matches returned by fmatcha.\n");
    return(23);
  }
  free(starts);

  char * anchor_path = "/tmp/regex_anchor_test.txt";
  FILE * anchor_file = fopen(anchor_path, "wb");
  if (anchor_file == NULL) {
    printf("ERROR: Failed to create temporary anchor test file.\n");
    return(46);
  }
  fputs("xxabc", anchor_file);
  fclose(anchor_file);

  fmatcha("{.}xx", anchor_path, &n_matches, &starts, &ends, &lines, 0.5);
  if ((n_matches != 1) || (starts[0] != 0) || (ends[0] != 2)) {
    printf("ERROR: Bad start-anchored matches returned by fmatcha.\n");
    return(47);
  }
  free(starts);

  fmatcha("{.}abc", anchor_path, &n_matches, &starts, &ends, &lines, 0.5);
  if (n_matches != 0) {
    printf("ERROR: Bad start-anchored no-match returned by fmatcha.\n");
    return(48);
  }

  fmatcha("abc{.}", anchor_path, &n_matches, &starts, &ends, &lines, 0.5);
  remove(anchor_path);
  if ((n_matches != 1) || (starts[0] != 2) || (ends[0] != 5)) {
    printf("ERROR: Bad end-anchored matches returned by fmatcha.\n");
    return(49);
  }
  free(starts);

  char * ascii_ratio_path = "/tmp/regex_ascii_ratio_test.bin";
  FILE * ascii_ratio_file = fopen(ascii_ratio_path, "wb");
  if (ascii_ratio_file == NULL) {
    printf("ERROR: Failed to create temporary ASCII ratio test file.\n");
    return(24);
  }
  for (int index = 0; index < 49; index++) fputc('a', ascii_ratio_file);
  for (int index = 0; index < 52; index++) fputc('\0', ascii_ratio_file);
  fclose(ascii_ratio_file);

  starts = NULL;
  ends = NULL;
  lines = NULL;
  fmatcha(".*a", ascii_ratio_path, &n_matches, &starts, &ends, &lines, 0.5);
  remove(ascii_ratio_path);
  if (n_matches != -3) {
    printf("ERROR: Bad ASCII ratio handling returned by fmatcha.\n");
    return(25);
  }

  starts = (int*) 1;
  ends = (int*) 1;
  lines = (int*) 1;
  fmatcha("a", "/tmp/regex_file_does_not_exist", &n_matches, &starts, &ends, &lines, 0.5);
  if ((n_matches != -2) || (starts != NULL) || (ends != NULL) || (lines != NULL)) {
    printf("ERROR: Bad file-open failure handling returned by fmatcha.\n");
    return(26);
  }

  char * high_byte_path = "/tmp/regex_high_byte_test.bin";
  FILE * high_byte_file = fopen(high_byte_path, "wb");
  if (high_byte_file == NULL) {
    printf("ERROR: Failed to create temporary high-byte test file.\n");
    return(27);
  }
  fputc(0xE9, high_byte_file);
  fclose(high_byte_file);
  char high_byte_regex[] = {(char) 0xE9, '\0'};
  fmatcha(high_byte_regex, high_byte_path, &n_matches, &starts, &ends, &lines, 0.0);
  remove(high_byte_path);
  if ((n_matches != 1) || (starts[0] != 0) || (ends[0] != 1)) {
    printf("ERROR: Bad high-byte handling returned by fmatcha.\n");
    return(28);
  }
  free(starts);

  char * line_path = "/tmp/regex_line_test.txt";
  FILE * line_file = fopen(line_path, "wb");
  if (line_file == NULL) {
    printf("ERROR: Failed to create temporary line test file.\n");
    return(29);
  }
  fputs("a\nb", line_file);
  fclose(line_file);
  fmatcha("\n", line_path, &n_matches, &starts, &ends, &lines, 0.5);
  if ((n_matches != 1) || (starts[0] != 1) || (ends[0] != 2) || (lines[0] != 1)) {
    printf("ERROR: Bad newline line handling returned by fmatcha.\n");
    return(30);
  }
  free(starts);
  fmatcha("a\nb", line_path, &n_matches, &starts, &ends, &lines, 0.5);
  remove(line_path);
  if ((n_matches != 1) || (starts[0] != 0) || (ends[0] != 3) || (lines[0] != 2)) {
    printf("ERROR: Bad multiline end-line handling returned by fmatcha.\n");
    return(31);
  }
  free(starts);
  
  printf("\n All tests PASSED.\n");
  // Successful return.
  return(0);
}

#endif
