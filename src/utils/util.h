//
// Created by wuyuncheng on 11/10/19.
//

#ifndef COLLABORATIVEML_UTIL_H
#define COLLABORATIVEML_UTIL_H

#include <stdio.h>
#include "gmp.h"
#include "libhcs.h"

#define FLOAT_PRECISION 8
#define PRECISION_THRESHOLD 1e-6
#define TOTAL_CLIENT_NUM 3 // for test
#define REQUIRED_CLIENT_DECRYPTION 3 // for test

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


void compute_thresholds(djcs_t_public_key *pk, mpz_t n, mpz_t positive_threshold, mpz_t negative_threshold);


#endif //COLLABORATIVEML_UTIL_H
