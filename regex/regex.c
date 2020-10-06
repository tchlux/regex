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
// 
//
// DEVELOPMENT:
// 
//  Write `matchl` that returns the *longest* match discovered.
//   Could this be done by replacing all "t*" with "t*{t}"?
// 
//  Support '**' syntax to make the '*' greedy.
//   Will implementing a greedy search require backtracking?
// 
//  Returning allocated memory:
//   Write `matchn` that returns *nonoverlapping* matches in arrays.
//    this could be done with repeated calls to matchs, 
//    starting after the most recent match
//   Write `matcha` that returns *all* starts and ends of matches in arrays.
//    this could be approximated with repeated calls to matchs,
//    subtracting out each match from the string by shifting contents,
//    however this will not get 
//   Write `randre` that generates a random string that matches the
//    given regular expression, tile the space of matches so that
//    all the shortest matches come first, then longer ones
//    
// 
//  Uncomment the line "#define DEBUG" and recompile to enable debugging.
// 
// ___________________________________________________________________

#define DEBUG

#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free

#define EXIT_TOKEN -1
#define REGEX_NO_TOKENS_ERROR -1
#define REGEX_UNCLOSED_GROUP_ERROR -2
#define REGEX_SYNTAX_ERROR -3
#define REGEX_EMPTY_GROUP_ERROR -4
#define STRING_EMPTY_ERROR -5
#define DEFAULT_GROUP_MOD ' '

//  Name? 
//    regex -- regular expression library
//    frex  -- fast regular expressions (frexi for case insensitive)
//    rexl  -- regular expression library (rexi for case insensitive)
//    rex   -- regular expressions (rexi for case insensitive)
//    rel   -- regular expression library (reli for case insensitive)
//    fre   -- fast regular expressions
//    freg  -- fast regular expressions
//    frel  -- fast regular expression library

#ifdef DEBUG
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
  else CHAR3[0] = c;
  return (char *) CHAR3;
}
#endif

// Count the number of tokens and groups in a regular expression.
void _count(const char * regex, int * tokens, int * groups) {
  // Initialize the number of tokens and groups to 0.
  (*tokens) = 0;
  (*groups) = 0;
  int i = 0;             // regex character index
  char token = regex[i]; // current character in regex
  int gc = 0;            // groups closed
  char pt = '\0';        // previous token
  // Count tokens and groups.
  while (token != '\0') {
    // Count a character set as a single character.
    if (token == '[') {
      // Increment the count of the number of groups.
      (*groups)++;
      int tokens_in_group = 0;
      // Loop until this character set is complete.
      i++;
      pt = token;
      token = regex[i];
      while ((token != '\0') && (token != ']')) {
	// One token for this character in the group
	(*tokens)++;
	tokens_in_group++;
	// Increment to next character.
	i++;
	pt = token;
	token = regex[i];
      }
      // Check for an error in the regular expression.
      if (token == '\0') {
	(*tokens) = -i-1;
	(*groups) = REGEX_UNCLOSED_GROUP_ERROR;
	return;
      // Check for an empty group error.
      } else if (tokens_in_group == 0) {
	(*tokens) = -i-1;
	(*groups) = REGEX_EMPTY_GROUP_ERROR;
	return;
      } else {
	// This group successfully closed.
	gc++;
      }
    // If this is the beginning of another type of group, count it.
    } else if ((token == '(') || (token == '{')) {
      (*groups)++;
    // Check for invalid regular expressions
    } else if (
       // starts with a special character
       ((i == 0) && ((token == ')') || (token == ']') || (token == '}') || 
		     (token == '*') || (token == '?') || (token == '|'))) ||
       // illegally placed * or ?
       ((i > 0) && ((token == '*') || (token == '?')) &&
	((pt == '*') || (pt == '?') || (pt == '(') || (pt == '{') || (pt == '|'))) ||
       // illegally placed ), ], or } after a |
       ((i > 0) && (pt == '|') && ((token == ')') || (token == ']') || (token == '}'))) ||
       // | at the end of the regex
       ((token == '|') && (regex[i+1] == '\0'))
       ) {
      (*tokens) = -i-1;
      (*groups) = REGEX_SYNTAX_ERROR;
      return;
    // Close opened groups
    } else if ((token == ')') || (token == '}')) {
      gc++;
      // too many closed groups, or an empty group.
      if ( (gc > (*groups)) ||
	   ((token == ')') && (pt == '(')) ||
	   ((token == '}') && (pt == '{')) ) {
	(*tokens) = -i-1;
	(*groups) = REGEX_EMPTY_GROUP_ERROR;
	return;
      }
    // If the character is counted (not special), count it as one.
    } else {
      (*tokens)++;
    }
    // Increment to the next step.
    i++;
    pt = token;
    token = regex[i];
  }
  // Error if there are unclosed groups.
  if (gc != (*groups)) {
    (*tokens) = -i-1;
    (*groups) = REGEX_UNCLOSED_GROUP_ERROR;
    return;
  }
  // Return the total number of tokens and groups.
  return;
}


// Read through the regular expression with the (already counted)
// number of tokens + groups and set the jump conditions.
void _set_jump(const char * regex, const int n_tokens, int n_groups,
	       char * tokens, int * jumps, int * jumpf, char * jumpi) {

  // Initialize storage for the first and first proceding token of groups.
  int * group_starts = malloc((4*n_groups+n_tokens+2)*sizeof(int) + 2*n_groups*sizeof(char));
  int * group_nexts = group_starts + n_groups;
  int * gi_stack = group_nexts + n_groups; // active group stack
  int * gc_stack = gi_stack + n_groups; // closed group stack
  int * redirect = 1 + gc_stack + n_groups; // jump redirection (might access -1 or n_tokens)
  char * s_stack = (char*) (redirect + n_tokens + 1); // active group start character stack
  char * g_mods = s_stack + n_groups; // track the modifiers on each group

  // Initialize all the group pointers to a known value.
  for (int j = 0; j < n_groups; j++) {
    group_starts[j] = EXIT_TOKEN;
    group_nexts[j] = EXIT_TOKEN;
    gi_stack[j] = EXIT_TOKEN;
    gc_stack[j] = EXIT_TOKEN;
    s_stack[j] = DEFAULT_GROUP_MOD;
    g_mods[j] = DEFAULT_GROUP_MOD;
    redirect[j] = j;
  }
  // declare the rest of redirect (through 
  for (int j = n_groups; j <= n_tokens; j++) redirect[j] = j;
  // (declare the -1 to point to -1)
  redirect[EXIT_TOKEN] = EXIT_TOKEN;

  // =================================================================
  //                          FIRST PASS
  //  Identify the first token and first "next token" for each group
  //  as well as the modifiers placed on each group (*, ?, |).
  // 
  int i = 0;    // regex index
  int nt = 0;   // total tokens seen
  int ng = 0;   // total groups seen
  int gi = -1;  // group index
  int iga = -1; // index of active group (in stack)
  int igc = -1; // group closed index (in stack)
  char cgs = '\0' ; // character for active group start
  char token = regex[i]; // current character

  // Loop until all tokens in the regex have been processed.
  while (token != '\0') {
    // Look for the beginning of a new group.
    if (((token == '(') || (token == '[') || (token == '{')) && (cgs != '[')) {
      gi = ng;		     // set current group index
      cgs = token;	     // set current group start character
      ng++;		     // increment the number of groups seen
      // Push group index and start character into stack.
      iga++;		    // increase active group index
      gi_stack[iga] = gi;   // push group index to stack
      s_stack[iga] = token; // push group start character to stack
      group_starts[gi] = nt; // set the start token for this group
    // Set the end of a group.
    } else if ((iga >= 0) &&
  	       (((cgs == '(') && (token == ')')) ||
  		((cgs == '[') && (token == ']')) ||
  		((cgs == '{') && (token == '}')))) {
      // Close the group, place it into the closed group stack.
      igc++;
      gc_stack[igc] = gi;
      // Check to see if the next character is a modifier.
      token = regex[i+1]; // (can reuse this, it will be overwritten after later i++)
      if ((token == '*') || (token == '?') || (token == '|')) {
  	g_mods[gi] = token;
      }
      // Pop previous group index and start character from stack.
      iga--;
      if (iga >= 0) {
  	gi = gi_stack[iga];
  	cgs = s_stack[iga];
      } else {
  	gi = -1;
  	cgs = '\0';
      }
    // Handle normal tokens.
    } else {
      // if (not special)
      if ((cgs == '[') || ((token != '*') && (token != '?') && (token != '|'))) {
  	// Set the "next" token for the recently closed groups.
  	for (int j = 0; j <= igc; j++) {
  	  group_nexts[gc_stack[j]] = nt;
  	}
  	igc = -1; // now no groups are waiting to be closed
      }
      tokens[nt] = token; // store the token
      jumps[nt] = nt+1; // initialize jump on successful match to next token
      jumpf[nt] = EXIT_TOKEN; // initialize jump on failed match to exit
      jumpi[nt] = 0; // initialize immediately check next to 0 (off)
      nt++; // increment to next token
    }
    // Cycle to the next token.
    i++;
    token = regex[i];
  }

  // Set the "next" token for the closed groups at the end.
  for (int j = 0; j <= igc; j++)
    group_nexts[gc_stack[j]] = nt;
  igc = -1;
  
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // DEBUG: Print out the tokens in the order given in the regex.
  #ifdef DEBUG
  if (DO_PRINT) {
  printf("\nTokens (before prefixing modifiers)\n  ");
  for (int j = 0; j < n_tokens; j++) printf("%-2s  ", SAFE_CHAR(tokens[j]));
  printf("\n");
  if (n_groups > 0) {
    printf("\n");
    printf("Groups (before prefixing modifiers)\n");
    for (int j = 0; j < n_groups; j++) {
      printf(" %d: %c (%-2d %s)  -->", j, g_mods[j], group_starts[j],
	     SAFE_CHAR(tokens[group_starts[j]]));
      printf("  (%-2d %s) \n", group_nexts[j]-1, 
	     SAFE_CHAR(tokens[group_nexts[j]-1]));
    }
  }
  }
  #endif
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  // =================================================================
  //                        SECOND PASS
  // Shift group modifier tokens to be prefixes (at front of group).
  // Assign all tokens to their final location (specials prefixed).
  // 
  i = 0;    // regex index
  nt = 0;   // total tokens seen
  ng = 0;   // total groups seen
  gi = -1;  // group index
  iga = -1; // index of active group (in stack)
  cgs = '\0'; // current group start
  token = regex[i]; // current character
  
  int gx = 0; // current group shift (number of tokens moved to frotn)
  while (token != '\0') {
    // Look for the beginning of a new group.
    if (((token == '(') || (token == '[') || (token == '{')) &&	(cgs != '[')) {
      // -------------------------------------------------------------
      // Shift this group start by any active modifiers on containing groups.
      if (gx > 0) group_starts[ng] += gx;
      // -------------------------------------------------------------
      gi = ng;		     // set current group index
      cgs = token;	     // set current group start character
      ng++;		     // increment the number of groups seen
      // Push group index and start character into stack.
      iga++;		    // increase active group index
      gi_stack[iga] = gi;   // push group index to stack
      s_stack[iga] = token; // push group start character to stack
      // -------------------------------------------------------------
      // Push a modifier onto the stack if it exists.
      if (g_mods[gi] != DEFAULT_GROUP_MOD) {      
	gx++; // increase count of prepended modifiers
	// push starts of all affected groups back by one character
	tokens[nt] = g_mods[gi]; // store this modifier at the front
	nt++; // increment the token counter
      }
      // -------------------------------------------------------------
    // Set the end of a group.
    } else if ((iga >= 0) &&
  	       (((cgs == '(') && (token == ')')) ||
  		((cgs == '[') && (token == ']')) ||
  		((cgs == '{') && (token == '}')))) {
      // decrement the number of shifts that have occurred for prefix
      if (g_mods[gi] != DEFAULT_GROUP_MOD) {
	gx--;
	const int last_in_group = nt-1;
	// Shift all "group_nexts" that are contained in this group.
	//  WARNING: This makes the algorithmic complexity quadratic
	//           with the number of nested groups. Unavoidable?
	for (int j = gi; j < ng; j++)
	  if (group_nexts[j] < last_in_group)
	    group_nexts[j]++;
      }
      // Pop group index from the stack.
      iga--;
      if (iga >= 0) {
	gi = gi_stack[iga];
	cgs = s_stack[iga];
      } else {
	gi = -1;
	cgs = '\0';
      }
    // ---------------------------------------------------------------
    // Handle tokens.
    } else if (nt < n_tokens) {
      // If the next token is special, put it in front.
      const char nx_token = regex[i+1]; // temporary storage
      // if in token set
      if (cgs == '[') { // do nothing
      // Not in token set, handle special looping modifiers on single tokens
      } else if ((nx_token == '*') || (nx_token == '?') || (nx_token == '|')) {
	tokens[nt] = nx_token; // store this modifier at the front
	nt++; // increment the token counter.
	i++; // increment the regex index counter
	tokens[nt+1] = token; // move the original token back one
      }
      // store the token, skip specials that were already stored earlier
      if ((cgs == '[') || ((token != '*') && (token != '?') && (token != '|'))) {
	tokens[nt] = token; // store this token.
	nt++; // increment the token counter.
      }
    }
    // ---------------------------------------------------------------
    // Cycle to the next token.
    i++;
    token = regex[i];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // DEBUG: Print out the tokens with prefixed modifiers
  #ifdef DEBUG
  if (DO_PRINT) {
  printf("\nTokens: (token / token index)\n  ");
  for (int j = 0; j < n_tokens; j++) printf("%-2s  ", SAFE_CHAR(tokens[j]));
  printf("\n  ");
  for (int j = 0; j < n_tokens; j++) printf("%-2d  ", j);
  printf("\n\n");
  // DEBUG: Print out the groups (new start and end with prefixes)
  if (n_groups > 0) {
    printf("Groups:  (group: mod, start token --> last token)\n");
    for (int j = 0; j < n_groups; j++) {
      printf(" %d: %c (%-2d %s)  -->", j, g_mods[j], group_starts[j],
	     SAFE_CHAR(tokens[group_starts[j]]));
      printf("  (%-2d %s) \n", group_nexts[j]-1, 
	     SAFE_CHAR(tokens[group_nexts[j]-1]));
    }
    printf("\n");
  }
  }
  #endif
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Define an in-line function (fast) that correctly redirects any
  // token jump. This assumes that negative redirects are contained
  // within a {} group and should logically flip "success" and "fail".
  //   i  --  integer index of the token to redirect
  //   s  --  integer intended "success" destination
  //   f  --  integer intended "fail" destination
  #define SET_JUMP(i, s, f)   \
    if (neg) {                \
      jumps[i] = redirect[f]; \
      jumpf[i] = redirect[s]; \
    } else {                  \
      jumps[i] = redirect[s]; \
      jumpf[i] = redirect[f]; \
    }

  // =================================================================
  //                        THIRD PASS
  // Assign jump-to success / failed / immediate for all tokens.
  // 
  i = 0;    // regex index
  nt = 0;   // total tokens seen
  ng = 0;   // total groups seen
  gi = -1;  // group index
  iga = -1; // index of active group (in stack)
  cgs = '\0'; // current group start
  token = regex[i]; // current character
  
  char neg = 0; // whether or not current section is negated
  while (token != '\0') {
    // Look for the beginning of a new group.
    if (((token == '(') || (token == '[') || (token == '{')) &&	(cgs != '[')) {
      // -------------------------------------------------------------
      gi = ng;		     // set current group index
      cgs = token;	     // set current group start character
      ng++;		     // increment the number of groups seen
      // Push group index and start character into stack.
      iga++;		    // increase active group index
      gi_stack[iga] = gi;   // push group index to stack
      s_stack[iga] = token; // push group start character to stack
      // -------------------------------------------------------------
      // Push a modifier onto the stack if it exists.
      if (g_mods[gi] != DEFAULT_GROUP_MOD) {      
	// set jump conditions for modifier (prevent reversal from negation)
	if (neg) { SET_JUMP(nt, group_nexts[gi], nt+1); }
	else { SET_JUMP(nt, nt+1, group_nexts[gi]); }
	redirect[nt] = nt; // reset redirect to completed token
	nt++; // increment the token counter
	// assign 'redirect' based on the modifier.
	if (g_mods[gi] == '*') {
	  redirect[group_nexts[gi]] = nt-1; // -1 because of nt++ above
	} else if (g_mods[gi] == '|') {
	  // search for a group that starts at the first token after this
	  int j = gi+1;
	  while ((j < n_groups) && (group_starts[j] < group_nexts[gi])) j++;
	  // if a group immediately follows (after the |)..
	  if ((j < n_groups) && (group_starts[j] == group_nexts[gi]))
	    redirect[group_nexts[gi]] = group_nexts[j];
	  // otherwise a single token follows (after the |)..
	  else
	    redirect[group_nexts[gi]] = group_nexts[gi]+1;
	}
      }
      // Toggle "negated" for negated groups.
      if (cgs == '{') neg = (neg + 1) % 2;
      // -------------------------------------------------------------
    // Set the end of a group.
    } else if ((iga >= 0) &&
  	       (((cgs == '(') && (token == ')')) ||
  		((cgs == '[') && (token == ']')) ||
  		((cgs == '{') && (token == '}')))) {
      // Toggle "negated" for negated groups.
      if (token == '}') neg = (neg + 1) % 2;
      // Pop group index from the stack.
      iga--;
      if (iga >= 0) {
	gi = gi_stack[iga];
	cgs = s_stack[iga];
      } else {
	gi = -1;
	cgs = '\0';
      }
    // ---------------------------------------------------------------
    // Handle tokens.
    } else if (nt < n_tokens) {
      // If the next token is special, put it in front.
      const char nx_token = regex[i+1]; // temporary storage
      // if in token set
      if (cgs == '[') {
	jumpi[nt] = 1; // set an immediate jump on failure
	// if this is the last token in the token set..
	if (nx_token == ']') {
	  jumpi[nt] = 2; // set this as an "end of token set" element
	  SET_JUMP(nt, group_nexts[gi], EXIT_TOKEN);
	// otherwise this is not the last token in the token set..
	} else {
	  // this group is negated, so exit on "success" (compensate for flip)
	  if (neg) {
	    SET_JUMP(nt, nt+1, EXIT_TOKEN);
	  // this is not negated nor last, success exist, failure steps to next
	  } else {
	    SET_JUMP(nt, group_nexts[gi], nt+1);
	  }
	}
      // Not in token set, handle special looping modifiers on single tokens
      } else if ((nx_token == '*') || (nx_token == '?') || (nx_token == '|')) {
	SET_JUMP(nt, nt+(neg?2:1), nt+(neg?1:2)); // set jump conditions for special token
	redirect[nt] = nt; // reset redirect to completed token
	nt++; // increment the token counter.
	i++; // increment the regex index counter
	// make tokens followed by * loop back to *
	if (nx_token == '*') {
	  SET_JUMP(nt, nt-1, EXIT_TOKEN);
	// make tokens followed by | correctly jump
	} else if (nx_token == '|'){  
	  const char nxnx_token = regex[i+1];
	  if ((nxnx_token == '(') || (nxnx_token == '[') || (nxnx_token == '{')) {
	    SET_JUMP(nt, group_nexts[ng+1], EXIT_TOKEN); // last token jumps after next group
	  } else {
	    SET_JUMP(nt, nt+2, EXIT_TOKEN); // last token jumps after next token
	  }
	// make tokens followed by ? correctly jump
	} else if (nx_token == '?') {
	  SET_JUMP(nt, nt+1, EXIT_TOKEN);
	}
      // if this is a standard token..
      } else {
	SET_JUMP(nt, nt+1, EXIT_TOKEN);
      }
      redirect[nt] = nt; // reset redirect to completed token
      // store the token, skip specials that were already stored earlier
      if ((cgs == '[') || ((token != '*') && (token != '?') && (token != '|'))) {
	nt++; // increment the token counter.
      }
    }
    // ---------------------------------------------------------------
    // Cycle to the next token.
    i++;
    token = regex[i];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // DEBUG: Print out the group starts and ends.
  #ifdef DEBUG
  if (DO_PRINT) {
  printf("Jumps/f/i:  (token: jump on match, jump on failed match, jump immediately on fail)\n");
  for (int i = 0; i < n_tokens; i++) {
    printf(" (%-2d%2s):  % -3d % -3d  %1d\n", i, SAFE_CHAR(tokens[i]), jumps[i], jumpf[i], jumpi[i]);
  }
  printf("\n");
  }
  #endif
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Free memory used for tracking the start and ends of groups.
  free(group_starts);
  return;
}

// Do a simple regular experession match.
void match(const char * regex, const char * string, int * start, int * end) {
  // Check for an empety string.
  if (string[0] == '\0') {
    (*start) = EXIT_TOKEN;
    (*end) = STRING_EMPTY_ERROR;
    return;
  }
  // Count the number of tokens and groups in this regular expression.
  int n_tokens, n_groups;
  _count(regex, &n_tokens, &n_groups);
  // Error mode, fewer than one token (no possible match).
  if (n_tokens <= 0) {
    // Set the error flag and return.
    if (n_tokens == 0) {
      (*start) = EXIT_TOKEN;
      (*end) = REGEX_NO_TOKENS_ERROR;
    } else {
      (*start) = n_tokens;
      (*end) = n_groups;
    }
    return;
  }
  // Initialize storage for tracking the current active tokens and
  // where to jump based on the string being parsed.
  const int mem_bytes = ((5*n_tokens+1)*sizeof(int) + (4*n_tokens+2)*sizeof(char));
  int * jumps = malloc(mem_bytes); // jump-to location after success
  int * jumpf = jumps + n_tokens; // jump-to location after failure
  int * active = jumpf + n_tokens; // presently active tokens in regex
  int * cstack = active + n_tokens + 1; // current stack of active tokens
  int * nstack = cstack + n_tokens; // next stack of active tokens
  char * tokens = (char*) (nstack + n_tokens); // regex index of each token (character)
  char * jumpi = tokens + n_tokens + 1; // immediately check next on failure
  char * incs = jumpi + n_tokens; // token flags for "in current stack"
  char * inns = incs + n_tokens; // token flags for "in next stack"
  // Terminate the two character arrays with the null character.
  tokens[n_tokens] = '\0';
  jumpi[n_tokens] = '\0';

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // DEBUG: Show regex and number of tokens computed.
  #ifdef DEBUG
  if (DO_PRINT) {
  printf("\nRegex: '");
  for (int j = 0; regex[j] != '\0'; j++) {
    printf("%s", SAFE_CHAR(regex[j]));
  }
  printf("'\n tokens: %d\n groups: %d\n", n_tokens, n_groups);
  }
  #endif
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Determine the jump-to tokens upon successful match and failed
  // match at each token in the regular expression.
  _set_jump(regex, n_tokens, n_groups, tokens, jumps, jumpf, jumpi);

  // Initialize to "no match".
  (*start) = EXIT_TOKEN;
  (*end) = 0;

  // Set all tokens to be inactive, convert ? to * for simplicity.
  active[n_tokens] = EXIT_TOKEN;
  for (int j = 0; j < n_tokens; j++) {
    // convert both "special tokens" to * for speed, exclude all
    // tokens with jumpi = 1 because those are inside token sets
    if ((jumpi[j] != 1) && ((tokens[j] == '?') || (tokens[j] == '|')))
      tokens[j] = '*';
    active[j] = EXIT_TOKEN; // token is inactive
    incs[j] = 0; // token is not in current stack
    inns[j] = 0; // token is not in next stack
  }
  // Set the current index in the string.
  int i = 0; // current index in string
  char c = string[i]; // current character in string
  int ics = 0; // index in current stack
  int ins = -1; // index in next stack
  int dest; // index of next token (for jump)
  void * temp; // temporary pointer (used for transferring nstack to cstack)
  cstack[ics] = 0; // set the first element in stack to '0'
  active[0] = 0; // set the start index of the first token
  incs[0] = 1; // the first token is in the current stack

  // Define an in-line substitution that will be used repeatedly in
  // a following while loop.
  // 
  // If the destination is valid, and the current start index (val)
  // is newer than the one to be overwritten, then stack the
  // new destination, assign active, and mark as set.
  // 
  // If the destination is the "done" token, then:
  //   free all memory that was allocated,
  //   set the start and end of the match
  //   return
  #define STACK_NEXT_TOKEN(stack, si, in_stack)\
    if ((dest >= 0) && (val >= active[dest])) {\
      if (in_stack[dest] == 0) {\
	si++;\
	stack[si] = dest;\
      }\
      if (dest == n_tokens) {\
        free(jumps);\
        (*start) = val;\
        (*end) = i;\
        if ((jumpi[j]) || (ct != '*')) (*end)++;\
        return;\
      } else {\
        in_stack[dest] = 1;\
        active[dest] = val;\
      }\
    }                                

  // Start searching for a regular expression match. (the character
  // 'c' is checked for null value at the end of the loop.
  do {
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #ifdef DEBUG
    if (DO_PRINT) {
    printf("--------------------------------------------------\n");
    printf("i = %d   c = '%s'\n\n", i, SAFE_CHAR(c));
    printf("stack:\n");
    for (int j = ics;  j >= 0; j--) {
      printf(" '%s' (at %2d) %d\n", SAFE_CHAR(tokens[cstack[j]]), cstack[j], active[cstack[j]]);
    }
    printf("\n");
    printf("active: (search token / index of match start)\n");
    for (int j = 0; j <= n_tokens; j++) {
      printf("  %-3s", SAFE_CHAR(tokens[j]));
    }
    printf("\n");
    for (int j = 0; j <= n_tokens; j++) {
      printf("  %-3d", active[j]);
    }
    printf("\n\n");
    }
    #endif
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Continue popping active elements from the current stack and
    // checking them for a match and jump conditions, add next tokens
    // to the next stack.
    while (ics >= 0) {
      // Pop next token to check from the stack, skip if already done.
      const int j = cstack[ics];
      ics--;
      incs[j] = 0;
      // Get the token and the "start index" for the match that led here.
      const char ct = tokens[j];
      int val = active[j];
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      #ifdef DEBUG
      if (DO_PRINT) {
      printf("    j = %d   ct = '%s'  %2d %2d \n", j, SAFE_CHAR(ct), jumps[j], jumpf[j]);
      }
      #endif
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // If this is a special character, add its tokens immediately to
      // the current stack (to be checked before next charactrer).
      if ((ct == '*') && (! jumpi[j])) {
        if (j == 0) val = i; // ignore leading tokens where possible
	dest = jumps[j];
	STACK_NEXT_TOKEN(cstack, ics, incs);
	dest = jumpf[j];
	STACK_NEXT_TOKEN(cstack, ics, incs);
      // Check to see if this token matches the current character.
      } else if ((c == ct) || ((ct == '.') && (! jumpi[j]) && (c != '\0'))) {
	dest = jumps[j];
	STACK_NEXT_TOKEN(nstack, ins, inns);
      // This token did not match, trigger a jump fail.
      } else {
	dest = jumpf[j];
	// jump immediately on fail if this is not the last token in a token set
	if (jumpi[j] == 1) { 
	  STACK_NEXT_TOKEN(cstack, ics, incs);
	// otherwise, put into the "next" stack
	} else { 
	  STACK_NEXT_TOKEN(nstack, ins, inns);
	}
      }
    }
    // Switch out the current stack with the next stack.
    //   switch stack of token indices
    temp = (void*) cstack; // store "current stack"
    cstack = nstack; // set "current stack"
    ics = ins; // set "index in current stack"
    nstack = (int*) temp; // set "next stack"
    //   switch flag arrays of "token in stack"
    temp = (void*) incs; // store "in current stack"
    incs = inns; // set "in current stack"
    inns = (char*) temp; // set "in next stack"
    ins = -1; // reset the count of elements in "next stack"

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #ifdef DEBUG
    if (DO_PRINT) {
    printf("\n");
    }
    #endif
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // If the just-parsed character was the end of the string, then break.
    if (c == '\0') {
      break;
    // Get the next character in the string (assuming it's null terminated).
    } else {
      i++;
      c = string[i];
    }
  } while (ics >= 0) ; // loop until the active stack is empty
  free(jumps); // free all memory that was allocated
  return;
} 


// If DEBUG is not define, make the main of this program be a command line interface.
#ifndef DEBUG
int main(int argc, char * argv[]) {
  printf("\n");
  printf("The main program has not been implemented yet.\n");
  printf("It should behave somewhat like `grep` once done.\n");
  printf("\n");
  printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d] = \"%s\"\n", i, argv[i]);
  }
  printf("\n");
}
#endif

#ifdef DEBUG
int run_tests(); // <- actually declared later
// For testing purposes.
int main(int argc, char * argv[]) {
  // singleton manual test.. (use "if (1)" to run, "if (0)" to skip)
  if (0) {
    // char * regex = "((\r\n)|\r|\n)";
    // char * string = "\r\n**** \n";
    char * regex = "[[][[]";
    char * string = "[[match]]";
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
  }
  return(run_tests());
}

int run_tests() {
  // test data array
  char * regexes[] = {
    // Invalid regular expressions.
    "*abc",
    "?abc",
    "|abc",
    ")abc",
    "}abc",
    "]abc",
    "abc|",
    "abc|*",
    "abc|?",
    "abc|)",
    "abc|]",
    "abc|}",
    "abc**",
    "abc*?",
    "abc?*",
    "abc??",
    "abc(*",
    "abc(?",
    "abc{*",
    "abc{?",
    "abc(",
    "abc{",
    "abc()",
    "abc{}",
    "abc[]",
    // Valid regular expressions.
    ".",
    ".*",
    "..",
    " (.|.)*d",
    ".* .*ad",
    "abc",
    ".*abc",
    ".((a*)|(b*))*.",
    "(abc)",
    "[abc]",
    "{abc}",
    "{[abc]}",
    "{{[abc]}}",
    "[ab][ab]",
    "{[ab][ab]}",
    "a*bc",
    "(ab)*c",
    "[ab]*c",
    "{ab}*c",
    "[a][b]*{[c]}",
    "{{a}[bcd]}",
    "a{[bcd]}e",
    "{{a}[bcd]{e}}",
    "(a(bc)?)*(d)",
    "(a(bc*)?)|d",
    "{a(bc*)?}|d",
    "{(a(bc*)?)}|d",
    "(a(bc)?)|(de)",
    "(a(z.)*)[bc]*d*",
    "(a(z.)*)[bc]*d*{e}f?g",
    "(a(z.)*)[bc]*d*{e}f?g|h",
    "({({ab}c?)*d}|(e(fg)?))",
    "({({[ab]}c?)*d}|(e(fg)?))",
    "({(a)({[bc]}d?e)*(f)}|g(hi)?)",
    "[*][*]*{[*]}",
    "[[][[]",
    ".*end{.}",
    // Last test regular expression must be empty!
    ""
  };

  // test data array
  int true_n_tokens[] = {
    // Invalid regular expressions.
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -4,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    // Valid regular expressions    
    1,
    2,
    2,
    6,
    7,
    3,
    5,
    8,
    3,
    3,
    3,
    3,
    3,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    5,
    5,
    6,
    7,
    7,
    7,
    7,
    9,
    13,
    15,
    11,
    11,
    13,
    4,
    2,
    6,
    // Last test regular expression must be empty!
    0
  };

  // test data array
  int true_n_groups[] = {
    // Invalid regular expressions.
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_UNCLOSED_GROUP_ERROR,
    REGEX_UNCLOSED_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    // Valid regular expressions    
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    3,
    1,
    1,
    1,
    2,
    3,
    2,
    3,
    0,
    1,
    1,
    1,
    4,
    3,
    2,
    4,
    3,
    2,
    2,
    3,
    3,
    3,
    4,
    4,
    6,
    7,
    8,
    4,
    2,
    1,
    // Last test regular expression must be empty!
    0
  };

  // test data array
  char * true_tokens[] = {
    // Invalid regular expressions.
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    // Valid regular expressions.
    ".",
    "*.",
    "..",
    " *|..d",
    "*. *.ad",
    "abc",
    "*.abc",
    ".*|*a*b.",
    "abc",
    "abc",
    "abc",
    "abc",
    "abc",
    "abab",
    "abab",
    "*abc",
    "*abc",
    "*abc",
    "*abc",
    "a*bc",
    "abcd",
    "abcde",
    "abcde",
    "*a?bcd",
    "|a?b*cd",
    "|a?b*cd",
    "|a?b*cd",
    "|a?bcde",
    "a*z.*bc*d",
    "a*z.*bc*de?fg",
    "a*z.*bc*de?f|gh",
    "|*ab?cde?fg",
    "|*ab?cde?fg",
    "|a*bc?defg?hi",
    "****",
    "[[",
    "*.end.",
    // Last test regular expression must be empty!
    ""
  };

  // test data array
  int true_jumps[] = {
    // Invalid regular expressions.
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // Valid regular expressions.
    1,
    1,0,
    1,2,
    1,2,3,1,1,6,
    1,0,3,4,3,6,7,
    1,2,3,
    1,0,3,4,5,
    1,2,3,4,3,6,5,8,
    1,2,3,
    3,3,3,
    -1,-1,-1,
    -1,-1,-1,
    3,3,3,
    2,2,4,4,
    -1,-1,-1,-1,
    1,0,3,4,
    1,2,0,4,
    1,0,0,4,
    1,-1,-1,4,
    1,2,1,-1,
    1,-1,-1,-1,
    1,-1,-1,-1,5,
    1,-1,-1,-1,5,
    1,2,3,4,0,6,
    1,2,3,4,5,4,7,
    1,-1,3,-1,5,-1,7,
    1,-1,3,-1,5,-1,7,
    1,2,3,4,7,6,7,
    1,2,3,1,5,4,4,8,7,
    1,2,3,1,5,4,4,8,7,-1,11,12,13,
    1,2,3,1,5,4,4,8,7,-1,11,12,13,15,15,
    1,2,3,4,5,-1,-1,8,9,10,11,
    1,2,4,4,5,-1,-1,8,9,10,11,
    1,-1,3,5,5,6,-1,-1,-1,10,11,12,13,
    1,2,1,-1,
    1,2,
    1,0,3,4,5,-1,
    // Last test regular expression must be empty!
    // {}
  };

  // test data array
  int true_jumpf[] = {
    // Invalid regular expressions.
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // Valid regular expressions.
    -1,
    2,-1,
    -1,-1,
    -1,5,4,-1,-1,-1,
    2,-1,-1,5,-1,-1,-1,
    -1,-1,-1,
    2,-1,-1,-1,-1,
    -1,7,5,7,-1,1,-1,-1,
    -1,-1,-1,
    1,2,-1,
    1,2,3,
    1,2,3,
    1,2,-1,
    1,-1,3,-1,
    1,2,3,4,
    2,-1,-1,-1,
    3,-1,-1,-1,
    3,2,-1,-1,
    3,2,0,-1,
    -1,3,-1,4,
    -1,2,3,4,
    -1,2,3,4,-1,
    -1,2,3,4,-1,
    5,-1,0,-1,-1,-1,
    6,-1,7,-1,7,-1,-1,
    6,2,7,4,7,4,-1,
    6,2,7,4,7,4,-1,
    5,-1,7,-1,-1,-1,-1,
    -1,4,-1,-1,7,6,-1,9,-1,
    -1,4,-1,-1,7,6,-1,9,-1,10,12,-1,-1,
    -1,4,-1,-1,7,6,-1,9,-1,10,12,-1,14,-1,-1,
    7,6,-1,-1,1,1,11,-1,11,-1,-1,
    7,6,3,-1,1,1,11,-1,11,-1,-1,
    9,2,8,4,-1,7,7,2,10,-1,13,-1,-1,
    -1,3,-1,4,
    -1,-1,
    2,-1,-1,-1,-1,6,
    // Last test regular expression must be empty!
    // {}
  };

  char true_jumpi[] = {
    // Invalid regular expressions.
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // Valid regular expressions.
    0,
    0,0,
    0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,
    1,1,2,
    0,0,0,
    1,1,2,
    1,1,2,
    1,2,1,2,
    1,2,1,2,
    0,0,0,0,
    0,0,0,0,
    0,1,2,0,
    0,0,0,0,
    2,0,2,2,
    0,1,1,2,
    0,1,1,2,0,
    0,1,1,2,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,1,2,0,0,
    0,0,0,0,0,1,2,0,0,0,0,0,0,
    0,0,0,0,0,1,2,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,2,0,0,0,0,0,0,0,
    0,0,0,1,2,0,0,0,0,0,0,0,0,
    2,0,2,2,
    2,2,
    0,0,0,0,0,0,
    // Last test regular expression must be empty!
    //
  };

  // test data array
  char * strings[] = {
    // Invalid regular expressions.
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    // Valid regular expressions.
    " abc",
    ".*",
    "..",
    " (.|.)*d",
    ".* .*ad",
    " abc",
    "      abc",
    " aabbb ",
    "abc",
    "c",
    "ddd",
    "d",
    "c",
    "ba",
    "cd",
    "aabc",
    "ababc",
    "baabc",
    "zzdc",
    "ad",
    "azw",
    "afe",
    "age",
    "abcabcd",
    "d",
    "zdb",
    "d",
    "abc",
    "az.bcd",
    "aztzsbcdfg",
    "aztzsbcdh",
    "abdabc",
    "efg",
    "gf",
    "*** test",
    "[[ test",
    " does it ever end",
    // Last test regular expression must be empty!
    ""
  };

  // test data array
  int match_starts[] = {
    // Invalid regular expressions.
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -4,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    -5,
    // Valid regular expressions.
    0,
    0,
    0,
    0,
    2,
    -1,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    4,
    4,
    -1,
    0,
    0,
    0,
    0,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    14,
    //
    -1
  };

  // test data array
  int match_ends[] = {
    // Invalid regular expressions.
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_SYNTAX_ERROR,
    REGEX_UNCLOSED_GROUP_ERROR,
    REGEX_UNCLOSED_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    REGEX_EMPTY_GROUP_ERROR,
    // Valid regular expressions.
    1,
    0,
    2,
    8,
    7,
    0,
    9,
    2,
    3,
    1,
    3,
    1,
    1,
    2,
    2,
    4,
    5,
    5,
    0,
    2,
    2,
    3,
    3,
    7,
    1,
    1,
    1,
    1,
    1,
    10,
    9,
    1,
    1,
    1,
    4,
    2,
    18,
    //
    STRING_EMPTY_ERROR
  };


  int done = 0;
  int i = -1;  // test index
  int ji = -1; // index in jumps / jumpf / jumpi
  while (done == 0) {
    i++; // increment test index counter

    // ===============================================================
    //                          _count     
    // 
    // Count the number of tokens and groups in this regular expression.
    int n_tokens, n_groups;
    _count(regexes[i], &n_tokens, &n_groups);

    // Verify the number of tokens and the number of groups..
    if (n_tokens != true_n_tokens[i]) {
      printf("\nRegex: '");
      for (int j = 0; regexes[i][j] != '\0'; j++) {
	printf("%s", SAFE_CHAR(regexes[i][j]));
      }
      printf("'\n\n");
      printf("ERROR: Wrong number of tokens returned by _count.\n");
      printf(" expected %d\n", true_n_tokens[i]);
      printf(" received %d\n", n_tokens);
      return(1);
    } else if (n_groups != true_n_groups[i]) {
      printf("\nRegex: '");
      for (int j = 0; regexes[i][j] != '\0'; j++) {
	printf("%s", SAFE_CHAR(regexes[i][j]));
      }
      printf("'\n\n");
      printf("ERROR: Wrong number of groups returned by _count.\n");
      printf(" expected %d\n", true_n_groups[i]);
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
    _set_jump(regexes[i], n_tokens, n_groups, tokens, jumps, jumpf, jumpi);

    // Verify the the tokens, jumps, jumpf, and jumpi..
    for (int j = 0; j < n_tokens; j++) {
      ji++; // (increment the test-wide counter for jumps)
      if (tokens[j] != true_tokens[i][j]) {
	printf("\nRegex: '");
	for (int j = 0; regexes[i][j] != '\0'; j++) {
	  printf("%s", SAFE_CHAR(regexes[i][j]));
	}
	printf("'\n\n");
	// Re-run the code with debug printing enabled.
	DO_PRINT = 1;
	_set_jump(regexes[i], n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
	printf("\n");
	printf("ERROR: Wrong TOKEN returned by _set_jump.\n");
	printf(" expected '%s' as token %d\n", SAFE_CHAR(true_tokens[i][j]), j);
	printf(" received '%s'\n", SAFE_CHAR(tokens[j]));
	return(3);
      } else if (jumps[j] != true_jumps[ji]) {
	printf("\nRegex: '");
	for (int j = 0; regexes[i][j] != '\0'; j++) {
	  printf("%s", SAFE_CHAR(regexes[i][j]));
	}
	printf("'\n");
	// Re-run the code with debug printing enabled.
	DO_PRINT = 1;
	_set_jump(regexes[i], n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
	printf("\n");
	printf("ERROR: Wrong JUMP S returned by _set_jump.\n");
	printf(" expected %d in col 0, row %d\n", true_jumps[ji], j);
	printf(" received %d\n", jumps[j]);
	return(4);
      } else if (jumpf[j] != true_jumpf[ji]) {
	printf("\nRegex: '");
	for (int j = 0; regexes[i][j] != '\0'; j++) {
	  printf("%s", SAFE_CHAR(regexes[i][j]));
	}
	printf("'\n");
	// Re-run the code with debug printing enabled.
	DO_PRINT = 1;
	_set_jump(regexes[i], n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
	printf("\n");
	printf("ERROR: Wrong JUMP F returned by _set_jump.\n");
	printf(" expected %d in col 1, row %d\n", true_jumpf[ji], j);
	printf(" received %d\n", jumpf[j]);
	return(5);
      } else if (jumpi[j] != true_jumpi[ji]) {
	printf("\nRegex: '");
	for (int j = 0; regexes[i][j] != '\0'; j++) {
	  printf("%s", SAFE_CHAR(regexes[i][j]));
	}
	printf("'\n");
	// Re-run the code with debug printing enabled.
	DO_PRINT = 1;
	_set_jump(regexes[i], n_tokens, n_groups, tokens, jumps, jumpf, jumpi);
	printf("\n");
	printf("ERROR: Wrong JUMP I returned by _set_jump.\n");
	printf(" expected %d in col 2, row %d\n", true_jumpi[ji], j);
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
    match(regexes[i], strings[i], &start, &end);

    if (start != match_starts[i]) {
    	DO_PRINT = 1;
    	match(regexes[i], strings[i], &start, &end);
    	printf("\nString: '");
    	for (int j = 0; strings[i][j] != '\0'; j++) {
    	  printf("%s", SAFE_CHAR(strings[i][j]));
    	}
	printf("'\n\n");
	printf("ERROR: Bad match START returned by match.\n");
	printf(" expected %d\n", match_starts[i]);
	printf(" received %d\n", start);
    	return(7);
    } else if (end != match_ends[i]) {
    	DO_PRINT = 1;
    	match(regexes[i], strings[i], &start, &end);	
    	printf("\nString: '");
    	for (int j = 0; strings[i][j] != '\0'; j++) {
    	  printf("%s", SAFE_CHAR(strings[i][j]));
    	}
	printf("'\n\n");
	printf("ERROR: Bad match END returned by match.\n");
	printf(" expected %d\n", match_ends[i]);
	printf(" received %d\n", end);
    	return(8);
    }

    // -------------------------------------------------------------

    // Exit once the empty regex has been verified.
    if (regexes[i][0] == '\0') done++;
  }
  
  printf("\n All tests PASSED.\n");
  // Successful return.
  return(0);
}

#endif


/*

----------------------------------------------------------------------
                        OLD CODE (trash bin)
----------------------------------------------------------------------


    if (DO_PRINT) {
      printf("\n token: %s", SAFE_CHAR(token));
      printf("\n redirect: ");
      for (int j = -1; j <= n_tokens; j++) printf("  %-3d", j);
      printf("\n           ");
      for (int j = -1; j <= n_tokens; j++) printf("  %-3d", redirect[j]);
      printf("\n");
    }


	    printf("\n assigned token redirect %d to token %d\n", 
		   group_nexts[gi], redirect[group_nexts[gi]]);


	    printf("\n assigned group redirect %d to group %d next, %d\n", 
		   group_nexts[gi], j, redirect[group_nexts[gi]]);


      // When do we know how the "group_next" is shifted?
      //   -> when the *next* was this group's *start*
      // Can a group that has *this* as next come before a group
      // that did not have this group as next?
      //   -> yes  (a(b))(c), a -> c
      // Can a group that started before the immediately previous group
      // in the stack have *this* as its next?
      //   -> no, because there cannot be empty groups, and if the
      //      group before the previous in the stack contained the
      //      previous, it would also contain this group and couldn't
      //      have this group's start as its next.


KEYBOARD MACRO FOR ADDING OUT-OF-BOUNDS CHECK, 
   PASTE INTO BUFFER THAT APPEARS WITH 
   M-x edit-last-kbd-macro

C-s			;; isearch-forward
[			;; self-insert-command
C-SPC			;; set-mark-command
C-s			;; isearch-forward
]			;; self-insert-command
C-b			;; backward-char
M-w			;; kill-ring-save
C-a			;; move-beginning-of-line
C-o			;; open-line
TAB			;; c-indent-line-or-region
if			;; self-insert-command * 2
SPC			;; self-insert-command
2*(			;; c-electric-paren
C-y			;; yank
SPC			;; self-insert-command
<			;; self-insert-command
SPC			;; self-insert-command
0			;; self-insert-command
)			;; c-electric-paren
SPC			;; self-insert-command
||			;; self-insert-command * 2
SPC			;; self-insert-command
(			;; c-electric-paren
C-y			;; yank
SPC			;; self-insert-command
>=			;; self-insert-command * 2
SPC			;; self-insert-command
n_groups		;; self-insert-command * 8
2*)			;; c-electric-paren
RET			;; newline
printf			;; self-insert-command * 6
(			;; c-electric-paren
""			;; self-insert-command * 2
C-b			;; backward-char
SPC			;; self-insert-command
ERROR			;; self-insert-command * 5
SPC			;; self-insert-command
C-y			;; yank
SPC			;; self-insert-command
out			;; self-insert-command * 3
SPC			;; self-insert-command
of			;; self-insert-command * 2
SPC			;; self-insert-command
range			;; self-insert-command * 5
:			;; c-electric-colon
SPC			;; self-insert-command
%d			;; self-insert-command * 2
C-f			;; forward-char
,			;; c-electric-semi&comma
SPC			;; self-insert-command
C-y			;; yank
)			;; c-electric-paren
;			;; c-electric-semi&comma
C-s			;; isearch-forward
[			;; self-insert-command
C-b			;; backward-char
C-SPC			;; set-mark-command
C-r			;; isearch-backward
SPC			;; self-insert-command
C-f			;; forward-char
M-w			;; kill-ring-save
C-a			;; move-beginning-of-line
C-p			;; previous-line
C-s			;; isearch-forward
of			;; self-insert-command * 2
SPC			;; self-insert-command
range			;; self-insert-command * 5
C-f			;; forward-char
C-b			;; backward-char
SPC			;; self-insert-command
(			;; c-electric-paren
C-y			;; yank
)			;; c-electric-paren
C-a			;; move-beginning-of-line
C-n			;; next-line

----------------------------------------------------------------------


    test[end] = '\0';
    test = test+start;
    printf("\n%s\n", test);


  printf("\nFirst memory address:  %p\n", (void*) jumps);
  printf(" last memory address:  %p\n", (void*) (jumpi+n_tokens));

  printf("\nFirst memory address:  %p\n", (void*) group_starts);
  printf(" last memory address:  %p\n", (void*) (g_mods+n_groups));
  printf("\n");
  	  printf(" gc_stack[%d]\t%p\n", j, (void*) &(gc_stack[j]) );
  	  printf(" group_nexts[%d]\t%p\n", gc_stack[j], (void*) &(group_nexts[gc_stack[j]]) );
      printf(" tokens[%d] \t%p\n", nt, (void*) &(tokens[nt])  );
      printf(" jumps[%d]  \t%p\n", nt, (void*) &(jumps[nt])   );
      printf(" jumpf[%d]  \t%p\n", nt, (void*) &(jumpf[nt])   );
      printf(" jumpi[%d]  \t%p\n", nt, (void*) &(jumpi[nt])   );


    printf("\n");
    printf("cstack:");
    for (int j=0; j<=ics; j++) {
      printf("  %-3d", cstack[j]);
    }
    printf("\n");
    printf("nstack:");
    for (int j=0; j<=ins; j++) {
      printf("  %-3d", nstack[j]);
    }

  // TODO
  // 
  //  once the actual matching code is written, memory should be
  //   redone so that all the necessary conditions are contiguous by
  //   index, instead of by condition type.
  // 

      // Do redirection correction for negated groups.
      if (cgs == '{') {
      	// Loop through all tokens in this group and reverse conditions..
      	for (int j = nt-1; j >= group_starts[gi]; j--) {
      	  int temps = jumps[j];
      	  token = tokens[j];
      	  // if this is a token set..
      	  if (jumpi[j]) {
      	    // TODO WARNING ERROR: This fails in the case of {{[ab]}}[ab]
      	    // if this is a repeated negation..
      	    if (temps == EXIT_TOKEN) {
      	      // if this is the last token (recover the "success" jump)
      	      if ((! jumpi[j+1]) || (jumpf[j] != j+1)) {
      		jumps[j] = jumpf[j];
      		jumpf[j] = temps;
      	      // if this is a non-last token (transfer "success" jump)
      	      } else {
      		jumps[j] = jumps[j+1];
      		jumpf[j] = j+1;
      	      }
      	    // if this is the first negation..
      	    } else {
      	      if (jumpf[j] != j+1) jumpf[j] = temps;
      	      jumps[j] = EXIT_TOKEN;
      	    }
      	  // If this is a regular token..
      	  } else if ((token != '*') && (token != '?') && (token != '|')) {
      	    jumps[j] = jumpf[j];
      	    jumpf[j] = temps;
      	  }
      	}
      }


// old NOT code for {}, new behavior is simpler and more intuitive

	// loop from start to here changing all special tokens that
	// point to redirect[group_nexts[gi]] to point to EXIT_TOKEN, 
	// make all tokens pointing to those special tokens that have
	// a failure exit redirct to redirect[group_nexts[gi]].
	for (int j = group_starts[gi]; j < nt; j++) {
	  // if the token is special and it jumps to the next group..
	  if (jumpf[j] == redirect[group_nexts[gi]]) {
	    // all tokens jumping into this ? will jump out on failure..
	    if (tokens[j] == '?') jumpf[j] = EXIT_TOKEN;
	    // there is no intuitive solution to * matching a negation..
	    else if (tokens[j] == '*') {
	      jumps[j] = EXIT_TOKEN;
	      jumpf[j] = EXIT_TOKEN;
	    }
	  // if the token jumps to a special token, and that special
	  // token jumps to the next group..
	  } else if (((tokens[jumps[j]] == '*') || (tokens[jumps[j]] == '?')) &&
		     ((jumpf[jumps[j]] == redirect[group_nexts[gi]]) ||
		      (jumps[jumps[j]] == redirect[group_nexts[gi]]))) {
	    // failure jumping "successfully" only happens if this is
	    // not a token that loops back to a *.
	    if ((j < jumps[j]) || (tokens[jumps[j]] != '*')) {
	      jumpf[j] = redirect[group_nexts[gi]];
	    } else {
	      jumps[j] = EXIT_TOKEN;
	    }
	  // if the token is in a token set and jumps out the group..
	  } else if (jumpi[j] && (jumps[j] == redirect[group_nexts[gi]])) {
	    jumps[j] = -1;
	  }
	}
	// if the last token used to point out of this group, the
	// conditions on that last token are flipped..
	if (((jumps[nt-1] == redirect[group_nexts[gi]]) && (jumpf[nt-1] == EXIT_TOKEN)) ||
	    ((jumpf[nt-1] == redirect[group_nexts[gi]]) && (jumps[nt-1] == EXIT_TOKEN))) {
	  gi = jumps[nt-1];
	  jumps[nt-1] = jumpf[nt-1];
	  jumpf[nt-1] = gi;
	// if this is a token set, correct the last character redirect
	} else if (jumpi[nt-1] && (jumps[nt-1] == EXIT_TOKEN) &&
		   (jumpf[nt-1] == EXIT_TOKEN)) {
	  jumpf[nt-1] = nt;
	}



       // Undo redirect from next token for negated groups.
	// Assign a redirect for negated groups (away from next group).
	if (cgs == '{') redirect[group_nexts[gi]] = EXIT_TOKEN;

    printf("\nRegex: %s\n", regex);
    printf("regex[%d] = '%c'\n", i, token);
    	printf("\n  nt nt+1 gn  %d %d %d %d\n", nt, jumps[nt], gi, group_nexts[gi]);
    	printf("\n  nt nt+1 nt+2  %d %d %d\n", nt, jumps[nt], jumpf[nt]);
    printf("         = '%c'\n", token);
    printf("  gi   %d\n", gi);
    printf("  iga  %d\n", iga);
    printf("  igc  %d\n", igc);
    printf("  cgs  %c\n", cgs);
    printf("  redirect\n    ");
    for (int j = 0; j < n_tokens; j++) printf("%c   ", tokens[j]);
    printf("\n    ");
    for (int j = 0; j < n_tokens; j++) printf("%-3d ", j);
    printf("\n");
    for (int j = -1; j <= n_tokens; j++) printf("%-3d ", redirect[j]);
    printf("\n  s ");
    for (int j = 0; j < n_tokens; j++) printf("%-3d ", jumps[j]);
    printf("\n  f ");
    for (int j = 0; j < n_tokens; j++) printf("%-3d ", jumpf[j]);
    printf("\n");



      	// jumps -> next group token
      	if ((iga > 0) && (s_stack[iga-1] == '{') &&
      	    group_nexts[gi_stack[iga-1]] == group_nexts[gi])
      	  jumps[nt] = EXIT_TOKEN; // match means {} matched, so exit
      	else
      	  jumps[nt] = group_nexts[gi]; // match means this group matched
      	// jumpf -> not the last token, so failure just moves to the next immediately
      	if (nt+1 != group_nexts[gi]) {
      	  jumpf[nt] = nt+1;
      	  jumpi[nt] = 1;
      	// if this is the last token in a {} then failure means sucess
      	} else if ((iga > 0) && (s_stack[iga-1] == '{') &&
      		   group_nexts[gi_stack[iga-1]] == group_nexts[gi]) {
      	  jumpf[nt] = group_nexts[gi];
      	}
      // else if in group
      } else if ((iga >= 0) && (s_stack[iga] == '{')) {
      	// a failure means the anti-group did not match (success)
      	jumpf[nt] = group_nexts[gi];
      }


	if ((iga >= 0) && (s_stack[iga] == '{'))
	  jumpf[nt] = EXIT_TOKEN; // set failure jump to exit for {}
	else
	  jumpf[nt] = nt+2; // set failure to next (non special) token

	  jumps[nt] = nt-1; // make * loop

      // handle standard groups
      if (token != '}') {
      	// check for repeating groups
      	if (g_mods[gi] == '*') 
      	  jumps[nt-1] = group_starts[gi]; // last token match causes cycle
      	// check for skipped groups
      	else if (g_mods[gi] == '|') {
      	  const char nx_token = regex[i+1]; // temporary storage
      	  int success_jump;
      	  if ((nx_token == '(') || (nx_token == '[') || (nx_token == '{'))
      	    success_jump = group_nexts[gi+1]; // last token jumps after next group
      	  else
      	    success_jump = nt+1; // last token jumps after next token
      	  jumps[nt-1] = success_jump;
      	  // TODO: test this ^^^
      	  // walk back over group redirecting the "success" jump
      	  for (int j = nt-1; j > group_starts[gi]; j--) {
      	    if (jumps[j] == group_nexts[gi]) jumps[j] = success_jump;
      	  }
      	}
      // handle negated groups (where last token match causes exit)
      } else {
      	jumps[nt-1] = EXIT_TOKEN; // success causes exit
      	// last token fail on an | goes to end of next token / group
      	if (g_mods[gi] == '|') {  //  make | correctly jump
      	  const char nxnx_token = regex[i+2];
      	  int fail_jump;
      	  if ((nxnx_token == '(') || (nxnx_token == '[') || (nxnx_token == '{'))
      	    fail_jump = group_nexts[gi+1]; // last token jumps after next group
      	  else
      	    fail_jump = nt+1; // last token jumps after next token
      	  jumpf[nt-1] = fail_jump;
      	  // walk backwards through tokens redirecting any incorrect jumps
      	  for (int j = nt-2; j > group_starts[gi]; j--) {
      	    if (jumpf[j] >= group_nexts[gi]) jumpf[j] = fail_jump;
      	  }

      	}
      }
      // TODO: test this ^^^



      // Re-assign the correct jump for the last token in token sets,
      // initially it would've been assigned a different jump on fail.
      if (token == ']') {
      	jumpi[nt-1] = 0; // no need to immediately jump on failure
      	JUMP(nt-1, group_nexts[gi], EXIT_TOKEN);
      }

  
   nja -> number of jump always conditions (count of * and ?)
   ntj -> number of tokens with a jump always condition, since
          multiple jump conditions can be assigned to one token
   jumpa should be of size (nja + 2*ntj + 1)
   jumpa = [start, count, jai1, jai2, ..., start, count, jai.., ..., -1]
  


    i = set_jumps(regex
    // Open and close character groups.
    if ((char_group < 0) && (s == '[')) {
      i++;
      s = regex[i];
      // If there is a real token in the character group, increment
      // token and set it as the first token of the character group.
      if ((s != '\0') && (s != ']')) {
  	char_group = token+1;
      }
    } else if ((char_group >= 0) && (s == ']')) {
      i++;
      s = regex[i];
      // If the next character causes a jump to the first character in
      // the group
    }
  }


  // If this group is followed by a "*" then all failed matches should
  // jump to the next token. Otherwise, failed matches terminate.

  // Storage for the current index in the regex and the jump arrays.
  int i; // regex index
  int j; // token index
  // Current character in the regex.
  char s = regex[i];
  while (s != '\0') {
    // If this starts a new group..
    if (((s == '[') || (s == '(')) && (first != '[')) {
      i++;
      step = set_jumps(s, regex+i, succ+step.b, fail+step.b);
    // If this ends the current "(" group..
    } else if ((s == ')') && (first == '(')) {
      // Check for a "*", if it exists, then set all failures of token
      // transitions in this group to exit the group and a success
      // in the final token to go back to the beginning.
      
    // If this ends the current "[" group..
    } else if ((s == ']') && (first == '[')) {
      i++;
      // Check for a "*". If it exists, set all successes to jump back
      // to the first token in this group and failures to jump out of
      // the group. If there is no "*"


      // Cycle over all tokens in this character group and set their
      // jumps to the appropriate places.

      return step;
    // If this is a special character
    } 

  }
  return step;


 */
