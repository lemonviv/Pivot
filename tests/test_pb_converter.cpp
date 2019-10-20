//
// Created by wuyuncheng on 18/10/19.
//

#include "test_pb_converter.h"
#include <string>

extern hcs_random *hr;
extern djcs_t_public_key *pk;
extern djcs_t_private_key *vk;
//static djcs_t_auth_server **au = (djcs_t_auth_server **)malloc(TOTAL_CLIENT_NUM * sizeof(djcs_t_auth_server *));
//static mpz_t *si = (mpz_t *)malloc(TOTAL_CLIENT_NUM * sizeof(mpz_t));
extern djcs_t_auth_server **au;
extern mpz_t *si;
extern mpz_t n, positive_threshold, negative_threshold;
extern int total_cases_num, passed_cases_num;

void test_pb_encode_number() {
    EncodedNumber number;
    number.set_float(n, 0.123456);
    number.type = Plaintext;

    std::string s;
    serialize_encoded_number(number, s);
    EncodedNumber deserialized_number;
    deserialize_number_from_string(deserialized_number, s);

    // test equals
    if (number.exponent == deserialized_number.exponent && number.type == deserialized_number.type
            && mpz_cmp(number.n, deserialized_number.n) == 0 && mpz_cmp(number.value, deserialized_number.value) == 0) {
        total_cases_num += 1;
        passed_cases_num += 1;
        logger(stdout, "test_pb_encode_number: succeed\n");
    } else {
        total_cases_num += 1;
        logger(stdout, "test_pb_encode_number: failed\n");
    }
}


void test_pb_batch_ids() {
    int *batch_ids = new int[5];
    for (int i = 0; i < 5; i++) {
        batch_ids[i] = i + 1;
    }

    std::string s;
    serialize_batch_ids(batch_ids, 5, s);
    int *deserialized_batch_ids = new int[5];
    deserialize_ids_from_string(deserialized_batch_ids, s);

    // test equals
    bool is_success = true;
    for (int i = 0; i < 5; i++) {
        if (batch_ids[i] != deserialized_batch_ids[i]) {
            is_success = false;
            break;
        }
    }

    if (is_success) {
        total_cases_num += 1;
        passed_cases_num += 1;
        logger(stdout, "test_pb_batch_ids: succeed\n");
    } else {
        total_cases_num += 1;
        logger(stdout, "test_pb_batch_ids: failed\n");
    }
}


void test_pb_batch_sums() {

    EncodedNumber *batch_sums = new EncodedNumber[2];
    batch_sums[0].set_float(n, 0.123456);
    batch_sums[0].type = Plaintext;
    batch_sums[1].set_float(n, 0.654321);
    batch_sums[1].type = Ciphertext;

    std::string s;
    serialize_batch_sums(batch_sums, 2, s);
    EncodedNumber *deserialized_partial_sums = new EncodedNumber[2];
    int x;
    deserialize_sums_from_string(deserialized_partial_sums, x, s);

    // test equals
    bool is_success = true;
    for (int i = 0; i < 2; i++) {
        if (batch_sums[i].exponent == deserialized_partial_sums[i].exponent
            && batch_sums[i].type == deserialized_partial_sums[i].type
            && mpz_cmp(batch_sums[i].n, deserialized_partial_sums[i].n) == 0
            && mpz_cmp(batch_sums[i].value, deserialized_partial_sums[i].value) == 0) {
            continue;
        } else {
            is_success = false;
        }
    }

    if (is_success) {
        total_cases_num += 1;
        passed_cases_num += 1;
        logger(stdout, "test_pb_partial_sums: succeed\n");
    } else {
        total_cases_num += 1;
        logger(stdout, "test_pb_partial_sums: failed\n");
    }
}


void test_pb_batch_losses() {
    EncodedNumber *batch_losses = new EncodedNumber[2];
    batch_losses[0].set_float(n, 0.123456);
    batch_losses[0].type = Plaintext;
    batch_losses[1].set_float(n, 0.654321);
    batch_losses[1].type = Ciphertext;

    std::string s;
    serialize_batch_losses(batch_losses, 2, s);
    EncodedNumber *deserialized_batch_losses = new EncodedNumber[2];
    deserialize_losses_from_string(deserialized_batch_losses, s);

    // test equals
    bool is_success = true;
    for (int i = 0; i < 2; i++) {
        if (batch_losses[i].exponent == deserialized_batch_losses[i].exponent
            && batch_losses[i].type == deserialized_batch_losses[i].type
            && mpz_cmp(batch_losses[i].n, deserialized_batch_losses[i].n) == 0
            && mpz_cmp(batch_losses[i].value, deserialized_batch_losses[i].value) == 0) {
            continue;
        } else {
            is_success = false;
        }
    }

    if (is_success) {
        total_cases_num += 1;
        passed_cases_num += 1;
        logger(stdout, "test_pb_batch_losses: succeed\n");
    } else {
        total_cases_num += 1;
        logger(stdout, "test_pb_batch_losses: failed\n");
    }
}


int test_pb() {

    logger(stdout, "****** Test protobuf serialization and deserialization ******\n");

    total_cases_num = 0;
    passed_cases_num = 0;

    test_pb_encode_number();
    test_pb_batch_ids();
    test_pb_batch_sums();
    test_pb_batch_losses();

    logger(stdout, "****** total_cases_num = %d, passed_cases_num = %d ******\n",
           total_cases_num, passed_cases_num);

    return 0;
}