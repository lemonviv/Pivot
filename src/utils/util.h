//
// Created by wuyuncheng on 11/10/19.
//

#ifndef COLLABORATIVEML_UTIL_H
#define COLLABORATIVEML_UTIL_H

#include <stdio.h>

#define FLOAT_PRECISION 8
#define PRECISION_THRESHOLD 1e-8

/**
 * log file
 *
 * @param out
 * @param format
 * @param ...
 */
void logger(FILE* out, const char *format, ...);

/**
 * stdout print for debug
 *
 * @param str
 */
void print_string(const char *str);


#endif //COLLABORATIVEML_UTIL_H
