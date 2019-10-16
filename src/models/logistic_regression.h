//
// Created by wuyuncheng on 11/10/19.
//

#ifndef COLLABORATIVEML_LOGISTIC_REGRESSION_H
#define COLLABORATIVEML_LOGISTIC_REGRESSION_H

#include <vector>
#include "gmp.h"
#include "../client/Client.h"
#include "../utils/util.h"
#include "../utils/encoder.h"

#define MAX_FEATURE_NUM 100

class LogisticRegression {
private:
    int batch_size;                                    // batch size in stochastic gradient descent
    int max_iteration;                                 // maximum number of iteration for training
    float converge_threshold;                          // threshold of model accuracy convergence
    float model_accuracy;                              // accuracy of the model
    int feature_num;                                   // feature number of a client
    //std::vector<EncodedNumber> local_weights;        // local weights of features
    EncodedNumber *local_weights;                      // local weights of features
public:
    /**
     * default constructor
     */
    LogisticRegression();

    /**
     * constructor with parameters
     *
     * @param param_batch_size
     * @param param_max_iteration
     * @param param_converge_threshold
     * @param param_feature_num
     */
    LogisticRegression(int param_batch_size,
            int param_max_iteration,
            float param_converge_threshold,
            int param_feature_num);
    /**
     * (client who owns label is responsible for training)
     * train a lr model using local_data
     *
     * @param super_client
     */
    void train(Client super_client);

    /**
     * given an instance with features, compute the result using local_weights
     *
     * @param pk
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res
     */
    void partial_predict(djcs_t_public_key* pk, hcs_random* hr,
            EncodedNumber instance[],
            EncodedNumber res);

    /**
     * init encrypted local weights with feature_num values
     *
     * @param pk
     * @param hr
     */
    void init_encrypted_local_weights(djcs_t_public_key* pk, hcs_random* hr);

    /**
     * compute partial aggregation result for an instance using local_weights,
     * and store result to res
     *
     * @param pk
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res (with 0)
     */
    void instance_partial_sum(djcs_t_public_key* pk, hcs_random* hr,
            EncodedNumber instance[],
            EncodedNumber res);

    /**
     * (client who owns labels do this computation)
     * aggregate the other clients' partial results
     *
     * @param pk
     * @param hr
     * @param partial_sum
     * @param client_num
     * @param aggregated_sum
     */
    void aggregate_partial_sum_instance(djcs_t_public_key* pk, hcs_random* hr,
            EncodedNumber partial_sum[],
            int client_num,
            EncodedNumber aggregated_sum);

    /**
     * compute batch loss by conversion between mpc and back
     *
     * @param pk
     * @param hr
     * @param aggregated_res
     * @param labels : should be represented with fixed point integer, with FLOAT_PRECISION
     * @param losses : should be truncated from K * FLOAT_PRECISION to FLOAT_PRECISION exponent
     */
    void compute_batch_loss(djcs_t_public_key* pk, hcs_random* hr,
            EncodedNumber aggregated_res[],
            EncodedNumber labels[],
            EncodedNumber losses[]);

    /**
     * update client's local weights when receiving loss
     * [w_j] := [w_j] - \alpha * \sum_{i=1}^{|B|}([losses[i]] * x_{ij})
     *
     * @param pk
     * @param hr
     * @param batch_data
     * @param losses
     * @param alpha : learning rate
     * @param lambda : regularization term, currently set NULL because of the exponent scaling problem
     */
    void update_local_weights(djcs_t_public_key* pk, hcs_random* hr,
            EncodedNumber **batch_data,
            EncodedNumber losses[],
            float alpha,
            float lambda = NULL);

    /**
     * destructor
     */
    ~LogisticRegression();
};


#endif //COLLABORATIVEML_LOGISTIC_REGRESSION_H