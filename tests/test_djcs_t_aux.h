//
// Created by wuyuncheng on 16/10/19.
//

#ifndef COLLABORATIVEML_TEST_DJCS_T_AUX_H
#define COLLABORATIVEML_TEST_DJCS_T_AUX_H

#include "gmp.h"

void decrypt_temp(mpz_t rop, mpz_t v);
void aux_compute_thresholds();
void test_encryption_decryption_int(int x);
void test_encryption_decryption_float(float x);
void test_ee_add();
void test_ep_mul();
void test_inner_product_int();
void test_inner_product_float();
int test_djcs_t_aux();

#endif //COLLABORATIVEML_TEST_DJCS_T_AUX_H
