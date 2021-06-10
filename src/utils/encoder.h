//
// Created by wuyuncheng on 12/10/19.
//

#ifndef PIVOT_ENCODER_H
#define PIVOT_ENCODER_H

#include "libhcs.h"
#include "gmp.h"
#include "util.h"

/**
 * After decryption, there are four states for the decrypted number:
 *
 * Invalid : corrupted encoded number (should mod n if needed)
 * Positive: encoded number is a positive number
 * Negative: encoded number is a negative number, should minus n before decoding
 * Overflow: encoded number is overflowed
 */
enum EncodedNumberState {Invalid, Positive, Negative, Overflow};

/**
 * describe the type of the EncodedNumber, three types
 *
 * Plaintext: the original value or revealed secret shared value
 * Ciphertext: the encrypted value by threshold Damgard Jurik cryptosystem
 * SecretShare: the secret shared value in SCALE-MAMBA
 */
enum EncodedNumberType {Plaintext, Ciphertext, SecretShare};

class EncodedNumber {
public:
    mpz_t            n;               // max value in public key for encoding
    mpz_t            value;           // the value in mpz_t form
    int              exponent;        // fixed pointed integer representation, 0 for integer, negative for float
    int              type;            // the encoded number type
    //bool             is_encrypted;    // default is false, a plaintext value; after encryption, set true
public:
    /**
     * default constructor
     */
    EncodedNumber();

    /**
     * copy constructor
     *
     * @param number
     */
    EncodedNumber(const EncodedNumber & number);

    /**
     * copy assignment constructor
     *
     * @param number
     * @return
     */
    EncodedNumber &operator = (const EncodedNumber & number);

    /**
     * set for int value
     *
     * @param n
     * @param v
     */
    void set_integer(mpz_t n, int v);

    /**
     * set for float value
     *
     * @param n
     * @param v
     * @param precision
     */
    void set_float(mpz_t n, float v, int precision = FLOAT_PRECISION);

    /**
     * destructor
     */
    ~EncodedNumber();

    /**
     * make two EncodedNumber exponent the same for computation
     * on the same level, only reasonable when is on plaintexts
     *
     * @param new_exponent
     */
    void decrease_exponent(int new_exponent);

    /**
     * given a mpz_t value, increase its exponent for control
     * NOTE: this function only work for plaintext and will lose some precision
     * should be used carefully
     *
     * @param new_exponent
     */
    void increase_exponent(int new_exponent);

    /**
     * decode to long value
     * @deprecated
     *
     * @param v
     */
    void decode(long & v);

    /**
     * decode to float value
     *
     * @param v
     */
    void decode(float & v);

    /**
     * when exponent is large, decode with truncation
     *
     * @param v
     * @param truncated_exponent
     */
    void decode_with_truncation(float & v, int truncated_exponent);

    /**
    * when exponent is large, decode with truncation
    *
    * @param truncated_exponent
    */
    //void truncate_exponent(int truncated_exponent);

    /**
     * check encoded number
     * if value >= n, then return Invalid
     * else if value <= max_int then return positive
     * else if value >= n - max_int then return negative
     * else (max_int < value < n - max_int) then return overflow
     *
     * NOTE: this function only works for the plaintext after decryption,
     * if an encoded number is not encrypted and decrypted, it should always return
     * a positive state because we assume the plaintext will never
     * surpass the max_int of the djcs_t cryptosystem?
     *
     * @return
     */
    EncodedNumberState check_encoded_number();

    /**
     * compute n // 3 - 1 as max_int
     *
     * @param max_int
     */
    void compute_decode_threshold(mpz_t max_int);

    /**
     * print
     */
    void print_encoded_number();
};

/**
 * represent a float with fixed pointed integer
 *
 * @param value
 * @param precision : exponent with precision
 * @return
 */
long long fixed_pointed_integer_representation(float value, int precision);

/**
 * encode an integer with mpz_t
 *
 * @param value
 * @param res
 * @param exponent
 */
void fixed_pointed_encode(long value, mpz_t res, int & exponent);

/**
 * encode a float with mpz_t
 *
 * @param value
 * @param precision
 * @param res
 * @param exponent
 */
void fixed_pointed_encode(float value, int precision, mpz_t res, int & exponent);

/**
 * decode a mpz_t to a long value when exponent is 0
 *
 * @param value
 * @param res
 */
void fixed_pointed_decode(long & value, mpz_t res);

/**
 * decode a mpz_t to a float value when exponent is not 0
 *
 * @param value
 * @param res
 * @param exponent
 */
void fixed_pointed_decode(float & value, mpz_t res, int exponent);

/**
 * decode a mpz_t to a float value when exponent is not 0 with precision truncation
 * NOTE: the truncation can only be applied on plaintexts
 *
 * @param value
 * @param res
 * @param exponent
 * @param truncated_exponent : truncate the res to desired precision to avoid overflow
 */
void fixed_pointed_decode_truncated(float & value, mpz_t res, int exponent, int truncated_exponent);

/**
 * temporary decrypt for debugging, should remove in practical use
 *
 * @param pk
 * @param au
 * @param required_client_num
 * @param rop
 * @param v
 */
void decrypt_temp(djcs_t_public_key *pk, djcs_t_auth_server **au, int required_client_num, EncodedNumber & rop, EncodedNumber v);

#endif //PIVOT_ENCODER_H
