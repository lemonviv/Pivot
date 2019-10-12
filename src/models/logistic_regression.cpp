//
// Created by wuyuncheng on 11/10/19.
//

#include "logistic_regression.h"
#include "../utils/encoder.h"
#include "../utils/djcs_aux.h"
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
}


void LogisticRegression::init_encrypted_local_weights(djcs_public_key *public_key, hcs_random* hr)
{
    srand(static_cast<unsigned> (time(0)));

    for (int i = 0; i < feature_num; ++i) {

        // 1. generate random fixed point float values
        float fr = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);

        // 2. compute public key size in encoded number
        mpz_t n;
        mpz_init(n);
        mpz_sub_ui(n, public_key->g, 1);

        // 3. set for float value
        local_weights[i].set_float(n, fr, 2 * FLOAT_PRECISION);

        // 4. encrypt with public_key
        djcs_encrypt(public_key, hr, local_weights[i].value, local_weights[i].value);

        mpz_clear(n);
    }
}


void LogisticRegression::partial_predict(djcs_public_key *public_key, hcs_random *hr,
        std::vector<EncodedNumber> instance, EncodedNumber res)
{
    instance_partial_sum(public_key, hr, instance, res);
}


void LogisticRegression::instance_partial_sum(djcs_public_key* public_key, hcs_random* hr,
        std::vector<EncodedNumber> instance, EncodedNumber res)
{
    std::vector<EncodedNumber> weights;
    for (int i = 0; i < instance.size(); ++i) {
        weights.push_back(local_weights[i]);
    }

    // homomorphic dot product computation
    djcs_aux_inner_product(public_key, hr, res, instance, weights);
}


void LogisticRegression::compute_batch_loss(djcs_public_key* public_key, hcs_random* hr,
                                            std::vector<EncodedNumber> aggregated_res,
                                            std::vector<EncodedNumber> labels,
                                            std::vector<EncodedNumber> losses)
{
    // homomorphic addition
    for (int i = 0; i < labels.size(); ++i) {
        // TODO: assume labels are already negative
        djcs_aux_encrypt(public_key, hr, labels[i], labels[i]);
        djcs_aux_ee_add(public_key, losses[i], aggregated_res[i], labels[i]);
    }
}


void LogisticRegression::aggregate_partial_sum_instance(djcs_public_key* public_key, hcs_random* hr,
        std::vector<EncodedNumber> partial_sum, int client_num, EncodedNumber aggregated_sum)
{
    djcs_aux_encrypt(public_key, hr, aggregated_sum, aggregated_sum);

    // homomorphic addition to aggregated_sum
    for (int i = 0; i < client_num; ++i) {
        djcs_aux_ee_add(public_key, aggregated_sum, aggregated_sum, partial_sum[i]);
    }
}


void update_local_weights(djcs_public_key public_key, hcs_random* hr,
                          std::vector<EncodedNumber> batch_data,
                          std::vector<EncodedNumber> losses,
                          float alpha, float lambda)
{
    // update client's local weights when receiving loss
    //     * according to L2 regularization (current do not consider L2 regularization because of scaling)
    //     * [w_j] := [w_j] - \alpha * \sum_{i=1}^{|B|}([losses[i]] * x_{ij})
    //     * here [w_j] is 2 * FLOAT_PRECISION, but 2 * \lambda * [w_j] should be 3 * FLOAT_PRECISION
    //     * How to resolve this problem? Truncation seems not be possible because the exponent of [w_j]
    //     * will expand in every iteration

    // should be careful for the fixed point integer representation
    // TODO: add an exponent indicator to the encrypted value


}


LogisticRegression::~LogisticRegression()
{
    // free local weights
    for (int j = 0; j < feature_num; ++j) {
    }
}


