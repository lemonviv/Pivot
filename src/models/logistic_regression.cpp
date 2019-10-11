//
// Created by wuyuncheng on 11/10/19.
//

#include "logistic_regression.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iomanip>

LogisticRegression::LogisticRegression(){}


LogisticRegression::LogisticRegression(int param_batch_size, int param_max_iteration,
        float param_converge_threshold, int param_feature_num)
{
    // copy params
    batch_size = param_batch_size;
    max_iteration = param_max_iteration;
    converge_threshold = param_converge_threshold;
    feature_num = param_feature_num;
    model_accuracy = 0.0;

    // init local weights
    for (int j = 0; j < feature_num; ++j) {
        mpz_init(local_weights[j]);
    }
}


void LogisticRegression::init_encrypted_local_weights(djcs_public_key *public_key, hcs_random* hr)
{
    srand(static_cast<unsigned> (time(0)));

    long ex = (long) pow(10, FLOAT_PRECISION);

    for (int i = 0; i < feature_num; ++i) {

        // 1. generate random fixed point float values
        float fr = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);

        // 2. fixed point integer representation
        std::stringstream ss;
        ss << std::fixed << std::setprecision(FLOAT_PRECISION) << fr;
        std::string s = ss.str();
        long r = (long) (::atof(s.c_str()) * ex);
        mpz_t big_r;
        mpz_init(big_r);
        mpz_set_ui(big_r, r);

        // 3. encrypt with public_key
        djcs_encrypt(public_key, hr, local_weights[i], big_r);
    }
}


void LogisticRegression::partial_predict(djcs_public_key *public_key, hcs_random *hr,
        std::vector<long> instance, mpz_t res)
{
    instance_partial_sum(public_key, hr, instance, res);
}


void LogisticRegression::instance_partial_sum(djcs_public_key* public_key, hcs_random* hr,
        std::vector<long> instance, mpz_t res)
{
    djcs_encrypt(public_key, hr, res, res);

    // homomorphic dot product computation
    for (int i = 0; i < feature_num; i++) {

        mpz_t t1, t2;

        mpz_init(t1);
        mpz_init(t2);
        mpz_set_ui(t1, instance[i]);

        djcs_ep_mul(public_key, t2, local_weights[i], t1);
        //gmp_printf("%Zd\n",t);
        djcs_ee_add(public_key, res, res, t2);

        mpz_clear(t1);
        mpz_clear(t2);
    }
}


void LogisticRegression::aggregate_partial_sum_instance(djcs_public_key* public_key, hcs_random* hr,
        mpz_t *partial_sum, int client_num, mpz_t aggregated_sum)
{
    djcs_encrypt(public_key, hr, aggregated_sum, aggregated_sum);

    // homomorphic addition to aggregated_sum
    for (int i = 0; i < client_num; ++i) {
        djcs_ee_add(public_key, aggregated_sum, aggregated_sum, partial_sum[i]);
    }
}


LogisticRegression::~LogisticRegression()
{
    // free local weights
    for (int j = 0; j < feature_num; ++j) {
        mpz_clear(local_weights[j]);
    }
}


