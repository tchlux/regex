// Regression tests for unanchored literal '*' sets in all search APIs.
// From repository root: cc -std=c11 regex/test_literal_sets.c -o /tmp/test_sets && /tmp/test_sets
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "regex.c"

int main(void) {
  struct { const char *pattern; const char *text; int start, end; } cases[] = {
    {"[*]", "xx*xx", 2, 3},
    {"[*]x", "zz*xzz", 2, 4},
    {"[*a]", "zza", 2, 3},
    {"[*a]", "zz*", 2, 3},
    {"([*])", "zz*", 2, 3},
    {"[*]", "*", 0, 1},
    {"[*]", "abc", -1, 0},
    {"[*]", "", -1, 0},
    {"{.}[*]", "zz*", -1, 0},
    {"{.}[*]", "*zz", 0, 1},
    {"[*]{.}", "zz*", 2, 3},
    {"[*]{.}", "*zz", -1, 0}
  };
  char path[] = "/tmp/regex-literal-sets-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  close(fd);
  for (size_t k = 0; k < sizeof(cases)/sizeof(cases[0]); k++) {
    int a, z, n = 0, *starts = NULL, *ends = NULL, *lines = NULL;
    match(cases[k].pattern, cases[k].text, &a, &z);
    assert(a == cases[k].start && z == cases[k].end);
    matcha(cases[k].pattern, cases[k].text, &n, &starts, &ends);
    assert(n == (a >= 0));
    if (n) assert(starts[0] == a && ends[0] == z);
    free(starts);
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    size_t len = strlen(cases[k].text);
    assert(fwrite(cases[k].text, 1, len, f) == len);
    assert(fclose(f) == 0);
    n = 0;
    fmatcha(cases[k].pattern, path, &n, &starts, &ends, &lines, 0.0);
    assert(n == (a >= 0));
    if (n) assert(starts[0] == a && ends[0] == z && lines[0] == 1);
    free(starts);
  }
  // Search must restart after each match, not just after leading text.
  int n = 0, *starts = NULL, *ends = NULL;
  matcha("[*]", "x*x*", &n, &starts, &ends);
  assert(n == 2 && starts[0] == 1 && ends[0] == 2 && starts[1] == 3 && ends[1] == 4);
  free(starts);
  FILE *f = fopen(path, "wb");
  assert(f != NULL && fputs("x*x*", f) >= 0 && fclose(f) == 0);
  int *lines = NULL;
  n = 0;
  fmatcha("[*]", path, &n, &starts, &ends, &lines, 0.0);
  assert(n == 2 && starts[0] == 1 && ends[0] == 2 && starts[1] == 3 && ends[1] == 4);
  assert(lines[0] == 1 && lines[1] == 1);
  free(starts);
  assert(unlink(path) == 0);
  puts("Literal-set regression tests passed.");
  return 0;
}
