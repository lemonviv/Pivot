//
// Created by wuyuncheng on 11/10/19.
//

#include "logistic_regression.h"
#include "../utils/encoder.h"
#include "../utils/djcs_t_aux.h"
#include "../utils/pb_converter.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <random>

extern djcs_t_auth_server **au;


LogisticRegression::LogisticRegression(){}


LogisticRegression::LogisticRegression(
        int param_batch_size,
        int param_max_iteration,
        float param_converge_threshold,
        float param_alpha,
        int param_feature_num)
{
    // copy params
    batch_size = param_batch_size;
    max_iteration = param_max_iteration;
    converge_threshold = param_converge_threshold;
    feature_num = param_feature_num;
    model_accuracy = 0.0;
    alpha = param_alpha;
    local_weights = new EncodedNumber[feature_num];
}


void LogisticRegression::train(Client client) {

    logger(stdout, "****** Training begin ******\n");

    // compute n using client->m_pk
    mpz_t n, positive_threshold, negative_threshold;
    mpz_init(n);
    mpz_init(positive_threshold);
    mpz_init(negative_threshold);
    compute_thresholds(client.m_pk, n, positive_threshold, negative_threshold);

    // init encrypted local weights
    init_encrypted_local_weights(client.m_pk, client.m_hr);

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    if (client.client_id == 0) {
        for (int i = 0; i < client.sample_num; i++) {
            data_indexes.push_back(i);
        }
    }

    // training
    for (int iter = 0; iter < MAX_ITERATION; iter++) {

        // step 1: random select a batch with batch size and encode the batch samples
        int *batch_ids = new int[batch_size];
        if (client.client_id == 0) {
            auto rng = std::default_random_engine();
            std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
            for (int i = 0; i < batch_size; i++) {
                batch_ids[i] = data_indexes[i];
                //logger(stdout, "selected batch ids by client 0: %d\n", batch_ids[i]);
            }

            // send to the other clients
            for (int i = 0; i < client.client_num; i++) {
                if (i != client.client_id) {
                    std::string s;
                    serialize_batch_ids(batch_ids, batch_size, s);
                    client.send_long_messages(client.channels[i].get(), s);
                }
            }
        } else {
            std::string recv_s;
            client.recv_long_messages(client.channels[0].get(), recv_s);
            deserialize_ids_from_string(batch_ids, recv_s);

//            for (int i = 0; i < batch_size; i++) {
//                logger(stdout, "received batch ids from client 0: %d\n", batch_ids[i]);
//            }
        }

        EncodedNumber **batch_data = new EncodedNumber*[batch_size];
        for (int i = 0; i < batch_size; i++) {
            batch_data[i] = new EncodedNumber[feature_num];
        }

        // select the former batch_size data
        for (int i = 0; i < batch_size; i++) {
            for (int j = 0; j < feature_num; j++) {
                batch_data[i][j].set_float(n, client.local_data[batch_ids[i]][j]);
            }
        }

        //logger(stdout, "step 1 computed succeed\n");

        // step 2: every client locally compute partial sum and send to client 0
        EncodedNumber *batch_partial_sums = new EncodedNumber[batch_size];
        for (int i = 0; i < batch_size; i++) {
            instance_partial_sum(client.m_pk, client.m_hr, batch_data[i], batch_partial_sums[i]);
        }

        EncodedNumber **batch_partial_sums_array = new EncodedNumber*[batch_size];
        for (int i = 0; i < batch_size; i++) {
            batch_partial_sums_array[i] = new EncodedNumber[client.client_num];
        }
        if (client.client_id == 0) {
            for (int j = 0; j < client.client_num; j++) {
                if (j == 0) {
                    for (int i = 0; i < batch_size; i++) {
                        batch_partial_sums_array[i][j] = batch_partial_sums[i];
                    }
                } else {
                    // receive from the other clients
                    std::string recv_s;
                    EncodedNumber *recv_batch_partial_sums = new EncodedNumber[batch_size];
                    client.recv_long_messages(client.channels[j].get(), recv_s);
                    deserialize_sums_from_string(recv_batch_partial_sums, batch_size, recv_s);
                    for (int i = 0; i < batch_size; i++) {
                        batch_partial_sums_array[i][j] = recv_batch_partial_sums[i];
                    }

                    delete [] recv_batch_partial_sums;
                }
            }

        } else {
            std::string s;
            serialize_batch_sums(batch_partial_sums, batch_size, s);
            client.send_long_messages(client.channels[0].get(), s);
        }

        //logger(stdout, "step 2 computed succeed\n");

        // step 3: client 0 aggregate the sums and call share decrypt (call mpc when mpc is ready)
        EncodedNumber *batch_aggregated_sums = new EncodedNumber[batch_size];
        EncodedNumber *decrypted_batch_aggregated_sums = new EncodedNumber[batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < batch_size; i++) {
                aggregate_partial_sum_instance(client.m_pk, client.m_hr,
                                                     batch_partial_sums_array[i], batch_aggregated_sums[i], client.client_num);
            }

            // call share decrypt
            client.share_batch_decrypt(batch_aggregated_sums, decrypted_batch_aggregated_sums, batch_size);

        } else {
            std::string s, response_s;
            client.recv_long_messages(client.channels[0].get(), s);
            client.decrypt_batch_piece(s, response_s, 0);
        }

        //logger(stdout, "step 3 computed succeed\n");

        // step 4: client 0 compute the logistic function and encrypt the result (should truncation)
        int correct_num = 0;
        EncodedNumber *reencrypted_batch_sums = new EncodedNumber[batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < batch_size; i++) {
                float t;
                decrypted_batch_aggregated_sums[i].decode(t);
                t = 1.0 / (1 + exp(0 - t));
                int predict_label = t >= 0.5 ? 1 : 0;
                if (predict_label == client.labels[batch_ids[i]]) correct_num += 1;
                // truncate to FLOAT_PRECISION, same as the labels
                reencrypted_batch_sums[i].set_float(decrypted_batch_aggregated_sums[i].n, t);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, reencrypted_batch_sums[i], reencrypted_batch_sums[i]);
            }
        }

        // add temporary code for viewing the training accuracy, now is full training dataset sgd
        logger(stdout, "Iteration %d accuracy = %f\n", iter, (float) correct_num / (float) client.sample_num);
        //logger(stdout, "step 4 computed succeed\n");

        // step 5: client 0 compute the losses of the batch, and send to every other client
        EncodedNumber *encrypted_losses = new EncodedNumber[batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < batch_size; i++) {
                encrypted_losses[i].set_float(n, (float) (0 - client.labels[batch_ids[i]]));
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_losses[i], encrypted_losses[i]);
                djcs_t_aux_ee_add(client.m_pk, encrypted_losses[i], encrypted_losses[i], reencrypted_batch_sums[i]);
            }

            update_local_weights(client.m_pk, client.m_hr, batch_data, encrypted_losses, alpha);

            for (int j = 0; j < client.client_num; j++) {
                if (j != client.client_id){
                    std::string s;
                    serialize_batch_losses(encrypted_losses, batch_size, s);
                    client.send_long_messages(client.channels[j].get(), s);
                }
            }
        } else {
            std::string recv_s;
            EncodedNumber *recv_encrypted_losses = new EncodedNumber[batch_size];
            client.recv_long_messages(client.channels[0].get(), recv_s);
            deserialize_sums_from_string(recv_encrypted_losses, batch_size, recv_s);

            // step 6: every client update its local weights
            update_local_weights(client.m_pk, client.m_hr, batch_data, recv_encrypted_losses, alpha);

            delete [] recv_encrypted_losses;
        }

        //logger(stdout, "step 5 and step 6 computed succeed\n");

        // free temporary memories
        delete [] batch_ids;
        delete [] batch_partial_sums;
        delete [] batch_aggregated_sums;
        delete [] decrypted_batch_aggregated_sums;
        delete [] reencrypted_batch_sums;
        delete [] encrypted_losses;
        for (int i = 0; i < batch_size; i++) {
            delete [] batch_data[i];
            delete [] batch_partial_sums_array[i];
        }
        delete [] batch_data;
        delete [] batch_partial_sums_array;
    }

    // share decrypt the local weights
    EncodedNumber *decrypted_weights = new EncodedNumber[feature_num];

    for (int i = 0; i < client.client_num; i++) {
        if (i == client.client_id) {
            client.share_batch_decrypt(local_weights, decrypted_weights, feature_num);
            for (int j = 0; j < feature_num; j++) {
                float weight;
                decrypted_weights[j].decode(weight);
                logger(stdout, "The decrypted weight %d = %f\n", j, weight);
            }
        } else {
            // decrypt piece
            std::string recv_s, response_s;
            client.recv_long_messages(client.channels[i].get(), recv_s);
            client.decrypt_batch_piece(recv_s, response_s, i);
        }
    }

    mpz_clear(n);
    mpz_clear(positive_threshold);
    mpz_clear(negative_threshold);

    logger(stdout, "****** Training end ******\n");
}



void LogisticRegression::test(Client client, int *test_sample_ids, int size, float & accuracy) {

    logger(stdout, "****** Begin test process ******\n");

    // compute n using client->m_pk
    mpz_t n, positive_threshold, negative_threshold;
    mpz_init(n);
    mpz_init(positive_threshold);
    mpz_init(negative_threshold);
    compute_thresholds(client.m_pk, n, positive_threshold, negative_threshold);

    // test process is similar to the train process in one iteration

    // step 1: super client sends the test_sample_ids to the other clients
    int *sample_ids = new int[size];
    if (client.client_id == 0) {
        for (int i = 0; i < size; i++) {
            sample_ids[i] = test_sample_ids[i];
        }

        // send to the other clients
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string s;
                serialize_batch_ids(sample_ids, size, s);
                client.send_long_messages(client.channels[i].get(), s);
            }
        }
    } else {
        std::string recv_s;
        client.recv_long_messages(client.channels[0].get(), recv_s);
        deserialize_ids_from_string(sample_ids, recv_s);
    }

    EncodedNumber **test_data = new EncodedNumber*[size];
    for (int i = 0; i < size; i++) {
        test_data[i] = new EncodedNumber[feature_num];
    }

    // select the former batch_size data
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < feature_num; j++) {
            test_data[i][j].set_float(n, client.local_data[sample_ids[i]][j]);
        }
    }

    // step 2: every client compute partial sums and send back to super client
    EncodedNumber *partial_sums = new EncodedNumber[size];
    for (int i = 0; i < size; i++) {
        instance_partial_sum(client.m_pk, client.m_hr, test_data[i], partial_sums[i]);
    }

    EncodedNumber **partial_sums_array = new EncodedNumber*[size];
    for (int i = 0; i < size; i++) {
        partial_sums_array[i] = new EncodedNumber[client.client_num];
    }
    if (client.client_id == 0) {
        for (int j = 0; j < client.client_num; j++) {
            if (j == 0) {
                for (int i = 0; i < size; i++) {
                    partial_sums_array[i][j] = partial_sums[i];
                }
            } else {
                // receive from the other clients
                std::string recv_s;
                EncodedNumber *recv_partial_sums = new EncodedNumber[size];
                client.recv_long_messages(client.channels[j].get(), recv_s);
                deserialize_sums_from_string(recv_partial_sums, size, recv_s);
                for (int i = 0; i < size; i++) {
                    partial_sums_array[i][j] = recv_partial_sums[i];
                }

                delete [] recv_partial_sums;
            }
        }

    } else {
        std::string s;
        serialize_batch_sums(partial_sums, size, s);
        client.send_long_messages(client.channels[0].get(), s);
    }

    // step 3: super client aggregate the partial sums and call the share decrypt (now without mpc)
    EncodedNumber *aggregated_sums = new EncodedNumber[size];
    EncodedNumber *decrypted_aggregated_sums = new EncodedNumber[size];
    if (client.client_id == 0) {
        for (int i = 0; i < size; i++) {
            aggregate_partial_sum_instance(client.m_pk, client.m_hr,
                                           partial_sums_array[i], aggregated_sums[i], client.client_num);
        }

        // call share decrypt
        client.share_batch_decrypt(aggregated_sums, decrypted_aggregated_sums, size);

    } else {
        std::string s, response_s;
        client.recv_long_messages(client.channels[0].get(), s);
        client.decrypt_batch_piece(s, response_s, 0);
    }

    // step 4: super client compute the logistic function and compare to the labels, returning accuracy
    int correct_num = 0;
    EncodedNumber *reencrypted_sums = new EncodedNumber[size];
    if (client.client_id == 0) {
        for (int i = 0; i < size; i++) {
            float t;
            decrypted_aggregated_sums[i].decode(t);
            t = 1.0 / (1 + exp(0 - t));
            int predict_label = t >= 0.5 ? 1 : 0;
            if (predict_label == client.labels[sample_ids[i]]) correct_num += 1;
            // truncate to FLOAT_PRECISION, same as the labels
            reencrypted_sums[i].set_float(decrypted_aggregated_sums[i].n, t);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, reencrypted_sums[i], reencrypted_sums[i]);
        }
    }

    accuracy = (float) correct_num / size;
    logger(stdout, "The test accuracy is %f\n", accuracy);

    // free temporary memories
    delete [] sample_ids;
    delete [] partial_sums;
    delete [] aggregated_sums;
    delete [] decrypted_aggregated_sums;
    delete [] reencrypted_sums;
    for (int i = 0; i < size; i++) {
        delete [] test_data[i];
        delete [] partial_sums_array[i];
    }
    delete [] test_data;
    delete [] partial_sums_array;

    mpz_clear(n);
    mpz_clear(positive_threshold);
    mpz_clear(negative_threshold);

    logger(stdout, "****** End test process ******\n");
}


void LogisticRegression::init_encrypted_local_weights(djcs_t_public_key *pk, hcs_random* hr, int precision)
{
    srand(static_cast<unsigned> (time(0)));

    for (int i = 0; i < feature_num; ++i) {

        // 1. generate random fixed point float values
        float fr = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);

        logger(stdout, "initialized local weight %d value = %f\n", i, fr);

        // 2. compute public key size in encoded number
        mpz_t n;
        mpz_init(n);
        mpz_sub_ui(n, pk->g, 1);

        // 3. set for float value
        EncodedNumber tmp;
        tmp.set_float(n, fr, precision);

        // 4. encrypt with public_key (should use another EncodeNumber to represent cipher?)
        djcs_t_aux_encrypt(pk, hr, local_weights[i], tmp);

        mpz_clear(n);
    }
}


void LogisticRegression::partial_predict(
        djcs_t_public_key *pk,
        hcs_random *hr,
        EncodedNumber instance[],
        EncodedNumber & res)
{
    instance_partial_sum(pk, hr, instance, res);
}


void LogisticRegression::instance_partial_sum(
        djcs_t_public_key* pk,
        hcs_random* hr,
        EncodedNumber instance[],
        EncodedNumber & res)
{
    // homomorphic dot product computation
    djcs_t_aux_inner_product(pk, hr, res, local_weights, instance, feature_num);
}


void LogisticRegression::compute_batch_loss(
        djcs_t_public_key* pk,
        hcs_random* hr,
        EncodedNumber aggregated_res[],
        EncodedNumber labels[],
        EncodedNumber *& losses)
{
    // homomorphic addition
    for (int i = 0; i < batch_size; ++i) {
        // TODO: assume labels are already negative
        djcs_t_aux_encrypt(pk, hr, labels[i], labels[i]);
        djcs_t_aux_ee_add(pk, losses[i], aggregated_res[i], labels[i]);
    }
}


void LogisticRegression::aggregate_partial_sum_instance(
        djcs_t_public_key* pk,
        hcs_random* hr,
        EncodedNumber partial_sum[],
        EncodedNumber & aggregated_sum,
        int client_num,
        int desired_precision) {

    aggregated_sum.set_float(partial_sum[0].n, 0.0, 0 - partial_sum[0].exponent);
    djcs_t_aux_encrypt(pk, hr, aggregated_sum, aggregated_sum);
    for (int i = 0; i < client_num; ++i) {
        djcs_t_aux_ee_add(pk, aggregated_sum, aggregated_sum, partial_sum[i]);
    }

    // here should truncate the losses precision from 3 * FLOAT_PRECISION to PRECISION
    // currently using decryption for truncated to desired precision
    // will truncate in mpc when SCALE-MAMBA is applied
    // TODO: using mpc for non-linear function computation and truncation

//    EncodedNumber t;
//    decrypt_temp(pk, au, TOTAL_CLIENT_NUM, t, aggregated_sum);

    // compute the logistic function of aggregated sum \frac{1}{1 + e^{-t.value}}
//    float x;
//    t.decode(x);
//    logger(stdout, "decoded value before non-linear function = %f\n", x);
//    x = 1.0 / (1 + exp(0 - x));
//    aggregated_sum.set_float(aggregated_sum.n, x, desired_precision);
//    aggregated_sum.type = Plaintext;
//    djcs_t_aux_encrypt(pk, hr, aggregated_sum, aggregated_sum);

//    decrypt_temp(pk, au, TOTAL_CLIENT_NUM, t, aggregated_sum);
//    t.decode(x);
//    logger(stdout, "decoded value after non-linear function = %f\n", x);
}


void LogisticRegression::update_local_weights(
        djcs_t_public_key* pk,
        hcs_random* hr,
        EncodedNumber **batch_data,
        EncodedNumber losses[],
        float alpha,
        float lambda)
{
    // update client's local weights when receiving loss
    //     * according to L2 regularization (current do not consider L2 regularization because of scaling)
    //     * here [w_j] is 2 * FLOAT_PRECISION, but 2 * \lambda * [w_j] should be 3 * FLOAT_PRECISION
    //     * How to resolve this problem? Truncation seems not be possible because the exponent of [w_j]
    //     * will expand in every iteration
    //     * [w_j] := [w_j] - \alpha * \sum_{i=1}^{|B|}([losses[i]] * x_{ij})
    //     *
    //     * losses[i] should be truncated to PRECISION in compute batch loss function

    if (batch_size == 0) {
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
    for (int i = 0; i < batch_size; i++) {
        // for each column
        for (int j = 0; j < feature_num; j++) {

            mpz_mul(batch_data[i][j].value, batch_data[i][j].value, encoded_alpha->value);
            batch_data[i][j].exponent += encoded_alpha->exponent;

            //truncate the plaintext results to desired exponent, lose some precision here
            //(here truncate 2 * FLOAT_PRECISION to FLOAT_PRECISION)
            batch_data[i][j].increase_exponent(0 - FLOAT_PRECISION);
        }
    }

    // 3. homomorphic update the result -- update each local weight
    for (int j = 0; j < feature_num; j++) {
        // for each sample i in the batch, compute \sum_{i=1}^{|B|} [losses[i]] * batch_data[i][j]
        EncodedNumber sum;
        // TODO: here exponent set for correct addition
        sum.set_float(n, 0.0, -(losses[0].exponent + batch_data[0][0].exponent));
        djcs_t_aux_encrypt(pk, hr, sum, sum);
        for (int i = 0; i < batch_size; i++) {
            EncodedNumber tmp;
            tmp.set_integer(n, 0);
            djcs_t_aux_ep_mul(pk, tmp, losses[i], batch_data[i][j]);
            djcs_t_aux_ee_add(pk, sum, sum, tmp);
        }
        // update by [w_j] := [w_j] + \sum_{i=1}^{|B|} [losses[i]] * batch_data[i][j]
        // where batch_data[i][j] = - batch_data[i][j] * \alpha
        // TODO: the sum exponent here should be truncated equal to local_weights[j] before addition (using mpc)
        djcs_t_aux_ee_add(pk, local_weights[j], local_weights[j], sum);
    }

    mpz_clear(n);
}


LogisticRegression::~LogisticRegression() {
    // free local weights
    // delete [] local_weights;
}