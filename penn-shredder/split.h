#ifndef SPLIT_H
#define SPLIT_H

#include "../penn-vector/Vec.h"

/*
 * Splits a string into non-empty tokens using the given delimiters.
 * The returned vector contains pointers to tokens within the original string.
 *
 * @param to_split  The C-string to split (modified in-place)
 * @param delim     C-string of delimiter characters
 * @return          Vector of char* tokens
 * @pre             to_split and delim are null-terminated
 * @post            to_split is modified; tokens share its lifetime
 */
Vec split_string(char* to_split, const char* delim);

#endif  // SPLIT_H
