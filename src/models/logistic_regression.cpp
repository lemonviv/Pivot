//
// Created by wuyuncheng on 11/10/19.
//

#include "logistic_regression.h"
#include "../utils/encoder.h"
#include "../utils/djcs_t_aux.h"
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


void LogisticRegression::init_encrypted_local_weights(djcs_t_public_key *pk, hcs_random* hr)
{
    srand(static_cast<unsigned> (time(0)));

    for (int i = 0; i < feature_num; ++i) {

        // 1. generate random fixed point float values
        float fr = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);

        // 2. compute public key size in encoded number
        mpz_t n;
        mpz_init(n);
        mpz_sub_ui(n, pk->g, 1);

        // 3. set for float value
        local_weights[i].set_float(n, fr, 2 * FLOAT_PRECISION);

        // 4. encrypt with public_key
        djcs_t_aux_encrypt(pk, hr, local_weights[i], local_weights[i]);

        mpz_clear(n);
    }
}


void LogisticRegression::partial_predict(djcs_t_public_key *pk, hcs_random *hr,
        std::vector<EncodedNumber> instance, EncodedNumber res)
{
    instance_partial_sum(pk, hr, instance, res);
}


void LogisticRegression::instance_partial_sum(djcs_t_public_key* pk, hcs_random* hr,
        std::vector<EncodedNumber> instance, EncodedNumber res)
{
    std::vector<EncodedNumber> weights;
    for (int i = 0; i < instance.size(); ++i) {
        weights.push_back(local_weights[i]);
    }

    // homomorphic dot product computation
    djcs_t_aux_inner_product(pk, hr, res, instance, weights);
}


void LogisticRegression::compute_batch_loss(djcs_t_public_key* pk, hcs_random* hr,
                                            std::vector<EncodedNumber> aggregated_res,
                                            std::vector<EncodedNumber> labels,
                                            std::vector<EncodedNumber> losses)
{
    // homomorphic addition
    for (int i = 0; i < labels.size(); ++i) {
        // TODO: assume labels are already negative
        djcs_t_aux_encrypt(pk, hr, labels[i], labels[i]);
        djcs_t_aux_ee_add(pk, losses[i], aggregated_res[i], labels[i]);
    }
}


void LogisticRegression::aggregate_partial_sum_instance(djcs_t_public_key* pk, hcs_random* hr,
        std::vector<EncodedNumber> partial_sum, int client_num, EncodedNumber aggregated_sum)
{
    djcs_t_aux_encrypt(pk, hr, aggregated_sum, aggregated_sum);

    // homomorphic addition to aggregated_sum
    for (int i = 0; i < client_num; ++i) {
        djcs_t_aux_ee_add(pk, aggregated_sum, aggregated_sum, partial_sum[i]);
    }
}


void LogisticRegression::update_local_weights(djcs_t_public_key* pk, hcs_random* hr,
                          std::vector< std::vector<EncodedNumber> > batch_data,
                          std::vector<EncodedNumber> losses,
                          float alpha, float lambda)
{
    // update client's local weights when receiving loss
    //     * according to L2 regularization (current do not consider L2 regularization because of scaling)
    //     * [w_j] := [w_j] - \alpha * \sum_{i=1}^{|B|}([losses[i]] * x_{ij})
    //     * here [w_j] is 2 * FLOAT_PRECISION, but 2 * \lambda * [w_j] should be 3 * FLOAT_PRECISION
    //     * How to resolve this problem? Truncation seems not be possible because the exponent of [w_j]
    //     * will expand in every iteration

    if (losses.size() != batch_data.size()) {
        logger(stdout, "the losses size %d not equal to batch data size %d\n",
                losses.size(), batch_data.size());
        return;
    }

    if (losses.size() == 0) {
        logger(stdout, "no update needed\n");
        return;
    }

    // 1. represent -alpha with EncodeNumber
    mpz_t n;
    mpz_init(n);
    mpz_set(n, losses[0].n);
    EncodedNumber *encoded_alpha = new EncodedNumber();
    encoded_alpha->set_float(n, 0 - alpha, FLOAT_PRECISION);

    // 2. multiply -alpha with batch_data
    // for each sample
    for (int i = 0; i < batch_data.size(); i++) {
        // for each column
        for (int j = 0; j < batch_data[0].size(); j++) {

            mpz_mul(batch_data[i][j].value, batch_data[i][j].value, encoded_alpha->value);
            batch_data[i][j].exponent += encoded_alpha->exponent;

            //truncate the plaintext results to desired exponent, lose some precision here
            //(here truncate 2 * FLOAT_PRECISION to FLOAT_PRECISION)
            batch_data[i][j].increase_exponent(0 - FLOAT_PRECISION);
        }
    }

    // 3. homomorphic update the result
    // update each local weight
    for (int j = 0; j < feature_num; j++) {
        // for each sample i in the batch, compute \sum_{i=1}^{|B|} [losses[i]] * batch_data[i][j]
        EncodedNumber sum;
        sum.set_integer(n, 0);
        djcs_t_aux_encrypt(pk, hr, sum, sum);
        for (int i = 0; i < batch_data.size(); i++) {
            EncodedNumber tmp;
            tmp.set_integer(n, 0);
            djcs_t_aux_ep_mul(pk, tmp, losses[i], batch_data[i][j]);
            djcs_t_aux_ee_add(pk, sum, sum, tmp);
        }
        // update by [w_j] := [w_j] + \sum_{i=1}^{|B|} [losses[i]] * batch_data[i][j]
        // where batch_data[i][j] = - batch_data[i][j] * \alpha
        djcs_t_aux_ee_add(pk, local_weights[j], local_weights[j], sum);
    }

    mpz_clear(n);

}


LogisticRegression::~LogisticRegression()
{
    // free local weights
    for (int j = 0; j < feature_num; ++j) {
    }
}


