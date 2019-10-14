//
// Created by wuyuncheng on 12/10/19.
//


#include "pcs_t_aux.h"


void pcs_t_aux_encrypt(pcs_t_public_key* pk, hcs_random* hr, EncodedNumber res, EncodedNumber plain) {
    mpz_set(res.n, plain.n);
    res.exponent = plain.exponent;
    pcs_t_encrypt(pk, hr, res.value, plain.value);
}


void pcs_t_aux_partial_decrypt(pcs_t_public_key* pk, pcs_t_auth_server* au, EncodedNumber res, EncodedNumber cipher) {
    mpz_set(res.n, cipher.n);
    res.exponent = cipher.exponent;
    pcs_t_share_decrypt(pk, au, res.value, cipher.value);
}


void pcs_t_aux_hcs_share_combine(pcs_t_public_key* pk, EncodedNumber res, hcs_shares* hs) {
    pcs_t_share_combine(pk, res.value, hs);
}


void pcs_t_aux_hcs_set_share(hcs_shares *shares, EncodedNumber partial_dec, unsigned long i) {
    hcs_set_share(shares, partial_dec.value, i);
}


void pcs_t_aux_ee_add(pcs_t_public_key* pk, EncodedNumber res, EncodedNumber cipher1, EncodedNumber cipher2) {

    if (mpz_cmp(cipher1.n, cipher2.n) != 0) {
        logger(stdout, "two ciphertexts not with the same public key\n");
        return;
    }
    mpz_set(res.n, cipher1.n);

    // align the two ciphertexts with the same exponent
    if (cipher1.exponent == cipher2.exponent) {
        res.exponent = cipher1.exponent;
    } else if (cipher1.exponent < cipher2.exponent) {
        // cipher1's precision is high, should decrease cipher2
        cipher2.decrease_exponent(cipher1.exponent);
        res.exponent = cipher1.exponent;
    } else {
        // cipher2's precision is high, should decrease cipher1
        cipher1.decrease_exponent(cipher2.exponent);
        res.exponent = cipher2.exponent;
    }

    pcs_t_ee_add(pk, res.value, cipher1.value, cipher2.value);
}


void pcs_t_aux_ep_mul(pcs_t_public_key* pk, EncodedNumber res, EncodedNumber cipher, EncodedNumber plain) {

    if (mpz_cmp(cipher.n, plain.n) != 0) {
        logger(stdout, "two values not under the same public key\n");
        return;
    }
    mpz_set(res.n, cipher.n);
    res.exponent = cipher.exponent + plain.exponent;
    pcs_t_ep_mul(pk, res.value, cipher.value, plain.value);
}


void pcs_t_aux_inner_product(pcs_t_public_key* pk, hcs_random* hr, EncodedNumber res,
        std::vector<EncodedNumber> ciphers, std::vector<EncodedNumber> plains) {

    if (ciphers.size() != plains.size()) {
        logger(stdout, "two vectors do not have the same size\n");
        return;
    }

    if (ciphers.size() == 0) {
        logger(stdout, "no elements in the two vectors\n");
        return;
    }

    // assume the elements in a vector has the same n and exponent
    if (mpz_cmp(ciphers[0].n, plains[0].n) != 0) {
        logger(stdout, "two values not under the same public key\n");
        return;
    }

    mpz_set(res.n, ciphers[0].n);
    res.exponent = ciphers[0].exponent + plains[0].exponent;
    mpz_set_ui(res.value, 0);
    pcs_t_aux_encrypt(pk, hr, res, res);
    for (int i = 0; i < ciphers.size(); ++i) {
        EncodedNumber number;
        pcs_t_aux_ep_mul(pk, number, ciphers[i], plains[i]);
        pcs_t_aux_ee_add(pk, res, res, number);
    }
}