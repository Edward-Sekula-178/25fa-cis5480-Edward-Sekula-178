#include <string.h>
#include "../penn-vector/Vec.h"
/*!
 * Breaks a given string into a sequence of non-empty tokens.
 * The tokens are stored in a vector and returned.
 *
 * @param to_split  the  C-string that will be broken up into tokens
 * @param delim     a C-string containing all the characters used to
 *                  delimit (mark the end of) the tokens.
 * @returns a vector of non-empty tokens
 * @pre Assumes that to_split and delim are both properly
 *      null-terminated C-strings
 * @post to_split will be modified by being "broken up"
 *       into tokens. Deallocating to_split would mean
 *       the pointers in the vector no longer point to
 *       the characters in the token.
 *
 * Notably, the to_split string that is passed in
 * is split in-place. Meaning that all returned char* will point
 * to substrings inside of the to_split string, so to_split and the
 * tokens share a lifetime. In other words, if one were to deallocate
 * to_split then all tokens will point to invalid memory.
 *
 * For example: consider that we have the following code:
 * char[] str = "marietta";
 * Vec tokens = split_string(str, "rt");
 *
 * The contents of the char array `str`after the split would be:
 * ['m', 'a', '\0', 'i', 'e', '\0', '\0', 'a', '\0']
 * and the vector would contain N character pointers:
 * - First pointer points to the token "ma" (index 0 in str)
 * - Second pointer points to the token "ie" (index 3 in str)
 * - Third pointer points to the token "a" (index 7 in str)
 */
Vec split_string(char* to_split, const char* delim) {
  Vec tokens = vec_new(5, NULL);
  char* token = strtok(to_split, delim);
  while (token != NULL) {
    vec_push_back(&tokens, token);
    token = strtok(NULL, delim);
  }
  return tokens;
}