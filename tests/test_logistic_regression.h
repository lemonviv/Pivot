//
// Created by wuyuncheng on 16/10/19.
//

#ifndef COLLABORATIVEML_TEST_LOGISTIC_REGRESSION_H
#define COLLABORATIVEML_TEST_LOGISTIC_REGRESSION_H

#include "../src/utils/encoder.h"
#include "../src/utils/util.h"
#include "../src/models/logistic_regression.h"
#include "gmp.h"
#include "libhcs.h"



int test_lr();
void test_init_weights(djcs_t_public_key *pk, hcs_random *hr);
void test_instance_partial_sum(djcs_t_public_key *pk, hcs_random *hr, int feature_num);
void test_partial_predict(djcs_t_public_key *pk, hcs_random *hr, int feature_num);
void test_aggregate_partial_sums(djcs_t_public_key *pk, hcs_random *hr, int client_num);
void test_compute_batch_losses(djcs_t_public_key *pk, hcs_random *hr, int batch_size);
void test_update_weights(djcs_t_public_key *pk, hcs_random *hr, int feature_num, int batch_size, float alpha);

#endif //COLLABORATIVEML_TEST_LOGISTIC_REGRESSION_H
