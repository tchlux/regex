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
      " abc", -1, 0 },

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
      " does it ever end", 0, 18 },

    { "[|]",    1, 1,
      "|",      (int[]){1}, (int[]){-1}, (char[]){2},
      "| test", 0, 1 },

    { "{[.]}*{.}", 3, 3,
      "*..",       (int[]){1,-1,-1}, (int[]){2,0,3}, (char[]){0,2,0},
      "anything",  0, 9 },

    { "[a]*{[a]}", 3, 3,
      "*aa",      (int[]){1,0,-1}, (int[]){2,-1,3}, (char[]){0,2,2},
      "abba",     0, 2 },

    { "[*]*{[*]}", 3, 3,
      "****",      (int[]){1,0,-1}, (int[]){2,-1,3}, (char[]){0,2,2},
      "*paths*",   0, 2 },

    { "[*]*{[*]}{[*]}", 4, 5,
      "****",           (int[]){1,0,-1,-1}, (int[]){2,-1,3,4}, (char[]){0,2,2,2},
      "*paths*",        0, 3 },

    // Last test regular expression must be empty!

    { "", 0, 0,
      "", NULL, NULL, NULL,
      "", -1, STRING_EMPTY_ERROR  }

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
  
  printf("\n All tests PASSED.\n");
  // Successful return.
  return(0);
}

#endif
