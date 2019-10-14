//
// Created by wuyuncheng on 12/10/19.
//

#ifndef COLLABORATIVEML_PCS_T_AUX_H
#define COLLABORATIVEML_PCS_T_AUX_H

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
void pcs_t_aux_encrypt(pcs_t_public_key* pk, hcs_random* hr, EncodedNumber res, EncodedNumber plain);


/**
 * decrypt an EncodedNumber and return an EncodedNumber
 *
 * @param pk
 * @param au
 * @param res : result
 * @param cipher
 */
void pcs_t_aux_partial_decrypt(pcs_t_public_key* pk, pcs_t_auth_server* au,
        EncodedNumber res, EncodedNumber cipher);


/**
 * combine shares give hcs_shares, should set n and exponent
 * before calling this function
 *
 * @param pk
 * @param res
 * @param hs
 */
void pcs_t_aux_hcs_share_combine(pcs_t_public_key* pk, EncodedNumber res, hcs_shares* hs);


/**
 * set a share for decryption
 *
 * @param shares
 * @param dec
 * @param i
 */
void pcs_t_aux_hcs_set_share(hcs_shares *shares, EncodedNumber partial_dec, unsigned long i);


/**
 * homomorphic addition of two ciphers and return an EncodedNumber
 *
 * @param pk
 * @param res : res
 * @param cipher1
 * @param cipher2
 */
void pcs_t_aux_ee_add(pcs_t_public_key* pk, EncodedNumber res, EncodedNumber cipher1, EncodedNumber cipher2);


/**
 * homomorphic multiplication of a cipher and a plain, return an EncodedNumber
 *
 * @param pk
 * @param res : res
 * @param cipher
 * @param plain
 */
void pcs_t_aux_ep_mul(pcs_t_public_key* pk, EncodedNumber res, EncodedNumber cipher, EncodedNumber plain);


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
void pcs_t_aux_inner_product(pcs_t_public_key* pk, hcs_random* hr, EncodedNumber res,
        std::vector<EncodedNumber> ciphers, std::vector<EncodedNumber> plains);

#endif //COLLABORATIVEML_PCS_T_AUX_H
