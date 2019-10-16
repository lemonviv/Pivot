//
// Created by wuyuncheng on 15/10/19.
//

#include "../src/utils/djcs_t_aux.h"
#include "../src/utils/util.h"
#include "../src/utils/encoder.h"
#include "libhcs.h"
#include "gmp.h"
#include <cmath>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "test_djcs_t_aux.h"

#define TOTAL_CLIENT_NUM 3
#define REQUIRED_CLIENT_DECRYPTION 3

static hcs_random *hr1;
static djcs_t_public_key *pk1;
static djcs_t_private_key *vk1;
static djcs_t_auth_server **au = (djcs_t_auth_server **)malloc(TOTAL_CLIENT_NUM * sizeof(djcs_t_auth_server *));
static mpz_t *si = (mpz_t *)malloc(TOTAL_CLIENT_NUM * sizeof(mpz_t));
static mpz_t n1, positive_threshold1, negative_threshold1;
int total_cases_num1, passed_cases_num1;


void decrypt_temp(mpz_t rop, mpz_t v) {
    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        //djcs_t_share_decrypt(pk1, au[j], dec[j], ciphers[2].value);
        djcs_t_share_decrypt(pk1, au[j], dec[j], v);
    }

    djcs_t_share_combine(pk1, rop, dec);
}


void aux_compute_thresholds() {
    mpz_t g;
    mpz_init(g);
    mpz_set(g, pk1->g);
    mpz_sub_ui(n1, g, 1);

    mpz_t t;
    mpz_init(t);
    mpz_fdiv_q_ui(t, n1, 3);
    mpz_sub_ui(positive_threshold1, t, 1);  // this is positive threshold
    mpz_sub(negative_threshold1, n1, positive_threshold1);  // this is negative threshold

    mpz_clear(g);
    mpz_clear(t);
}


void test_encryption_decryption_int(int x) {

    EncodedNumber *a = new EncodedNumber();
    a->set_integer(n1, x);
    EncodedNumber *encrypted_a = new EncodedNumber();
    djcs_t_aux_encrypt(pk1, hr1, *encrypted_a, *a);

    EncodedNumber *decrypted_a = new EncodedNumber();
    decrypted_a->exponent = encrypted_a->exponent;
    mpz_set(decrypted_a->n, encrypted_a->n);
    mpz_t *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        djcs_t_share_decrypt(pk1, au[j], dec[j], encrypted_a->value);
    }
    djcs_t_share_combine(pk1, decrypted_a->value, dec);
    decrypted_a->is_encrypted = false;

    // decode decrypted_a
    long y;
    decrypted_a->decode(y);
    if (x != y) {
        logger(stdout, "test_encryption_decryption_int: "
                       "the decrypted plaintext %d is not match original plaintext %d, failed\n", y, x);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_encryption_decryption_int；"
                       "the encryption and decryption test succeed\n");
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }
}


void test_encryption_decryption_float(float x) {

    auto *a = new EncodedNumber();
    a->set_float(n1, x, FLOAT_PRECISION);
    auto *encrypted_a = new EncodedNumber();
    djcs_t_aux_encrypt(pk1, hr1, *encrypted_a, *a);

    auto *decrypted_a = new EncodedNumber();
    decrypted_a->exponent = encrypted_a->exponent;
    mpz_set(decrypted_a->n, encrypted_a->n);
    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        djcs_t_share_decrypt(pk1, au[j], dec[j], encrypted_a->value);
    }
    djcs_t_share_combine(pk1, decrypted_a->value, dec);
    decrypted_a->is_encrypted = false;

    // decode decrypted_a
    float y;
    decrypted_a->decode(y);
    if (fabs(y - x) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_encryption_decryption_float: "
                       "the decrypted plaintext %f is not match original plaintext %f, failed\n", y, x);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_encryption_decryption_float: "
                       "the encryption and decryption test succeed\n");
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }
}


void test_ee_add() {
    auto *plain1 = new EncodedNumber();
    plain1->set_float(n1, 0.25, FLOAT_PRECISION);
    auto *cipher1 = new EncodedNumber();
    djcs_t_aux_encrypt(pk1, hr1, *cipher1, *plain1);

    auto *plain2 = new EncodedNumber();
    plain2->set_float(n1, 0.12, FLOAT_PRECISION);
    auto *cipher2 = new EncodedNumber();
    djcs_t_aux_encrypt(pk1, hr1, *cipher2, *plain2);

    auto *res = new EncodedNumber();
    djcs_t_aux_ee_add(pk1, *res, *cipher1, *cipher2);

    auto *decryption = new EncodedNumber();
    decryption->exponent = res->exponent;
    mpz_set(decryption->n, res->n);

    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        djcs_t_share_decrypt(pk1, au[j], dec[j], res->value);
    }
    djcs_t_share_combine(pk1, decryption->value, dec);
    decryption->is_encrypted = false;

    // decode decrypted_a
    float y;
    decryption->decode(y);
    if (fabs(y - (0.25 + 0.12)) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_ee_add: "
                       "the decrypted result %f is not match original plaintext addition %f, failed\n", y, 0.25 + 0.12);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_ee_add: "
                       "the homomorphic addition computation test succeed\n");
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }
}


void test_ep_mul() {

    auto *plain1 = new EncodedNumber();
    plain1->set_float(n1, 0.25, FLOAT_PRECISION);
    auto *cipher1 = new EncodedNumber();
    djcs_t_aux_encrypt(pk1, hr1, *cipher1, *plain1);

    auto *plain2 = new EncodedNumber();
    plain2->set_float(n1, 0.12, FLOAT_PRECISION);

    auto *res = new EncodedNumber();
    djcs_t_aux_ep_mul(pk1, *res, *cipher1, *plain2);

    auto *decryption = new EncodedNumber();
    decryption->exponent = res->exponent;
    mpz_set(decryption->n, res->n);

    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        djcs_t_share_decrypt(pk1, au[j], dec[j], res->value);
    }
    djcs_t_share_combine(pk1, decryption->value, dec);
    decryption->is_encrypted = false;

    // decode decrypted_a
    float y;
    decryption->decode(y);
    if (fabs(y - (0.25 * 0.12)) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_ep_mul: "
                       "the decrypted result %f is not match original plaintext addition %f, failed\n", y, 0.25 + 0.12);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_ep_mul: "
                       "the homomorphic multiplication computation test succeed\n");
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }
}


void test_inner_product_int() {

    int feature_num = 3;
    long plain_res = 0;

    EncodedNumber *ciphers = new EncodedNumber[feature_num];
    EncodedNumber *plains = new EncodedNumber[feature_num];

    for (int i = 0; i < feature_num; i++) {

        EncodedNumber *plain1 = new EncodedNumber();
        plain1->set_integer(n1, i + 1);
        djcs_t_aux_encrypt(pk1, hr1, ciphers[i], *plain1);
        plains[i].set_integer(n1, feature_num + i);

        plain_res = plain_res + (i + 1) * (feature_num + i);
    }

    EncodedNumber res;
    djcs_t_aux_inner_product(pk1, hr1, res, ciphers, plains, feature_num);

    auto *decryption = new EncodedNumber();
    decryption->exponent = res.exponent;
    mpz_set(decryption->n, res.n);

    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        //djcs_t_share_decrypt(pk1, au[j], dec[j], ciphers[2].value);
        djcs_t_share_decrypt(pk1, au[j], dec[j], res.value);
    }

    djcs_t_share_combine(pk1, decryption->value, dec);
    decryption->is_encrypted = false;

    // decode decrypted_a
    long y;
    decryption->decode(y);
    if (fabs(y - plain_res) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_inner_product_int: "
                       "the decrypted result %d is not match original plaintext inner product %d, failed\n", y, plain_res);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_inner_product_int: "
                       "the homomorphic inner product computation test succeed, expected value = %d, real value = %d\n", plain_res, y);
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }

    delete [] ciphers;
    delete [] plains;
}



void test_inner_product_float() {

    int feature_num = 5;
    float plain_res = 0.0;

    EncodedNumber *ciphers = new EncodedNumber[feature_num];
    EncodedNumber *plains = new EncodedNumber[feature_num];

    for (int i = 0; i < feature_num; i++) {
        //ciphers[i] = new EncodedNumber();

        EncodedNumber *plain1 = new EncodedNumber();
        plain1->set_float(n1, ((float) i + 1) / ((float) feature_num + i), FLOAT_PRECISION);
        djcs_t_aux_encrypt(pk1, hr1, ciphers[i], *plain1);
        plains[i].set_float(n1, ((float) i + 2) / ((float) feature_num + i), FLOAT_PRECISION);

        plain_res = plain_res + ((float) i + 1) / ((float) feature_num + i) * ((float) i + 2) / ((float) feature_num + i);
    }

    EncodedNumber res;
    djcs_t_aux_inner_product(pk1, hr1, res, ciphers, plains, feature_num);

    auto *decryption = new EncodedNumber();
    decryption->exponent = res.exponent;
    mpz_set(decryption->n, res.n);

    auto *dec = (mpz_t *) malloc (REQUIRED_CLIENT_DECRYPTION * sizeof(mpz_t));
    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        mpz_init(dec[j]);
    }

    for (int j = 0; j < REQUIRED_CLIENT_DECRYPTION; j++) {
        //djcs_t_share_decrypt(pk1, au[j], dec[j], ciphers[2].value);
        djcs_t_share_decrypt(pk1, au[j], dec[j], res.value);
    }

    djcs_t_share_combine(pk1, decryption->value, dec);
    decryption->is_encrypted = false;

    // decode decrypted_a
    float y;
    decryption->decode(y);
    if (fabs(y - plain_res) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_inner_product_float: "
                       "the decrypted result %f is not match original plaintext inner product %f, failed\n", y, plain_res);
        total_cases_num1 += 1;
    } else {
        logger(stdout, "test_inner_product_float: "
                       "the homomorphic inner product computation test succeed, expected_value = %f, real_value = %f\n", plain_res, y);
        total_cases_num1 += 1;
        passed_cases_num1 += 1;
    }

    delete [] ciphers;
    delete [] plains;
}


int test_djcs_t_aux()
{
    logger(stdout, "Test djcs_t auxiliary functions\n");

    hr1 = hcs_init_random();
    pk1 = djcs_t_init_public_key();
    vk1 = djcs_t_init_private_key();

    djcs_t_generate_key_pair(pk1, vk1, hr1, 1, 1024, 3, 3);

    mpz_t *coeff = djcs_t_init_polynomial(vk1, hr1);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_init(si[i]);
        djcs_t_compute_polynomial(vk1, coeff, si[i], i);
        au[i] = djcs_t_init_auth_server();
        djcs_t_set_auth_server(au[i], si[i], i);
    }

    mpz_init(n1);
    mpz_init(positive_threshold1);
    mpz_init(negative_threshold1);

    total_cases_num1 = 0;
    passed_cases_num1 = 0;

    // compute threshold
    aux_compute_thresholds();

    // test encryption decryption int
    test_encryption_decryption_int(10);
    test_encryption_decryption_int(-10);  // not sure why call two times cause double free exception

    // test encryption decryption float
    test_encryption_decryption_float(0.123456);
    test_encryption_decryption_float(-0.654321);

    // test homomorphic addition
    test_ee_add();

    // test homomorphic multiplication
    test_ep_mul();

    // test homomorphic inner product int
    test_inner_product_int();

    // test homomorphic inner product float
    test_inner_product_float();


    logger(stdout, "total_cases_num = %d, passed_cases_num = %d\n",
           total_cases_num1, passed_cases_num1);

    // free memory
    hcs_free_random(hr1);
    djcs_t_free_public_key(pk1);
    djcs_t_free_private_key(vk1);

    mpz_clear(n1);
    mpz_clear(positive_threshold1);
    mpz_clear(negative_threshold1);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_clear(si[i]);
        djcs_t_free_auth_server(au[i]);
    }
    free(si);
    free(au);

    djcs_t_free_polynomial(vk1, coeff);

    return 0;
}