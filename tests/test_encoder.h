//void test_increase_exponent_negative_float()
// Created by wuyuncheng on 16/10/19.
//

#ifndef COLLABORATIVEML_TEST_ENCODER_H
#define COLLABORATIVEML_TEST_ENCODER_H

void compute_thresholds();
void test_positive_int(int x);
void test_negative_int(int x);
void test_positive_float(float x);
void test_negative_float(float x);
void test_encoded_number_state();
void test_decrease_exponent_positive_int();
void test_decrease_exponent_negative_int();
void test_decrease_exponent_positive_float();
void test_decrease_exponent_negative_float();
void test_increase_exponent_positive_float();
void test_increase_exponent_negative_float();
void test_decode_with_truncation_float();
int test_encoder();

#endif //COLLABORATIVEML_TEST_ENCODER_H
