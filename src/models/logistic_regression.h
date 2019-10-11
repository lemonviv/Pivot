//
// Created by wuyuncheng on 11/10/19.
//

#ifndef COLLABORATIVEML_LOGISTIC_REGRESSION_H
#define COLLABORATIVEML_LOGISTIC_REGRESSION_H

#include <vector>
#include "../utils/util.h"
#include "gmp.h"
#include "../client/Client.h"

#define MAX_FEATURE_NUM 100
#define FLOAT_PRECISION 8

class LogisticRegression {
private:
    int batch_size;                                    // batch size in stochastic gradient descent
    int max_iteration;                                 // maximum number of iteration for training
    float converge_threshold;                          // threshold of model accuracy convergence
    float model_accuracy;                              // accuracy of the model
    int feature_num;                                   // feature number of a client
    mpz_t local_weights[MAX_FEATURE_NUM];              // local weights of features
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
    LogisticRegression(int param_batch_size, int param_max_iteration,
            float param_converge_threshold, int param_feature_num);
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
     * @param public_key
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res
     */
    void partial_predict(djcs_public_key* public_key, hcs_random* hr,
            std::vector<long> instance, mpz_t res);

    /**
     * init encrypted local weights with feature_num values
     *
     * @param public_key
     * @param hr
     */
    void init_encrypted_local_weights(djcs_public_key* public_key, hcs_random* hr);

    /**
     * compute partial aggregation result for an instance using local_weights,
     * and store result to res
     *
     * @param public_key
     * @param hr
     * @param instance (fixed point integer representation)
     * @param res (with 0)
     */
    void instance_partial_sum(djcs_public_key* public_key, hcs_random* hr,
            std::vector<long> instance, mpz_t res);

    /**
     * (client who owns labels do this computation)
     * aggregate the other clients' partial results
     *
     * @param public_key
     * @param hr
     * @param partial_sum
     * @param client_num
     * @param aggregated_sum
     */
    void aggregate_partial_sum_instance(djcs_public_key* public_key, hcs_random* hr,
            mpz_t *partial_sum, int client_num, mpz_t aggregated_sum);

    /**
     * compute batch loss by conversion between mpc and back
     *
     * @param aggregated_res
     * @param labels
     */
    void compute_batch_loss(mpz_t *aggregated_res, mpz_t *labels);

    /**
     * update client's local weights when receiving loss
     *
     * @param batch_data
     * @param loss
     */
    void update_local_weights(std::vector<float> batch_data, mpz_t *loss);

    /**
     * destructor
     */
    ~LogisticRegression();
};




#endif //COLLABORATIVEML_LOGISTIC_REGRESSION_H
