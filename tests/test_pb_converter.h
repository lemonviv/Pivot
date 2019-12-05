//
// Created by wuyuncheng on 18/10/19.
//

#ifndef COLLABORATIVEML_TEST_PB_CONVERTER_H
#define COLLABORATIVEML_TEST_PB_CONVERTER_H

#include "../src/utils/pb_converter.h"
#include "../src/utils/encoder.h"
#include "../src/utils/util.h"

void test_pb_encode_number();
void test_pb_batch_ids();
void test_pb_batch_sums();
void test_pb_batch_losses();
void test_pb_pruning_condition_result();
void test_pb_encrypted_statistics();
void test_pb_updated_info();
void test_pb_split_info();
int test_pb();

#endif //COLLABORATIVEML_TEST_PB_CONVERTER_H
