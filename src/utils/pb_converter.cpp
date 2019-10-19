//
// Created by wuyuncheng on 18/10/19.
//

#include "pb_converter.h"
#include "util.h"
#include "../include/protobuf/common.pb.h"
#include "../include/protobuf/logistic.pb.h"



void serialize_encoded_number(EncodedNumber number, std::string & output_str) {

    com::collaborative::ml::PB_EncodedNumber pb_number;

    std::string n_str, value_str;
    n_str = mpz_get_str(NULL, 10, number.n);
    value_str = mpz_get_str(NULL, 10, number.value);

    pb_number.set_n(n_str);
    pb_number.set_value(value_str);
    pb_number.set_exponent(number.exponent);
    pb_number.set_type(number.type);

    pb_number.SerializeToString(& output_str);
}


void deserialize_number_from_string(EncodedNumber & number, std::string input_str) {

    com::collaborative::ml::PB_EncodedNumber deserialized_pb_number;
    if (!deserialized_pb_number.ParseFromString(input_str)) {
        logger(stdout, "Failed to parse PB_EncodedNumber from string\n");
        return;
    }

    mpz_set_str(number.n, deserialized_pb_number.n().c_str(), 10);
    mpz_set_str(number.value, deserialized_pb_number.value().c_str(), 10);
    number.exponent = deserialized_pb_number.exponent();
    number.type = deserialized_pb_number.type();
}


void serialize_batch_ids(int *batch_ids, int size, std::string & output_str) {

    com::collaborative::ml::PB_BatchIds pb_batch_ids;
    for (int i = 0; i < size; i++) {
        pb_batch_ids.add_batch_id(batch_ids[i]);
    }
    pb_batch_ids.SerializeToString(&output_str);
}

void deserialize_ids_from_string(int *& batch_ids, std::string input_str) {

    com::collaborative::ml::PB_BatchIds deserialized_pb_batch_ids;
    if (!deserialized_pb_batch_ids.ParseFromString(input_str)) {
        logger(stdout, "Failed to parse PB_BatchIds from string\n");
        return;
    }

    for (int i = 0; i < deserialized_pb_batch_ids.batch_id_size(); i++) {
        batch_ids[i] = deserialized_pb_batch_ids.batch_id(i);
    }
}


void serialize_batch_sums(EncodedNumber *batch_sums, int size, std::string & output_str) {

    com::collaborative::ml::PB_BatchSums pb_batch_sums;

    for (int i = 0; i < size; i++) {

        com::collaborative::ml::PB_EncodedNumber *pb_number = pb_batch_sums.add_batch_sum();

        std::string n_str, value_str;
        n_str = mpz_get_str(NULL, 10, batch_sums[i].n);
        value_str = mpz_get_str(NULL, 10, batch_sums[i].value);
        pb_number->set_n(n_str);
        pb_number->set_value(value_str);
        pb_number->set_exponent(batch_sums[i].exponent);
        pb_number->set_type(batch_sums[i].type);
    }

    pb_batch_sums.SerializeToString(&output_str);
}


void deserialize_sums_from_string(EncodedNumber *& partial_sums, std::string input_str) {

    com::collaborative::ml::PB_BatchSums deserialized_batch_partial_sums;
    if (!deserialized_batch_partial_sums.ParseFromString(input_str)) {
        logger(stdout, "Failed to parse PB_BatchPartialSums from string\n");
        return;
    }

    for (int i = 0; i < deserialized_batch_partial_sums.batch_sum_size(); i++) {

        com::collaborative::ml::PB_EncodedNumber pb_number = deserialized_batch_partial_sums.batch_sum(i);

        mpz_set_str(partial_sums[i].n, pb_number.n().c_str(), 10);
        mpz_set_str(partial_sums[i].value, pb_number.value().c_str(), 10);
        partial_sums[i].exponent = pb_number.exponent();
        partial_sums[i].type = pb_number.type();
    }
}


void serialize_batch_losses(EncodedNumber *batch_losses, int size, std::string & output_str) {

    com::collaborative::ml::PB_BatchLosses pb_batch_losses;

    for (int i = 0; i < size; i++) {

        com::collaborative::ml::PB_EncodedNumber *pb_number = pb_batch_losses.add_batch_loss();

        std::string n_str, value_str;
        n_str = mpz_get_str(NULL, 10, batch_losses[i].n);
        value_str = mpz_get_str(NULL, 10, batch_losses[i].value);
        pb_number->set_n(n_str);
        pb_number->set_value(value_str);
        pb_number->set_exponent(batch_losses[i].exponent);
        pb_number->set_type(batch_losses[i].type);
    }

    pb_batch_losses.SerializeToString(&output_str);
}


void deserialize_losses_from_string(EncodedNumber *& batch_losses, std::string input_str) {

    com::collaborative::ml::PB_BatchLosses deserialized_batch_losses;
    if (!deserialized_batch_losses.ParseFromString(input_str)) {
        logger(stdout, "Failed to parse PB_BatchPartialSums from string\n");
        return;
    }

    for (int i = 0; i < deserialized_batch_losses.batch_loss_size(); i++) {

        com::collaborative::ml::PB_EncodedNumber pb_number = deserialized_batch_losses.batch_loss(i);

        mpz_set_str(batch_losses[i].n, pb_number.n().c_str(), 10);
        mpz_set_str(batch_losses[i].value, pb_number.value().c_str(), 10);
        batch_losses[i].exponent = pb_number.exponent();
        batch_losses[i].type = pb_number.type();
    }
}