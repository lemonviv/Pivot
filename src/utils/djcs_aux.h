//
// Created by wuyuncheng on 12/10/19.
//

#ifndef COLLABORATIVEML_DJCS_AUX_H
#define COLLABORATIVEML_DJCS_AUX_H

#include <vector>
#include "gmp.h"
#include "libhcs.h"
#include "encoder.h"


/**
 * encrypt an EncodedNumber and return an EncodedNumber
 *
 * @param pk
 * @param hr
 * @param res : result
 * @param plain
 */
void djcs_aux_encrypt(djcs_public_key* pk, hcs_random* hr, EncodedNumber res, EncodedNumber plain);


/**
 * decrypt an EncodedNumber and return an EncodedNumber
 *
 * @param vk
 * @param res : result
 * @param cipher
 */
void djcs_aux_decrypt(djcs_private_key* vk, EncodedNumber res, EncodedNumber cipher);


/**
 * homomorphic addition of two ciphers and return an EncodedNumber
 *
 * @param pk
 * @param res : res
 * @param cipher1
 * @param cipher2
 */
void djcs_aux_ee_add(djcs_public_key* pk, EncodedNumber res, EncodedNumber cipher1, EncodedNumber cipher2);


/**
 * homomorphic multiplication of a cipher and a plain, return an EncodedNumber
 *
 * @param pk
 * @param res : res
 * @param cipher
 * @param plain
 */
void djcs_aux_ep_mul(djcs_public_key* pk, EncodedNumber res, EncodedNumber cipher, EncodedNumber plain);


/**
 * homomorphic inner product of a cipher vector and a plain vector
 * return an EncodedNumber
 *
 * @param pk
 * @param hr
 * @param res
 * @param ciphers
 * @param plains
 */
void djcs_aux_inner_product(djcs_public_key* pk, hcs_random* hr, EncodedNumber res,
        std::vector<EncodedNumber> ciphers, std::vector<EncodedNumber> plains);

#endif //COLLABORATIVEML_DJCS_AUX_H
