#include "../regex/regex.c"

void regex_count(const char * regex, int * tokens, int * groups) {
  _count(regex, tokens, groups);
}

void regex_set_jump(const char * regex, const int n_tokens, int n_groups,
                    char * tokens, int * jumps, int * jumpf, char * jumpi) {
  _set_jump(regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
}

