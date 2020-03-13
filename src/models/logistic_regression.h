//
// Created by wuyuncheng on 11/10/19.
//

#ifndef PIVOT_LOGISTIC_REGRESSION_H
#define PIVOT_LOGISTIC_REGRESSION_H

#include <vector>
#include "gmp.h"
#include "../client/Client.h"
#include "../utils/util.h"
#include "../utils/encoder.h"

#define MAX_FEATURE_NUM 100
#define BATCH_SIZE 10
#define MAX_ITERATION 50
#define CONVERGENCE_THRESHOLD 1e-6
#define ALPHA 0.2
#define NUM_SPDZ_PARTIES 3
#define SPDZ_PORT_BASE 14000

class LogisticRegression {
public:
    int batch_size;                                    // batch size in stochastic gradient descent
    int max_iteration;                                 // maximum number of iteration for training
    float converge_threshold;                          // threshold of model accuracy convergence
    float alpha;                                       // learning rate
    float model_accuracy;                              // accuracy of the model
    int feature_num;                                   // feature number of a client
    //std::vector<EncodedNumber> local_weights;        // local weights of features
    EncodedNumber *local_weights;                      // local weights of features
    std::vector< std::vector<float> > training_data;   // training dataset
    std::vector< std::vector<float> > testing_data;    // training dataset
    std::vector<int> training_data_labels;             // labels of training dataset
    std::vector<int> testing_data_labels;             // labels of testing dataset
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
     * @param alpha
     * @param param_feature_num
     */
    LogisticRegression(
            int param_batch_size,
            int param_max_iteration,
            float param_converge_threshold,
            float alpha,
            int param_feature_num);
    /**
     * distributed train a lr model using local_data
     *
     * @param client
     */
    void train(Client client);


    /**
     * init training data and test data according to split fraction
     * call by client 0
     *
     * @param client
     * @param split
     */
    void init_datasets(Client client, float split);


    /**
     * init training data and test data according to new indexes received
     *
     * @param client
     * @param new_indexes
     * @param split
     */
    void init_datasets_with_indexes(Client client, int new_indexes[], float split);


    /**
     * test the model accuracy
     *
     * @param client
     * @param type: 0 train data, 1 test data
     * @param accuracy
     */
    void test(Client client, int type, float & accuracy);

    /**
     * given an instance with features, compute the result using local_weights
     *
     * @param pk
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res
     */
    void partial_predict(
            djcs_t_public_key* pk,
            hcs_random* hr,
            EncodedNumber instance[],
            EncodedNumber & res);

    /**
     * init encrypted local weights with feature_num values
     *
     * @param pk
     * @param hr
     * @param precision
     */
    void init_encrypted_local_weights(
            djcs_t_public_key* pk,
            hcs_random* hr,
            int precision = 2 * FLOAT_PRECISION);

    /**
     * compute partial aggregation result for an instance using local_weights,
     * and store result to res
     *
     * @param pk
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res (with 0)
     */
    void instance_partial_sum(
            djcs_t_public_key* pk,
            hcs_random* hr,
            EncodedNumber instance[],
            EncodedNumber & res);

    /**
     * (client who owns labels do this computation)
     * aggregate the other clients' partial results
     *
     * @param pk
     * @param hr
     * @param partial_sum
     * @param aggregated_sum
     * @param client_num
     * @param desired_precision
     * */
    void aggregate_partial_sum_instance(
            djcs_t_public_key* pk,
            hcs_random* hr,
            EncodedNumber partial_sum[],
            EncodedNumber & aggregated_sum,
            int client_num,
            int desired_precision = FLOAT_PRECISION);

    /**
     * compute batch loss by conversion between mpc and back
     *
     * @param pk
     * @param hr
     * @param aggregated_res
     * @param labels : should be represented with fixed point integer, with FLOAT_PRECISION
     * @param losses : should be truncated from K * FLOAT_PRECISION to FLOAT_PRECISION exponent
     */
    void compute_batch_loss(
            djcs_t_public_key* pk,
            hcs_random* hr,
            EncodedNumber aggregated_res[],
            EncodedNumber labels[],
            EncodedNumber *& losses);

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
    void update_local_weights(
            djcs_t_public_key* pk,
            hcs_random* hr,
            EncodedNumber **batch_data,
            EncodedNumber losses[],
            float alpha,
            float lambda = NULL);

    /**
     * destructor
     */
    ~LogisticRegression();
};


#endif //PIVOT_LOGISTIC_REGRESSION_H