#include <iostream>
#include <string>
#include <random>
#include <ios>
#include <fstream>
#include "src/utils/util.h"
#include "src/utils/encoder.h"
#include "src/utils/djcs_t_aux.h"
#include "src/utils/pb_converter.h"
#include "src/models/logistic_regression.h"
#include "src/client/Client.h"
#include "src/models/cart_tree.h"
#include "src/models/feature.h"
#include "src/models/tree_node.h"
#include "src/models/random_forest.h"
#include "src/models/gbdt.h"

#include "tests/test_encoder.h"
#include "tests/test_djcs_t_aux.h"
#include "tests/test_logistic_regression.h"
#include "tests/test_pb_converter.h"

hcs_random *hr;
djcs_t_public_key *pk;
djcs_t_private_key *vk;
mpz_t n, positive_threshold, negative_threshold;
djcs_t_auth_server **au = (djcs_t_auth_server **)malloc(TOTAL_CLIENT_NUM * sizeof(djcs_t_auth_server *));
mpz_t *si = (mpz_t *)malloc(TOTAL_CLIENT_NUM * sizeof(mpz_t));

FILE * logger_out;

void system_setup() {
    hr = hcs_init_random();
    pk = djcs_t_init_public_key();
    vk = djcs_t_init_private_key();

    djcs_t_generate_key_pair(pk, vk, hr, 1, 1024, TOTAL_CLIENT_NUM, TOTAL_CLIENT_NUM);
    hr = hcs_init_random();
    mpz_t *coeff = djcs_t_init_polynomial(vk, hr);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_init(si[i]);
        djcs_t_compute_polynomial(vk, coeff, si[i], i);
        au[i] = djcs_t_init_auth_server();
        djcs_t_set_auth_server(au[i], si[i], i);
    }
    mpz_init(n);
    mpz_init(positive_threshold);
    mpz_init(negative_threshold);
    compute_thresholds(pk, n, positive_threshold, negative_threshold);

    djcs_t_free_polynomial(vk, coeff);
}

void system_free() {

    // free memory
    hcs_free_random(hr);
    djcs_t_free_public_key(pk);
    djcs_t_free_private_key(vk);

    mpz_clear(n);
    mpz_clear(positive_threshold);
    mpz_clear(negative_threshold);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_clear(si[i]);
        djcs_t_free_auth_server(au[i]);
    }

    free(si);
    free(au);
}

void test_share_decrypt(Client client) {

    for (int i = 0; i < client.client_num; i++) {
        if (i != client.client_id){
            std::string send_message = "Hello world from client " + std::to_string(client.client_id);
            int size = send_message.size();
            std::string recv_message;
            byte buffer[100];
            client.send_messages(i, send_message);
            client.recv_messages(i, recv_message, buffer, size);

            string longMessage = "Hi, this is a long message to test the writeWithSize approach";
            client.channels[i]->writeWithSize(longMessage);

            vector<byte> resMsg;
            client.channels[i]->readWithSizeIntoVector(resMsg);
            const byte * uc = &(resMsg[0]);
            string resMsgStr(reinterpret_cast<char const*>(uc), resMsg.size());
            string eq = (resMsgStr == longMessage)? "yes" : "no";
            cout << "Got long message: " << resMsgStr << ".\nequal? " << eq << "!" << endl;
        }
    }

    if (client.client_id == 0) {
        // client with label
        EncodedNumber a;
        a.set_float(n, 0.123456);
        //djcs_t_aux_encrypt(pk, hr, a, a);
        a.print_encoded_number();

        for (int i = 1; i < client.client_num; i++) {
            std::string s;
            serialize_encoded_number(a, s);
            //std::string s1 = "hello world" + std::to_string(i);
            client.channels[i]->writeWithSize(s);
            //client.send_messages(client.channels[i], s);
            //logger(logger_out, "send message: %s\n", s.c_str());
        }
    } else {
        vector<byte> resMsg;
        client.channels[0]->readWithSizeIntoVector(resMsg);
        const byte * uc = &(resMsg[0]);
        std::string s(reinterpret_cast<char const*>(uc), resMsg.size());
        EncodedNumber t, tt;
        deserialize_number_from_string(t, s);
        t.print_encoded_number();
        //client.recv_messages(client.channels[0], s, buffer, 999);
        //logger(logger_out, "receive message: %s\n", s.c_str());
    }


    // test share decrypt
    if (client.client_id == 0) {

        EncodedNumber *ciphers = new EncodedNumber[2];
        ciphers[0].set_float(n, 0.123456);
        ciphers[1].set_float(n, 0.654321);

        djcs_t_aux_encrypt(client.m_pk, client.m_hr, ciphers[0], ciphers[0]);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, ciphers[1], ciphers[1]);

        ciphers[0].print_encoded_number();

        // decrypt using global key
        EncodedNumber dec_t0, dec_t1;
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, dec_t0, ciphers[0]);
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, dec_t1, ciphers[1]);
        float x0, x1;
        dec_t0.decode(x0);
        dec_t1.decode(x1);
        logger(logger_out, "The decrypted value of ciphers[0] using global keys = %f\n", x0);
        logger(logger_out, "The decrypted value of ciphers[1] using global keys = %f\n", x1);

        EncodedNumber *share_decrypted_res = new EncodedNumber[2];
        client.share_batch_decrypt(ciphers, share_decrypted_res, 2);

        float share_x0, share_x1;
        share_decrypted_res[0].decode(share_x0);
        share_decrypted_res[1].decode(share_x1);
        logger(logger_out, "The decrypted value of ciphers[0] using share decryption = %f\n", share_x0);
        logger(logger_out, "The decrypted value of ciphers[1] using share decryption = %f\n", share_x1);
        delete [] share_decrypted_res;
    } else {

        std::string s, response_s;
        client.recv_long_messages(0, s);
        client.decrypt_batch_piece(s, response_s, 0);
    }
}

void logistic_regression(Client client) {

    LogisticRegression model(BATCH_SIZE, MAX_ITERATION, CONVERGENCE_THRESHOLD, ALPHA, client.feature_num);

    logger(logger_out, "init finished\n");

    float split = 0.8;
    if (client.client_id == 0) {
        model.init_datasets(client, split);
    } else {
        int *new_indexes = new int[client.sample_num];
        std::string recv_s;
        client.recv_long_messages(0, recv_s);
        deserialize_ids_from_string(new_indexes, recv_s);
        model.init_datasets_with_indexes(client, new_indexes, split);

        delete [] new_indexes;
    }

    model.train(client);

    int test_size = client.sample_num * (1 - SPLIT_PERCENTAGE);
    int *sample_ids = new int[test_size];
    for (int i = 0; i < test_size; i++) {
        sample_ids[i] = client.sample_num - test_size + i;
    }

    float accuracy = 0.0;
    model.test(client, 1, accuracy);
    logger(logger_out, "Testing accuracy = %f \n", accuracy);

    delete [] sample_ids;
}

float decision_tree(Client & client, int solution_type, int optimization_type, int class_num, int tree_type,
                    int max_bins, int max_depth, int num_trees) {

    logger(logger_out, "Begin decision tree training\n");

    struct timeval decision_tree_training_1, decision_tree_training_2;
    double decision_tree_training_time = 0;
    gettimeofday(&decision_tree_training_1, NULL);

    int m_global_feature_num = GLOBAL_FEATURE_NUM;
    int m_local_feature_num = client.local_data[0].size();
    int m_internal_node_num = 0;
    int m_type = tree_type;
    int m_classes_num = class_num;
    if (m_type == 1) {m_classes_num = 2;}
    int m_max_depth = max_depth;
    int m_max_bins = max_bins;
    int m_prune_sample_num = PRUNE_SAMPLE_NUM;
    float m_prune_threshold = PRUNE_VARIANCE_THRESHOLD;
    int m_solution_type = solution_type;
    int m_optimization_type = optimization_type;

    DecisionTree model(m_global_feature_num, m_local_feature_num, m_internal_node_num, m_type, m_classes_num,
            m_max_depth, m_max_bins, m_prune_sample_num, m_prune_threshold, m_solution_type, m_optimization_type);

    logger(logger_out, "Init decision tree model succeed\n");

    float split = SPLIT_PERCENTAGE;
    if (client.client_id == 0) {
        model.init_datasets(client, split);
        //model.test_indicator_vector_correctness();
    } else {
        int *new_indexes = new int[client.sample_num];
        std::string recv_s;
        client.recv_long_messages(0, recv_s);
        deserialize_ids_from_string(new_indexes, recv_s);
        model.init_datasets_with_indexes(client, new_indexes, split);
        delete [] new_indexes;
    }

    logger(logger_out, "Training data size = %d\n", model.training_data.size());
    model.init_features();
    model.init_root_node(client);
    model.build_tree_node(client, 0);

    logger(logger_out, "End decision tree training\n");
    logger(logger_out, "The internal node number is %d\n", model.internal_node_num);

    gettimeofday(&decision_tree_training_2, NULL);
    decision_tree_training_time += (double)((decision_tree_training_2.tv_sec - decision_tree_training_1.tv_sec) * 1000 + (double)(decision_tree_training_2.tv_usec - decision_tree_training_1.tv_usec) / 1000);
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "******** Decision tree training time: %'.3f ms **********\n", decision_tree_training_time);
    logger(logger_out, "*********************************************************************\n");


    struct timeval decision_tree_prediction_1, decision_tree_prediction_2;
    double decision_tree_prediction_average_time = 0;
    gettimeofday(&decision_tree_prediction_1, NULL);

    float accuracy = 0.0;
    model.test_accuracy(client, accuracy);

    gettimeofday(&decision_tree_prediction_2, NULL);
    decision_tree_prediction_average_time += (double)((decision_tree_prediction_2.tv_sec - decision_tree_prediction_1.tv_sec) * 1000 +
            (double)(decision_tree_prediction_2.tv_usec - decision_tree_prediction_1.tv_usec) / 1000);
    decision_tree_prediction_average_time = decision_tree_prediction_average_time / (double) model.testing_data.size();
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "********* Average decision tree prediction time: %'.3f ms ************\n", decision_tree_prediction_average_time);
    logger(logger_out, "*********************************************************************\n");

    if (client.client_id == 0) {
        logger(logger_out, "Accuracy = %f\n", accuracy);
        std::string result_log_file = LOGGER_HOME;
        result_log_file += "result.log";
        std::ofstream result_log(result_log_file, std::ios_base::app | std::ios_base::out);
        result_log << accuracy << std::endl;
    }
    return accuracy;
}

float random_forest(Client & client, int solution_type, int optimization_type, int class_num, int tree_type,
                    int max_bins, int max_depth, int num_trees) {
    logger(logger_out, "Begin random forest training\n");

    struct timeval random_forest_training_1, random_forest_training_2;
    double random_forest_training_time = 0;
    gettimeofday(&random_forest_training_1, NULL);

    int m_tree_num = num_trees;
    int m_global_feature_num = GLOBAL_FEATURE_NUM;
    int m_local_feature_num = client.local_data[0].size();
    int m_internal_node_num = 0;
    int m_type = tree_type;
    int m_classes_num = class_num;
    if (m_type == 1) m_classes_num = 2;
    int m_max_depth = max_depth;
    int m_max_bins = max_bins;
    int m_prune_sample_num = PRUNE_SAMPLE_NUM;
    float m_prune_threshold = PRUNE_VARIANCE_THRESHOLD;

    RandomForest model(m_tree_num, m_global_feature_num, m_local_feature_num, m_internal_node_num, m_type, m_classes_num,
                           m_max_depth, m_max_bins, m_prune_sample_num, m_prune_threshold, solution_type, optimization_type);

    // split datasets to training part and testing part
    float split = SPLIT_PERCENTAGE;
    if (client.client_id == 0) {
        model.init_datasets(client, split);
    } else {
        int *new_indexes = new int[client.sample_num];
        std::string recv_s;
        client.recv_long_messages(0, recv_s);
        deserialize_ids_from_string(new_indexes, recv_s);
        model.init_datasets_with_indexes(client, new_indexes, split);
        delete [] new_indexes;
    }

    float sample_rate = RF_SAMPLE_RATE;
    model.build_forest(client, sample_rate);

    gettimeofday(&random_forest_training_2, NULL);
    random_forest_training_time += (double)((random_forest_training_2.tv_sec - random_forest_training_1.tv_sec) * 1000 +
            (double)(random_forest_training_2.tv_usec - random_forest_training_1.tv_usec) / 1000);
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "******** Random forest training time: %'.3f ms **********\n", random_forest_training_time);
    logger(logger_out, "*********************************************************************\n");

    struct timeval random_forest_prediction_1, random_forest_prediction_2;
    double random_forest_prediction_average_time = 0;
    gettimeofday(&random_forest_prediction_1, NULL);

    float accuracy = 0.0;
    model.test_accuracy(client, accuracy);

    gettimeofday(&random_forest_prediction_2, NULL);
    random_forest_prediction_average_time += (double)((random_forest_prediction_2.tv_sec - random_forest_prediction_1.tv_sec) * 1000 +
            (double)(random_forest_prediction_2.tv_usec - random_forest_prediction_1.tv_usec) / 1000);
    random_forest_prediction_average_time = random_forest_prediction_average_time / (double) model.testing_data.size();
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "********* Average random forest prediction time: %'.3f ms ************\n", random_forest_prediction_average_time);
    logger(logger_out, "*********************************************************************\n");


    if (client.client_id == 0) {
        logger(logger_out, "Accuracy = %f\n", accuracy);
        std::string result_log_file = LOGGER_HOME;
        result_log_file += "result.log";
        std::ofstream result_log(result_log_file, std::ios_base::app | std::ios_base::out);
        result_log << accuracy << std::endl;
    }
    return accuracy;
}

float gbdt(Client & client, int solution_type, int optimization_type, int class_num, int tree_type,
           int max_bins, int max_depth, int num_trees) {

    logger(logger_out, "Begin GBDT training\n");

    struct timeval gbdt_training_1, gbdt_training_2;
    double gbdt_training_time = 0;
    gettimeofday(&gbdt_training_1, NULL);

    int m_tree_num = num_trees;
    int m_global_feature_num = GLOBAL_FEATURE_NUM;
    int m_local_feature_num = client.local_data[0].size();
    int m_internal_node_num = 0;
    int m_type = tree_type;
    int m_classes_num = class_num;
    if (m_type == 1) m_classes_num = 2;
    int m_max_depth = max_depth;
    int m_max_bins = max_bins;
    int m_prune_sample_num = PRUNE_SAMPLE_NUM;
    float m_prune_threshold = PRUNE_VARIANCE_THRESHOLD;

    GBDT model(m_tree_num, m_global_feature_num, m_local_feature_num, m_internal_node_num, m_type, m_classes_num,
                       m_max_depth, m_max_bins, m_prune_sample_num, m_prune_threshold, solution_type, optimization_type);

    logger(logger_out, "Correct init gbdt\n");

    float split = SPLIT_PERCENTAGE;
    if (client.client_id == 0) {
        model.init_datasets(client, split);
        //model.test_indicator_vector_correctness();
    } else {
        int *new_indexes = new int[client.sample_num];
        std::string recv_s;
        client.recv_long_messages(0, recv_s);
        deserialize_ids_from_string(new_indexes, recv_s);
        model.init_datasets_with_indexes(client, new_indexes, split);
        delete [] new_indexes;
    }

    model.build_gbdt_with_spdz(client);

    gettimeofday(&gbdt_training_2, NULL);
    gbdt_training_time += (double)((gbdt_training_2.tv_sec - gbdt_training_1.tv_sec) * 1000 +
                                            (double)(gbdt_training_2.tv_usec - gbdt_training_1.tv_usec) / 1000);
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "******** GBDT training time: %'.3f ms **********\n", gbdt_training_time);
    logger(logger_out, "*********************************************************************\n");

    struct timeval gbdt_prediction_1, gbdt_prediction_2;
    double gbdt_prediction_average_time = 0;
    gettimeofday(&gbdt_prediction_1, NULL);

    float accuracy = 0.0;
    model.test_accuracy(client, accuracy);

    gettimeofday(&gbdt_prediction_2, NULL);
    gbdt_prediction_average_time += (double)((gbdt_prediction_2.tv_sec - gbdt_prediction_1.tv_sec) * 1000 +
                                                      (double)(gbdt_prediction_2.tv_usec - gbdt_prediction_1.tv_usec) / 1000);
    gbdt_prediction_average_time = gbdt_prediction_average_time / (double) model.testing_data.size();
    logger(logger_out, "*********************************************************************\n");
    logger(logger_out, "********* Average GBDT prediction time: %'.3f ms ************\n", gbdt_prediction_average_time);
    logger(logger_out, "*********************************************************************\n");

    //model.test_accuracy_with_spdz(client, accuracy);
    if (client.client_id == 0) {
        logger(logger_out, "Accuracy = %f\n", accuracy);
        std::string result_log_file = LOGGER_HOME;
        result_log_file += "result.log";
        std::ofstream result_log(result_log_file, std::ios_base::app | std::ios_base::out);
        result_log << accuracy << std::endl;
    }
    return accuracy;
}


int main(int argc, char *argv[]) {

    int client_num = TOTAL_CLIENT_NUM;
    int client_id;
    if (argv[1] != NULL) {
        client_id = atoi(argv[1]);
    }
    bool has_label = (client_id == 0);
    std::string network_file = DEFAULT_NETWORK_FILE;
    std::string s1 = DEFAULT_DATA_FILE_PREFIX;

    int solution_type = 0;  // type = 0, basic solution; type = 1, enhanced solution
    int optimization_type = 0;  // type = 0, no optimization; type = 1, combining splits; type = 2, parallelism; type = 3, full optimization
    int class_num = DEFAULT_CLASSES_NUM;
    int algorithm_type = 0;  // 0 for decision tree; 1 for random forest; 2 for gbdt
    int tree_type = TREE_TYPE;   // 0 for classification tree; 1 for regression tree
    int max_bins = MAX_BINS;
    int max_depth = MAX_DEPTH;
    int num_trees = NUM_TREES;

    if (argc > 2) {
        if (argv[2] != NULL) {
            client_num = atoi(argv[2]);
        }
    }
    if (argc > 3) {
        if (argv[3] != NULL) {
            class_num = atoi(argv[3]);
        }
    }
    if (argc > 4) {
        if (argv[4] != NULL) {
            algorithm_type = atoi(argv[4]);
        }
    }
    if (argc > 5) {
        if (argv[5] != NULL) {
            tree_type = atoi(argv[5]);
        }
    }
    if (argc > 6) {
        if (argv[6] != NULL) {
            solution_type = atoi(argv[6]);
        }
    }
    if (argc > 7) {
        if (argv[7] != NULL) {
            optimization_type = atoi(argv[7]);
        }
    }
    if (argc > 8) {
        if (argv[8] != NULL) {
            network_file = argv[8];
        }
    }

    std::string dataset = "datasets"; // default dataset
    if (argc > 9) {
        if (argv[9] != NULL) {
            dataset = argv[9];
        }
    }
    s1 += dataset;

    std::string s2 = std::to_string(client_id);
    std::string data_file = s1 + "/client_" + s2 + ".txt";

    if (argc > 10) {
        if (argv[10] != NULL) {
            max_bins = atoi(argv[10]);
        }
    }

    if (argc > 11) {
        if (argv[11] != NULL) {
            max_depth = atoi(argv[11]);
        }
    }

    if (argc > 12) {
        if (argv[12] != NULL) {
            num_trees = atoi(argv[12]);
        }
    }

    //test_pb();

    if (client_id == 0) {
        system_setup();
    }

    // create logger file
    std::string logger_file_name = LOGGER_HOME;
    std::string alg_name;
    switch (algorithm_type) {
        case 1:
            alg_name = "RF";
            break;
        case 2:
            alg_name = "GBDT";
            break;
        default:
            alg_name = "DT";
            break;
    }
    logger_file_name += dataset;
    logger_file_name += "_";
    logger_file_name += alg_name;
    logger_file_name += "_";
    logger_file_name += get_timestamp_str();
    logger_file_name += "_client";
    logger_file_name += to_string(client_id);
    logger_file_name += ".txt";
    logger_out = fopen(logger_file_name.c_str(), "wb");

    Client client(client_id, client_num, has_label, network_file, data_file);

    // set up keys
    if (client.client_id == 0) {
        client.set_keys(pk, hr, si[client.client_id], client.client_id);

        // send keys
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string keys_i;
                client.serialize_send_keys(keys_i, pk, si[i], i);
                client.send_long_messages(i, keys_i);
            }
        }
    } else {
        // receive keys from client 0
        std::string recv_keys;
        client.recv_long_messages(0, recv_keys);
        client.recv_set_keys(recv_keys);
        mpz_init(n);
        mpz_init(positive_threshold);
        mpz_init(negative_threshold);
        compute_thresholds(client.m_pk, n, positive_threshold, negative_threshold);
    }

    switch(algorithm_type) {
        case 1:
            random_forest(client, solution_type, optimization_type, class_num, tree_type, max_bins, max_depth, num_trees);
            break;
        case 2:
            gbdt(client, solution_type, optimization_type, class_num, tree_type, max_bins, max_depth, num_trees);
            break;
        default:
            decision_tree(client, solution_type, optimization_type, class_num, tree_type, max_bins, max_depth, num_trees);
            break;
    }

    //logistic_regression(client)
    //decision_tree(client, solution_type, optimization_type);
    //random_forest(client, solution_type, optimization_type, class_num, tree_type);
    //gbdt(client, solution_type, optimization_type);

    //test_share_decrypt(client);

    /**NOTE: the following test is only for single client test with client_id = 0
     * Need to make the test codes independent with the main function
     * */
    //test_encoder();
    //test_djcs_t_aux();
    //test_lr();
    //test_pb();

    if (client_id == 0) {
        system_free();
    } else {
        mpz_clear(n);
        mpz_clear(positive_threshold);
        mpz_clear(negative_threshold);
    }

    logger(logger_out, "The End\n");

    return 0;
}