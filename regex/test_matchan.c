// Bounded aggregate matcher: golden results, differential slices, and OOM paths.
// From repository root: cc -std=c11 -Wall -Wextra -pedantic regex/test_matchan.c -o /tmp/test_matchan && /tmp/test_matchan
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Count all core-library allocations and fail each allocation point in turn.
static long fail_at = -1, allocations = 0, live = 0;
static void * test_malloc(size_t size) {
  long call = allocations++;
  if (call == fail_at) return NULL;
  void *p = malloc(size);
  if (p) live++;
  return p;
}
static void * test_realloc(void *p, size_t size) {
  long call = allocations++;
  if (call == fail_at) return NULL;
  int was_null = p == NULL;
  void *q = realloc(p, size);
  if (q && was_null) live++;
  return q;
}
static void test_free(void *p) {
  if (p) { assert(live > 0); live--; }
  free(p);
}
#define malloc test_malloc
#define free test_free
#define realloc test_realloc
#include "regex.c"
#undef malloc
#undef free
#undef realloc

static void expect(const char *pattern, const char *text, size_t len,
                   int cap, int count, const int *a, const int *z) {
  int n = cap, *starts = NULL, *ends = NULL;
  matchan(pattern, text, len, &n, &starts, &ends);
  assert(n == count);
  for (int i = 0; i < count; i++) {
    assert(starts[i] == a[i]);
    assert(ends[i] == z[i]);
  }
  test_free(starts);
  assert(live == 0);
}

// Compare with a terminated copy, not with an assumed regex dialect.
// Allocate precisely len bytes so ASan also checks reads at the bound.
static void compare(const char *pattern, const char *text, size_t len, int cap) {
  char *slice = malloc(len ? len : 1), *copy = malloc(len + 1);
  assert(slice && copy);
  memcpy(slice, text, len);
  memcpy(copy, text, len);
  copy[len] = '\0';
  int n = cap, m = cap, *a = NULL, *z = NULL, *b = NULL, *y = NULL;
  matchan(pattern, slice, len, &n, &a, &z);
  matcha(pattern, copy, &m, &b, &y);
  assert(n == m);
  assert((a == NULL) == (b == NULL));
  assert((z == NULL) == (y == NULL));
  int count = n == -1 ? 1 : n;
  for (int i = 0; i < count; i++) assert(a[i] == b[i] && z[i] == y[i]);
  test_free(a); test_free(b);
  free(slice); free(copy);
  assert(live == 0);
}

static void allocation_failures(const char *pattern, const char *text) {
  int n = 0, *a = NULL, *z = NULL;
  allocations = 0;
  matchan(pattern, text, strlen(text), &n, &a, &z);
  assert(n >= 0 || n == -1);
  long total = allocations;
  test_free(a);
  assert(live == 0 && total > 0);
  for (long i = 0; i < total; i++) {
    fail_at = i;
    allocations = 0;
    n = 0;
    a = z = (int *) (uintptr_t) 1;
    matchan(pattern, text, strlen(text), &n, &a, &z);
    assert(n == REGEX_MEMORY_ERROR);
    assert(a == NULL && z == NULL && live == 0);
  }
  fail_at = -1;
}

int main(void) {
  expect("b", "abc", 1, 0, 0, NULL, NULL);
  expect("b{.}", "abc", 2, 0, 1, (int[]){1}, (int[]){2});
  expect("{.}a", "abc", 1, 0, 1, (int[]){0}, (int[]){1});
  expect("{.}b", "abc", 2, 0, 0, NULL, NULL);
  expect("aa", "aaaa!", 4, 0, 3, (int[]){0,1,2}, (int[]){2,3,4});
  expect("x.*a", "xaaa!", 4, 0, 1, (int[]){0}, (int[]){4});
  expect("a", "aaaa!", 4, 2, 2, (int[]){0,1}, (int[]){1,2});
  expect("a?", NULL, 0, 0, 1, (int[]){0}, (int[]){0});
  expect("a", NULL, 0, 0, 0, NULL, NULL);
  // Preserve existing terminating-NUL negation behavior, including offsets.
  expect("{a}", NULL, 0, 0, 1, (int[]){0}, (int[]){1});
  expect("b", "a\0b", 3, 0, 0, NULL, NULL);
  const char high[] = {(char)0xe9, 'b'};
  const char high_pattern[] = {(char)0xe9, 0};
  expect(high_pattern, high, 1, 0, 1, (int[]){0}, (int[]){1});

  const char *patterns[] = {
    "a", "b", ".", ".*", "aa", "a?", "a*", "a|b", "(ab)|(ba)",
    "[ab]", "[*]", "[*a]", "{a}", "{[ab]}", "a{.}", "{.}a", "{.}a{.}",
    "a.*b", "(ab)*", "[ab]*", "a(b|a)?", "{.}.*", "{.}",
    "", "[", "()", "a|", "*", "{.}[", "{.}()"
  };
  const int caps[] = {-1, 0, 1, 2, 1024};
  // All short texts over an alphabet including embedded NUL and a high byte.
  const char alphabet[] = {'a', 'b', '*', '\0', (char)0xe9};
  size_t comparisons = 0;
  for (size_t len = 0, combinations = 1; len <= 4; len++, combinations *= 5) {
    for (size_t v = 0; v < combinations; v++) {
      char text[4];
      size_t digits = v;
      for (size_t j = 0; j < len; j++) { text[j] = alphabet[digits % 5]; digits /= 5; }
      for (size_t p = 0; p < sizeof(patterns)/sizeof(patterns[0]); p++)
        for (size_t c = 0; c < sizeof(caps)/sizeof(caps[0]); c++) {
          compare(patterns[p], text, len, caps[c]);
          comparisons++;
        }
    }
  }
  // Exercise the default 1024 cap, explicit larger caps and array growth.
  char long_text[1100];
  memset(long_text, 'a', sizeof(long_text));
  compare("a", long_text, sizeof(long_text), 0);
  compare("a", long_text, sizeof(long_text), 1100);
  int n = 0, *a = NULL, *z = NULL;
  matchan("a", long_text, sizeof(long_text), &n, &a, &z);
  assert(n == 1024);
  test_free(a);
  n = 1100;
  matchan("a", long_text, sizeof(long_text), &n, &a, &z);
  assert(n == 1100 && a[1099] == 1099 && z[1099] == 1100);
  test_free(a);

  // Reject unrepresentable lengths before touching the input or allocating.
  size_t bad_lengths[] = {(size_t)INT_MAX, (size_t)INT_MAX + 1, (size_t)-1};
  for (size_t i = 0; i < sizeof(bad_lengths)/sizeof(bad_lengths[0]); i++) {
    n = 1; a = z = (int *)(uintptr_t)1; allocations = 0;
    matchan("a", NULL, bad_lengths[i], &n, &a, &z);
    assert(n == REGEX_LENGTH_ERROR && a == NULL && z == NULL && allocations == 0);
  }
  // Maximum accepted bound need not be scanned beyond an actual early NUL.
  n = 0;
  matchan("a", "", (size_t)INT_MAX - 1, &n, &a, &z);
  assert(n == 0);
  test_free(a);
  allocation_failures("a", "aaaaaaaaaaaaaaaaaaaaa");
  allocation_failures("(a|b)*c", "aaabc");
  allocation_failures("a", "bbb");
  allocation_failures("[", "abc");
  allocation_failures("", "abc");
  assert(live == 0);
  printf("matchan tests passed (%zu differential cases, golden and allocation checks).\n", comparisons);
  return 0;
}
