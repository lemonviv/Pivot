//
// Created by wuyuncheng on 26/11/19.
//

#include "cart_tree.h"
#include "../utils/djcs_t_aux.h"
#include "../utils/pb_converter.h"
#include <iomanip>
#include <random>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <map>
#include <stack>
#include "omp.h"
#include <chrono>
#include "../utils/score.h"

#include "../utils/spdz/spdz_util.h"
extern FILE * logger_out;

DecisionTree::DecisionTree() {}


DecisionTree::DecisionTree(int m_global_feature_num, int m_local_feature_num, int m_internal_node_num, int m_type, int m_classes_num, int m_max_depth,
        int m_max_bins, int m_prune_sample_num, float m_prune_threshold, int m_solution_type, int m_optimization_type) {

    global_feature_num = m_global_feature_num;
    local_feature_num = m_local_feature_num;
    for (int i = 0; i < m_local_feature_num; i++) {feature_types.push_back(0);} // default continuous variables
    internal_node_num = m_internal_node_num;
    type = m_type;
    classes_num = m_classes_num;
    max_depth = m_max_depth;
    max_bins = m_max_bins;
    prune_sample_num = m_prune_sample_num;
    prune_threshold = m_prune_threshold;

    // the maximum nodes, complete binary tree
    int maximum_nodes = pow(2, max_depth + 1) - 1;
    tree_nodes = new TreeNode[maximum_nodes];
    features = new Feature[local_feature_num];

    if (m_solution_type == 0) {
        solution_type = Basic;
    } else {
        solution_type = Enhanced;
    }

    switch (m_optimization_type) {
        case 1:
            optimization_type = CombiningSplits;
            break;
        case 2:
            optimization_type = Parallelism;
            break;
        case 3:
            optimization_type = All;
            break;
        default:
            optimization_type = Non;
            break;
    }
}


void DecisionTree::init_datasets(Client & client, float split) {

    //logger(logger_out, "Begin init dataset\n");

    int training_data_size = client.sample_num * split;
    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    for (int i = 0; i < client.sample_num; i++) {
        data_indexes.push_back(i);
    }

    //TODO: remove randomness for testing
    std::random_device rd;
    std::default_random_engine rng(rd());
    //auto rng = std::default_random_engine();
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);

    // select the former training data size as training data, and the latter as testing data
    for (int i = 0; i < data_indexes.size(); i++) {
        if (i < training_data_size) {
            // add to training dataset and labels
            training_data.push_back(client.local_data[data_indexes[i]]);
            if (client.has_label) {
                training_data_labels.push_back(client.labels[data_indexes[i]]);
            }
        } else {
            // add to testing dataset and labels
            testing_data.push_back(client.local_data[data_indexes[i]]);
            if (client.has_label) {
                testing_data_labels.push_back(client.labels[data_indexes[i]]);
            }
        }
    }

    int *new_indexes = new int[client.sample_num];
    for (int i = 0; i < client.sample_num; i++) {
        new_indexes[i] = data_indexes[i];
    }

    // send the data_indexes to the other client, and the other client splits in the same way
    for (int i = 0; i < client.client_num; i++) {
        if (i != client.client_id) {
            std::string s;
            serialize_batch_ids(new_indexes, client.sample_num, s);
            client.send_long_messages(i, s);
        }
    }

    // pre-compute indicator vectors or variance vectors for labels
    // here already assume that client_id == 0 (super client)
    if (type == 0) {
        // classification, compute binary vectors and store
        int * sample_num_per_class = new int[classes_num];
        for (int i = 0; i < classes_num; i++) {sample_num_per_class[i] = 0;}
        for (int i = 0; i < classes_num; i++) {
            std::vector<int> indicator_vec;
            for (int j = 0; j < training_data_labels.size(); j++) {
                if (training_data_labels[j] == (float) i) {
                    indicator_vec.push_back(1);
                    sample_num_per_class[i] += 1;
                } else {
                    indicator_vec.push_back(0);
                }
            }
            indicator_class_vecs.push_back(indicator_vec);

            indicator_vec.clear();
            indicator_vec.shrink_to_fit();
        }
        for (int i = 0; i < classes_num; i++) {
            logger(logger_out, "Class %d sample num = %d\n", i, sample_num_per_class[i]);
        }
        delete [] sample_num_per_class;
    } else {
        // regression, compute variance necessary stats
        std::vector<float> label_square_vec;
        for (int j = 0; j < training_data_labels.size(); j++) {
            label_square_vec.push_back(training_data_labels[j] * training_data_labels[j]);
        }
        variance_stat_vecs.push_back(training_data_labels); // the first vector is the actual label vector
        variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector
        classes_num = 2;  // for regression, the classes num is set to 2 for y and y^2

        label_square_vec.clear();
        label_square_vec.shrink_to_fit();
    }

    delete [] new_indexes;
    data_indexes.clear();
    data_indexes.shrink_to_fit();

    logger(logger_out, "Init dataset finished\n");
}


void DecisionTree::init_datasets_with_indexes(Client & client, int *new_indexes, float split) {

    //logger(logger_out, "Begin init dataset with indexes\n");

    int training_data_size = client.sample_num * split;
    // select the former training data size as training data, and the latter as testing data
    for (int i = 0; i < client.sample_num; i++) {
        if (i < training_data_size) {
            // add to training dataset and labels
            training_data.push_back(client.local_data[new_indexes[i]]);
            if (client.has_label) {
                training_data_labels.push_back(client.labels[new_indexes[i]]);
            }
        } else {
            // add to testing dataset and labels
            testing_data.push_back(client.local_data[new_indexes[i]]);
            if (client.has_label) {
                testing_data_labels.push_back(client.labels[new_indexes[i]]);
            }
        }
    }

    logger(logger_out, "Init dataset with indexes finished\n");
}


void DecisionTree::init_features() {

    //logger(logger_out, "Begin init features\n");

    for (int i = 0; i < local_feature_num; i++) {
        // 1. extract feature values of the i-th feature, compute samples_num
        // 2. check if distinct values number <= max_bins, if so, update splits_num as distinct number
        // 3. init feature, and assign to features[i]
        std::vector<float> feature_values;
        for (int j = 0; j < training_data.size(); j++) {
            feature_values.push_back(training_data[j][i]);
        }

        features[i].id = i;
        features[i].is_used = 0;
        features[i].is_categorical = feature_types[i];
        features[i].num_splits = max_bins - 1;
        features[i].max_bins = max_bins;
        features[i].set_feature_data(feature_values, training_data.size());
        features[i].sort_feature();
        features[i].find_splits();
        features[i].compute_split_ivs();

        //features[i].test_split_correctness();
        feature_values.clear();
        feature_values.shrink_to_fit();
    }

    logger(logger_out, "Init features finished\n");
}


void DecisionTree::init_root_node(Client & client) {

    //logger(logger_out, "Begin init root node\n");

    // Note that for the root node, every client can init the encrypted sample mask vector
    // but the label vectors need to be received from the super client
    // assume that the global feature number is known beforehand
    tree_nodes[0].is_leaf = -1;
    tree_nodes[0].available_feature_ids.reserve(local_feature_num);
    for (int i = 0; i < local_feature_num; i++) {
        tree_nodes[0].available_feature_ids.push_back(i);
    }
    tree_nodes[0].available_global_feature_num = global_feature_num;
    tree_nodes[0].sample_size = training_data.size();
    tree_nodes[0].type = type;
    tree_nodes[0].best_feature_id = -1;
    tree_nodes[0].best_client_id = -1;
    tree_nodes[0].best_split_id = -1;
    tree_nodes[0].depth = 0;
    tree_nodes[0].is_self_feature = -1;
    tree_nodes[0].left_child = -1;
    tree_nodes[0].right_child = -1;
    tree_nodes[0].sample_iv = new EncodedNumber[training_data.size()];

    EncodedNumber tmp;
    tmp.set_integer(client.m_pk->n[0], 1);

    // init encrypted mask vector on the root node
    for (int i = 0; i < training_data.size(); i++) {
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].sample_iv[i], tmp);
    }

    if (type == 0) {
        EncodedNumber max_impurity;
        max_impurity.set_float(client.m_pk->n[0], MAX_IMPURITY);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].impurity, max_impurity);
    } else {
        EncodedNumber max_variance;
        max_variance.set_float(client.m_pk->n[0], MAX_VARIANCE);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].impurity, max_variance);
    }

    logger(logger_out, "Init root node finished\n");
}


bool DecisionTree::check_pruning_conditions_revise(Client & client, int node_index) {

    //logger(logger_out, "Check pruning conditions\n");

    int is_satisfied = 0;
    std::string result_str;
    int recv_node_index = -1;
    EncodedNumber *label = new EncodedNumber[2];

    if (client.client_id == 0) {
        // super client check the pruning conditions
        // 1. available global feature num is 0
        // 2. the number of samples is less than a threshold
        // 3. the number of class is 1 (impurity == 0) or variance less than a threshold
        long available_num;
        EncodedNumber available_samples_num;
        available_samples_num.set_integer(client.m_pk->n[0], 0);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, available_samples_num, available_samples_num);
        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            djcs_t_aux_ee_add(client.m_pk, available_samples_num, available_samples_num, tree_nodes[node_index].sample_iv[i]);
        }

        // check if available_samples_num is less than a threshold (prune_sample_num), together with impurity
        // TODO: currently assume decryption on the sum and impurity, should convert to shares for computation
        EncodedNumber *encrypted_conditions = new EncodedNumber[2];
        EncodedNumber *decrypted_conditions = new EncodedNumber[2];
        encrypted_conditions[0] = available_samples_num;
        encrypted_conditions[1] = tree_nodes[node_index].impurity;

        client.share_batch_decrypt(encrypted_conditions, decrypted_conditions, 2);
        decrypted_conditions[0].decode(available_num);

        if ((tree_nodes[node_index].depth == max_depth) || (tree_nodes[node_index].available_global_feature_num == 0)) {
            // case 1
            is_satisfied = 1;
            logger(logger_out, "Pruning condition case 1 satisfied\n");
        } else {
            // communicate with SPDZ parties, to receive the label or check if the pruning condition is satisfied
            // case 2
            if ((int) available_num < prune_sample_num) {
                is_satisfied = 1;
                logger(logger_out, "Pruning condition case 2 satisfied\n");
            } else {
                // case 3
                if (type == 0) {
                    // check if impurity is equal to 0
                    float impurity;
                    decrypted_conditions[1].decode(impurity);
                    if (impurity == 0.0) {
                        is_satisfied = 1;
                        logger(logger_out, "Pruning condition case 3 satisfied\n");
                    }
                } else {
                    // check if variance is less than a threshold
                    float variance;
                    decrypted_conditions[1].decode(variance);
                    if (variance <= prune_threshold) {
                        is_satisfied = 1;
                        logger(logger_out, "Pruning condition case 3 satisfied\n");
                    }
                }
            }
        }

        // the first message for notifying whether is_satisfied == 1
        label[0].set_float(client.m_pk->n[0], -1); //dummy set the value
        serialize_prune_check_result(node_index, is_satisfied, label[0], result_str);

        // send to the other client
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, result_str);
            }
        }

        // pack pruning condition result pb string and sends to the other clients
        if (is_satisfied) {  // TODO: here should find majority class as label for classification, should modify latter
            // compute label information
            // for classification, the label is djcs_t_aux_dot_product(labels, sample_ivs) / available_num (might incorrect)
            // for regression, the label is djcs_t_aux_dot_product(labels, sample_ivs) / available_num
            if (type == 0) {
                EncodedNumber *class_sample_nums = new EncodedNumber[indicator_class_vecs.size()];
                for (int xx = 0; xx < indicator_class_vecs.size(); xx++) {
                    class_sample_nums[xx].set_integer(client.m_pk->n[0], 0);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, class_sample_nums[xx], class_sample_nums[xx]);
                    for (int j = 0; j < indicator_class_vecs[xx].size(); j++) {
                        if (indicator_class_vecs[xx][j] == 1) {
                            djcs_t_aux_ee_add(client.m_pk, class_sample_nums[xx], class_sample_nums[xx], tree_nodes[node_index].sample_iv[j]);
                        }
                    }
                }

                // TODO: should send to SPDZ for finding the majority class
                std::string sample_nums_str;
                EncodedNumber *decrypted_class_sample_nums = new EncodedNumber[indicator_class_vecs.size()];
                client.share_batch_decrypt(class_sample_nums, decrypted_class_sample_nums, indicator_class_vecs.size());

                float majority_class_label = -1;
                long majority_class_sample_num = -1;
                for (int j = 0; j < indicator_class_vecs.size(); j++) {
                    long x;
                    decrypted_class_sample_nums[j].decode(x);
                    logger(logger_out, "class %d num = %d\n", j, x);
                    if (majority_class_sample_num < x) {
                        majority_class_label = (float) j;
                        majority_class_sample_num = x;
                    }
                }

                //logger(logger_out, "Majority class label = %f\n", majority_class_label);

                EncodedNumber majority_label;
                majority_label.set_float(client.m_pk->n[0], majority_class_label, 2 * FLOAT_PRECISION);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, majority_label, majority_label);
                label[1] = majority_label;

                delete [] class_sample_nums;
                delete [] decrypted_class_sample_nums;
            } else {
                float inv_available_num = 1.0 / (float) available_num;
                EncodedNumber *inv_encoded = new EncodedNumber[1];
                inv_encoded[0].set_float(client.m_pk->n[0], inv_available_num);

                EncodedNumber *encoded_labels = new EncodedNumber[training_data_labels.size()];
                for (int i = 0; i < training_data_labels.size(); i++) {
                    encoded_labels[i].set_float(client.m_pk->n[0], training_data_labels[i]);
                }

                EncodedNumber *dot_product_res = new EncodedNumber[1];
                djcs_t_aux_inner_product(client.m_pk, client.m_hr, dot_product_res[0],
                                         tree_nodes[node_index].sample_iv, encoded_labels, training_data_labels.size());
                djcs_t_aux_ep_mul(client.m_pk, label[1], dot_product_res[0], inv_encoded[0]);

                delete [] inv_encoded;
                delete [] encoded_labels;
                delete [] dot_product_res;

            }

            // send encrypted impurity and plaintext label
            std::string second_result_str;
            serialize_prune_check_result(node_index, is_satisfied, label[1], second_result_str);
            tree_nodes[node_index].is_leaf = 1;
            tree_nodes[node_index].label = label[1];

            // send to the other client
            for (int i = 0; i < client.client_num; i++) {
                if (i != client.client_id) {
                    client.send_long_messages(i, second_result_str);
                }
            }
        }
        delete [] encrypted_conditions;
        delete [] decrypted_conditions;
    } else {
        // decrypt required information for checking pruning conditions
        std::string s, response_s;
        client.recv_long_messages(0, s);
        client.decrypt_batch_piece(s, response_s, 0);

        // receive the result of the pruning conditions check
        // leave update to the outside
        client.recv_long_messages(0, result_str);
        deserialize_prune_check_result(recv_node_index, is_satisfied, label[0], result_str);

        if (recv_node_index != node_index) {
            logger(logger_out, "Tree node index does not match\n");
            exit(1);
        }

        if (is_satisfied == 1) {
            if (type == 0) { // type == 0, need decrypt the label information for compute majority class
                std::string ss, response_ss;
                client.recv_long_messages(0, ss);
                client.decrypt_batch_piece(ss, response_ss, 0);
            }
            // receive another message for updating the label
            std::string second_result_str;
            client.recv_long_messages(0, second_result_str);
            int second_recv_node_index, second_is_satisfied;
            deserialize_prune_check_result(second_recv_node_index, second_is_satisfied, label[1], second_result_str);

            tree_nodes[node_index].is_leaf = 1;
            tree_nodes[node_index].label = label[1];
        }
    }

    logger(logger_out, "Pruning conditions check finished\n");
    delete [] label;
    return is_satisfied;
}


void DecisionTree::build_tree_node(Client & client, int node_index) {

    logger(logger_out, "************* Begin build tree node %d, tree depth = %d *************\n", node_index, tree_nodes[node_index].depth);

    /** recursively build a decision tree
     *
     *  // 1. check if some pruning conditions are satisfied
     *  //      1.1 feature set is empty;
     *  //      1.2 the number of samples are less than a pre-defined threshold
     *  //      1.3 if classification, labels are same; if regression, label variance is less than a threshold
     *  // 2. if satisfied, return a leaf node with majority class or mean label;
     *  //      else, continue to step 3
     *  // 3. super client computes some encrypted label information and broadcast to the other clients
     *  // 4. every client locally compute necessary encrypted statistics, i.e., #samples per class for classification or variance info
     *  // 5. the clients convert the encrypted statistics to shares and send to SPDZ parties
     *  // 6. wait for SPDZ parties return (i_*, j_*, s_*), where i_* is client id, j_* is feature id, and s_* is split id
     *  // 7. client who owns the best split feature do the splits and update mask vector, and broadcast to the other clients
     *  // 8. every client updates mask vector and local tree model
     *  // 9. recursively build the next two tree nodes
     *
     * */

    struct timeval tree_node_1, tree_node_2;
    double tree_node_time = 0;
    gettimeofday(&tree_node_1, NULL);

    if (node_index >= pow(2, max_depth + 1) - 1) {
        logger(logger_out, "Node exceeds the maximum tree depth\n");
        exit(1);
    }

    // if GBDT should find labels by dot product of two ciphertext vectors
    if (GBDT_FLAG == 1) {
        logger(logger_out, "Simulate GBDT ciphertext multiplication\n");
        // simulate multiplication between [z] and [y]
        EncodedNumber * encoded_values = new EncodedNumber[training_data.size()];
        EncodedNumber * encrypted_values = new EncodedNumber[training_data.size()];
        for (int i = 0; i < training_data.size(); i++) {
            encoded_values[i].set_float(client.m_pk->n[0], 0.25);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_values[i], encoded_values[i]);
        }

        EncodedNumber * encoded_values2 = new EncodedNumber[training_data.size()];
        EncodedNumber * encrypted_values2 = new EncodedNumber[training_data.size()];
        for (int i = 0; i < training_data.size(); i++) {
            encoded_values2[i].set_float(client.m_pk->n[0], 1.5);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_values2[i], encoded_values2[i]);
        }

        EncodedNumber * res = new EncodedNumber[training_data.size()];
        client.cipher_vectors_multiplication(encrypted_values2, encrypted_values, res, training_data.size());

        delete [] encoded_values;
        delete [] encrypted_values;
        delete [] encoded_values2;
        delete [] encrypted_values2;
        delete [] res;
    }

    /** step 1: check pruning conditions and update tree node accordingly */
    if (check_pruning_conditions_revise(client, node_index)) {
        return; // the corresponding process is in the check function
    }

    int used_classes_num = classes_num; // default is not packing

    // if pruning conditions are not satisfied (note that if satisfied, the handle is in the function)
    EncodedNumber ** encrypted_label_vecs;
    EncodedNumber *encrypted_labels; // flatten
    int sample_num = training_data.size();
    std::string result_str;

    /*** init static gfp for sending private batch shares ***/
    string prep_data_prefix = get_prep_dir(NUM_SPDZ_PARTIES, 128, gf2n::default_degree());
    //logger(logger_out, "prep_data_prefix = %s \n", prep_data_prefix.c_str());
    initialise_fields(prep_data_prefix);
    bigint::init_thread();
    // setup sockets
    std::vector<int> sockets = setup_sockets(NUM_SPDZ_PARTIES, client.client_id, client.host_names, SPDZ_PORT_NUM_DT);
    //for (int i = 0; i < NUM_SPDZ_PARTIES; i++) {
    //    logger(logger_out, "socket %d = %d\n", i, sockets[i]);
    //}

    /** step 3: super client computes some encrypted label information and broadcast to the other clients */
    tree_nodes[node_index].is_leaf = 0;
    // super client send encrypted labels to the other clients
    if (client.client_id == 0) {
        //struct timeval encrypted_label_1, encrypted_label_2;
        //double encrypted_label_time = 0;
        //gettimeofday(&encrypted_label_1, NULL);

        encrypted_labels = new EncodedNumber[used_classes_num * sample_num];
        for (int i = 0; i < used_classes_num; i++) {
            for (int j = 0; j < sample_num; j++) {
                EncodedNumber tmp;
                if (type == 0) {
                    tmp.set_float(client.m_pk->n[0], indicator_class_vecs[i][j]); // classification use indicator_class_vecs
                } else {
                    tmp.set_float(client.m_pk->n[0], variance_stat_vecs[i][j]); // regression use variance_stat_vecs
                }
                djcs_t_aux_ep_mul(client.m_pk, encrypted_labels[i * sample_num + j], tree_nodes[node_index].sample_iv[j], tmp);
            }
        }
        // TODO: simulate the running time when using GBDT model
        if (GBDT_FLAG == 1) {
            for (int i = 0; i < used_classes_num; i++) {
                EncodedNumber * tmp = new EncodedNumber[sample_num];
                for (int j = 0; j < sample_num; j++) {
                    tmp[j].set_float(client.m_pk->n[0], variance_stat_vecs[i][j]);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, tmp[j], tmp[j]);
                }
                EncodedNumber * res = new EncodedNumber[sample_num];
                client.cipher_vectors_multiplication(tree_nodes[node_index].sample_iv, tmp, res, sample_num);
                delete [] tmp;
                delete [] res;
            }
        }

        //gettimeofday(&encrypted_label_2, NULL);
        //encrypted_label_time += (double)((encrypted_label_2.tv_sec - encrypted_label_1.tv_sec) * 1000 + (double)(encrypted_label_2.tv_usec - encrypted_label_1.tv_usec) / 1000);
        //logger(logger_out, "Local encrypted label computation time: %'.3f ms\n", encrypted_label_time);

        // serialize and send to the other client
        serialize_encrypted_label_vector(node_index, used_classes_num, training_data_labels.size(), encrypted_labels, result_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, result_str);
            }
        }
    } else {
        client.recv_long_messages(0, result_str);
        int recv_node_index;
        deserialize_encrypted_label_vector(recv_node_index, encrypted_labels, result_str);
    }

    encrypted_label_vecs = new EncodedNumber*[used_classes_num];
    for (int i = 0; i < used_classes_num; i++) {
        encrypted_label_vecs[i] = new EncodedNumber[sample_num];
    }
    for (int i = 0; i < used_classes_num * sample_num; i++) {
        int a = i / sample_num;
        int b = i % sample_num;
        encrypted_label_vecs[a][b] = encrypted_labels[i];
    }

    /** step 4: every client locally compute necessary encrypted statistics, i.e., #samples per class for classification or variance info
     * pruning conditions not satisfied, compute encrypted statistics locally */

    // for each feature, for each split, for each class, compute necessary encrypted statistics
    // store the encrypted statistics, and convert to secret shares, and send to SPDZ parties for mpc computation
    // receive the (i_*, j_*, s_*) and encrypted impurity for the left child and right child, update tree_nodes

    // step 1: client inits a two-dimensional encrypted vector with size
    //          \sum_{i=0}^{node[available_feature_ids.size()]} features[i].num_splits
    // step 2: client computes encrypted statistics
    // step 3: client sends encrypted statistics
    // step 4: client 0 computes a large encrypted statistics matrix, and broadcasts total splits num
    // step 5: client converts the encrypted statistics matrix into secret shares
    // step 6: clients jointly send shares to SPDZ parties

    int local_splits_num = 0;
    int available_local_feature_num = tree_nodes[node_index].available_feature_ids.size();
    for (int i = 0; i < available_local_feature_num; i++) {
        int feature_id = tree_nodes[node_index].available_feature_ids[i];
        local_splits_num = local_splits_num + features[feature_id].num_splits;
    }

    int global_split_num = 0;
    EncodedNumber **global_encrypted_statistics;
    EncodedNumber *global_left_branch_sample_nums;
    EncodedNumber *global_right_branch_sample_nums;

    EncodedNumber **encrypted_statistics;
    EncodedNumber *encrypted_left_branch_sample_nums;
    EncodedNumber *encrypted_right_branch_sample_nums;

    std::vector< std::vector<float> > stats_shares;
    std::vector<int> left_sample_nums_shares;
    std::vector<int> right_sample_nums_shares;
    std::vector<int> client_split_nums;

    if (client.client_id == 0) {
        // compute local encrypted statistics
        if (local_splits_num != 0) {
            encrypted_statistics = new EncodedNumber*[local_splits_num];
            for (int i = 0; i < local_splits_num; i++) {
                encrypted_statistics[i] = new EncodedNumber[2 * used_classes_num];
            }
            encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
            encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];

            // call compute function
            compute_encrypted_statistics(client, node_index, encrypted_statistics,
                    encrypted_label_vecs, encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums);
        }

        global_encrypted_statistics = new EncodedNumber*[MAX_GLOBAL_SPLIT_NUM];
        for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
            global_encrypted_statistics[i] = new EncodedNumber[2 * used_classes_num];
        }
        global_left_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];
        global_right_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];

        // pack self
        if (local_splits_num == 0) {
            client_split_nums.push_back(0);
        } else {
            client_split_nums.push_back(local_splits_num);
            for (int i = 0; i < local_splits_num; i++) {
                global_left_branch_sample_nums[i] = encrypted_left_branch_sample_nums[i];
                global_right_branch_sample_nums[i] = encrypted_right_branch_sample_nums[i];
                for (int j = 0; j < 2 * used_classes_num; j++) {
                    global_encrypted_statistics[i][j] = encrypted_statistics[i][j];
                }
            }
        }

        global_split_num += local_splits_num;

        // receive from the other clients of the encrypted statistics
        for (int i = 0; i < client.client_num; i++) {
            std::string recv_encrypted_statistics_str;
            if (i != client.client_id) {
                client.recv_long_messages(i, recv_encrypted_statistics_str);

                int recv_client_id, recv_node_index, recv_split_num, recv_classes_num;
                EncodedNumber **recv_encrypted_statistics;
                EncodedNumber *recv_left_sample_nums;
                EncodedNumber *recv_right_sample_nums;

                deserialize_encrypted_statistics(recv_client_id, recv_node_index, recv_split_num, recv_classes_num,
                        recv_left_sample_nums, recv_right_sample_nums, recv_encrypted_statistics, recv_encrypted_statistics_str);

                // pack the encrypted statistics
                if (recv_split_num == 0) {
                    client_split_nums.push_back(0);
                    continue;
                } else {
                    client_split_nums.push_back(recv_split_num);
                    for (int j = 0; j < recv_split_num; j++) {
                        global_left_branch_sample_nums[global_split_num + j] = recv_left_sample_nums[j];
                        global_right_branch_sample_nums[global_split_num + j] = recv_right_sample_nums[j];
                        for (int k = 0; k < 2 * used_classes_num; k++) {
                            global_encrypted_statistics[global_split_num + j][k] = recv_encrypted_statistics[j][k];
                        }
                    }
                    global_split_num += recv_split_num;
                }

                delete [] recv_left_sample_nums;
                delete [] recv_right_sample_nums;
                for (int xx = 0; xx < recv_split_num; xx++) {
                    delete [] recv_encrypted_statistics[xx];
                }
                delete [] recv_encrypted_statistics;
            }
        }

        logger(logger_out, "The global_split_num = %d\n", global_split_num);

        // send the total number of splits for the other clients to generate secret shares
        //logger(logger_out, "Send global split num to the other clients\n");
        std::string split_info_str;
        serialize_split_info(global_split_num, client_split_nums, split_info_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, split_info_str);
            }
        }
    } else {
        if (local_splits_num == 0) {
            logger(logger_out, "Local feature used up\n");
            std::string s;
            serialize_encrypted_statistics(client.client_id, node_index, local_splits_num, used_classes_num,
                    encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums, encrypted_statistics, s);
            client.send_long_messages(0, s);
        } else {
            encrypted_statistics = new EncodedNumber*[local_splits_num];
            for (int i = 0; i < local_splits_num; i++) {
                encrypted_statistics[i] = new EncodedNumber[2 * used_classes_num];
            }
            encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
            encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];

            // call compute function
            compute_encrypted_statistics(client, node_index, encrypted_statistics,
                    encrypted_label_vecs, encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums);

            // send encrypted statistics to the super client
            std::string s;
            serialize_encrypted_statistics(client.client_id, node_index, local_splits_num, used_classes_num,
                    encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums, encrypted_statistics, s);
            client.send_long_messages(0, s);
        }

        //logger(logger_out, "Receive global split num from the super client\n");

        std::string recv_split_info_str;
        client.recv_long_messages(0, recv_split_info_str);
        deserialize_split_info(global_split_num, client_split_nums, recv_split_info_str);

        logger(logger_out, "The global_split_num = %d\n", global_split_num);
    }

    /** step 5: encrypted statistics computed finished, convert the encrypted values to secret shares */
    struct timeval conversion_1, conversion_2;
    double conversion_time = 0;
    gettimeofday(&conversion_1, NULL);

    if (client.client_id == 0) {
        // receive encrypted shares from the other clients
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string recv_s;
                EncodedNumber **recv_other_client_enc_shares;
                EncodedNumber *recv_left_shares;
                EncodedNumber *recv_right_shares;
//                EncodedNumber **recv_other_client_enc_shares = new EncodedNumber*[global_split_num];
//                for (int i = 0; i < global_split_num; i++) {
//                    encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
//                }
//                EncodedNumber *recv_left_shares = new EncodedNumber[global_split_num];
//                EncodedNumber *recv_right_shares = new EncodedNumber[global_split_num];
                int recv_client_id, recv_node_index, recv_split_num, recv_classes_num;
                client.recv_long_messages(i, recv_s);
                deserialize_encrypted_statistics(recv_client_id, recv_node_index, recv_split_num, recv_classes_num,
                        recv_left_shares, recv_right_shares, recv_other_client_enc_shares, recv_s);

                // aggregate the data into global encrypted vectors
                for (int j = 0; j < global_split_num; j++) {
                    djcs_t_aux_ee_add(client.m_pk, global_left_branch_sample_nums[j], global_left_branch_sample_nums[j], recv_left_shares[j]);
                    djcs_t_aux_ee_add(client.m_pk, global_right_branch_sample_nums[j], global_right_branch_sample_nums[j], recv_right_shares[j]);
                    for (int k = 0; k < 2 * used_classes_num; k++) {
                        djcs_t_aux_ee_add(client.m_pk, global_encrypted_statistics[j][k],
                                global_encrypted_statistics[j][k], recv_other_client_enc_shares[j][k]);
                    }
                }

                delete [] recv_left_shares;
                delete [] recv_right_shares;
                for (int xx = 0; xx < global_split_num; xx++) {
                    delete [] recv_other_client_enc_shares[xx];
                }
                delete [] recv_other_client_enc_shares;
            }
        }

        //logger(logger_out, "The global encrypted statistics are already aggregated\n");

        // call share decryption and convert to shares
        EncodedNumber **decrypted_global_statistics = new EncodedNumber*[global_split_num];
        for (int i = 0; i < global_split_num; i++) {
            decrypted_global_statistics[i] = new EncodedNumber[2 * used_classes_num];
        }
        EncodedNumber *decrypted_left_shares = new EncodedNumber[global_split_num];
        EncodedNumber *decrypted_right_shares = new EncodedNumber[global_split_num];

        // decrypt left shares and set to shares vector
        client.share_batch_decrypt(global_left_branch_sample_nums, decrypted_left_shares, global_split_num,
                (optimization_type == Parallelism || optimization_type == All));
        for (int i = 0; i < global_split_num; i++) {
            long x;
            decrypted_left_shares[i].decode(x);
            left_sample_nums_shares.push_back(x);
        }

        // decrypt right shares and set to shares vector
        client.share_batch_decrypt(global_right_branch_sample_nums, decrypted_right_shares, global_split_num,
                (optimization_type == Parallelism || optimization_type == All));
        for (int i = 0; i < global_split_num; i++) {
            long x;
            decrypted_right_shares[i].decode(x);
            right_sample_nums_shares.push_back(x);
        }

        // decrypt encrypted statistics and set to shares vector
        for (int i = 0; i < global_split_num; i++) {
            std::vector<float> tmp;
            client.share_batch_decrypt(global_encrypted_statistics[i], decrypted_global_statistics[i],
                    2 * used_classes_num, (optimization_type == Parallelism || optimization_type == All));
            for (int j = 0; j < 2 * used_classes_num; j++) {
                float x;
                decrypted_global_statistics[i][j].decode(x);
                tmp.push_back(x);
                //logger(logger_out, "statistics share[%d][%d] = %f\n", i, j, x);
            }
            stats_shares.push_back(tmp);
        }

        delete [] decrypted_left_shares;
        delete [] decrypted_right_shares;
        for (int i = 0; i < global_split_num; i++) {
            delete [] decrypted_global_statistics[i];
        }
        delete [] decrypted_global_statistics;

        //logger(logger_out, "Shares decrypt succeed, currently all the client shares are obtained\n");
    } else {
        // generate random shares, encrypt, and send to the super client
        global_encrypted_statistics = new EncodedNumber*[global_split_num];
        for (int i = 0; i < global_split_num; i++) {
            global_encrypted_statistics[i] = new EncodedNumber[2 * used_classes_num];
        }
        global_left_branch_sample_nums = new EncodedNumber[global_split_num];
        global_right_branch_sample_nums = new EncodedNumber[global_split_num];

        for (int i = 0; i < global_split_num; i++) {
            int r_left = static_cast<int> (rand() % MAXIMUM_RAND_VALUE);
            int r_right = static_cast<int> (rand() % MAXIMUM_RAND_VALUE);
            left_sample_nums_shares.push_back(0 - r_left);
            right_sample_nums_shares.push_back(0 - r_right);

            EncodedNumber a_left, a_right;
            a_left.set_integer(client.m_pk->n[0], r_left);
            a_right.set_integer(client.m_pk->n[0], r_right);

            djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_left_branch_sample_nums[i], a_left);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_right_branch_sample_nums[i], a_right);

            std::vector<float> tmp;
            for (int j = 0; j < 2 * used_classes_num; j++) {
                float r_stat = static_cast<float> (rand() % MAXIMUM_RAND_VALUE);
                tmp.push_back(0 - r_stat);
                //logger(logger_out, "statistics share[%d][%d] = %f\n", i, j, r_stat);

                EncodedNumber a_stat;
                a_stat.set_float(client.m_pk->n[0], r_stat);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_encrypted_statistics[i][j], a_stat);
            }

            stats_shares.push_back(tmp);
        }

        // serialize encrypted statistics and send to the super client
        std::string s_enc_shares;
        serialize_encrypted_statistics(client.client_id, node_index, global_split_num, used_classes_num,
                global_left_branch_sample_nums, global_right_branch_sample_nums, global_encrypted_statistics, s_enc_shares);
        client.send_long_messages(0, s_enc_shares);

        //logger(logger_out, "Send encrypted shares succeed\n");

        // receive share decrypt information, and decrypt the corresponding information
        std::string s_left_shares, response_s_left_shares, s_right_shares, response_s_right_shares;
        client.recv_long_messages(0, s_left_shares);
        client.decrypt_batch_piece(s_left_shares, response_s_left_shares, 0,
                (optimization_type == Parallelism || optimization_type == All));

        client.recv_long_messages(0, s_right_shares);
        client.decrypt_batch_piece(s_right_shares, response_s_right_shares, 0,
                (optimization_type == Parallelism || optimization_type == All));

        for (int i = 0; i < global_split_num; i++) {
            std::string s_stat_shares, response_s_stat_shares;
            client.recv_long_messages(0, s_stat_shares);
            client.decrypt_batch_piece(s_stat_shares, response_s_stat_shares, 0,
                    (optimization_type == Parallelism || optimization_type == All));
        }
    }

    logger(logger_out, "Conversion to secret shares succeed\n");

//    logger(logger_out, "Print the secret shares of client statistics for debugging\n");
//    for (int i = 0; i < stats_shares.size(); i++) {
//        for (int j = 0; j < stats_shares[0].size(); j++) {
//            logger(logger_out, "client_statistics[%d][%d] = %f\n", i, j, stats_shares[i][j]);
//        }
//    }

    gettimeofday(&conversion_2, NULL);
    conversion_time += (double)((conversion_2.tv_sec - conversion_1.tv_sec) * 1000 + (double)(conversion_2.tv_usec - conversion_1.tv_usec) / 1000);
    logger(logger_out, "Secret share conversion time: %'.3f ms\n", conversion_time);

    //double current_collapse_time = 0;
    //current_collapse_time += (double) ((conversion_2.tv_sec - tree_node_1.tv_sec) * 1000 + (double)(conversion_2.tv_usec - tree_node_1.tv_usec) / 1000);
    //logger(logger_out, "Current collapse time: %'.3f ms\n", current_collapse_time);

    struct timeval spdz_1, spdz_2;
    double spdz_time = 0;
    gettimeofday(&spdz_1, NULL);

    /** step 6: secret shares conversion finished, talk to SPDZ parties for MPC computations */
    if (client.client_id == 0) {
        send_public_parameters(type, global_split_num, classes_num, used_classes_num, sockets, NUM_SPDZ_PARTIES);
        //logger(logger_out, "Finish send public parameters to SPDZ engines\n");
    }

    for (int i = 0; i < global_split_num; i++) {
        for (int j = 0; j < used_classes_num * 2; j++) {
            vector<float> x;
            x.push_back(stats_shares[i][j]);
            send_private_batch_shares(x, sockets, NUM_SPDZ_PARTIES);
        }
    }

    for (int i = 0; i < global_split_num; i++) {
        vector<gfp> input_values_gfp(1);
        input_values_gfp[0].assign(left_sample_nums_shares[i]);
        send_private_inputs(input_values_gfp, sockets, NUM_SPDZ_PARTIES);
    }

    for (int i = 0; i < global_split_num; i++) {
        vector<gfp> input_values_gfp(1);
        input_values_gfp[0].assign(right_sample_nums_shares[i]);
        send_private_inputs(input_values_gfp, sockets, NUM_SPDZ_PARTIES);
    }

    //logger(logger_out, "Finish send private values to SPDZ engines\n");

    // receive result from the SPDZ parties
    // TODO: communicate with SPDZ parties
    int index_in_global_split_num = 0;
    vector<float> impurities = receive_result_dt(sockets, NUM_SPDZ_PARTIES, 3, index_in_global_split_num);
    EncodedNumber *encrypted_impurities = new EncodedNumber[impurities.size()];
    for (int i = 0; i < impurities.size(); i++) {
        encrypted_impurities[i].set_float(client.m_pk->n[0], impurities[i], FLOAT_PRECISION);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_impurities[i], encrypted_impurities[i]);
    }

    logger(logger_out, "Received: best_split_index = %d\n", index_in_global_split_num);

    gettimeofday(&spdz_2, NULL);
    spdz_time += (double)((spdz_2.tv_sec - spdz_1.tv_sec) * 1000 + (double)(spdz_2.tv_usec - spdz_1.tv_usec) / 1000);
    logger(logger_out, "SPDZ time: %'.3f ms\n", spdz_time);

    //double current_collapse_time_2 = 0;
    //current_collapse_time_2 += (double) ((spdz_2.tv_sec - tree_node_1.tv_sec) * 1000 + (double)(spdz_2.tv_usec - tree_node_1.tv_usec) / 1000);
    //logger(logger_out, "Current collapse time 2: %'.3f ms\n", current_collapse_time_2);

    /** step 7: update tree nodes, including sample iv for the next tree node computation */
    int left_child_index = 2 * node_index + 1;
    int right_child_index = 2 * node_index + 2;

    // convert the index_in_global_split_num to (i_*, index_*) and send to i_* client
    int i_star = -1;
    int index_star = -1;
    int index_tmp = index_in_global_split_num;
    for (int i = 0; i < client_split_nums.size(); i++) {
        if (index_tmp < client_split_nums[i]) {
            i_star = i;
            index_star = index_tmp;
            break;
        } else {
            index_tmp = index_tmp - client_split_nums[i];
        }
    }

    if (i_star == client.client_id) {
        // compute locally and broadcast
        // find the j_* feature and s_* split
        int j_star = -1;
        int s_star = -1;
        int index_star_tmp = index_star;
        for (int i = 0; i < tree_nodes[node_index].available_feature_ids.size(); i++) {
            int feature_id = tree_nodes[node_index].available_feature_ids[i];
            if (index_star_tmp < features[feature_id].num_splits) {
                j_star = feature_id;
                s_star = index_star_tmp;
                break;
            } else {
                index_star_tmp = index_star_tmp - features[feature_id].num_splits;
            }
        }

        // now we have (i_*, j_*, s_*), retrieve s_*-th split ivs and update sample_ivs of two branches
        EncodedNumber *aggregate_enc_impurities = new EncodedNumber[impurities.size()];
        for (int i = 0; i < impurities.size(); i++) {
            aggregate_enc_impurities[i] = encrypted_impurities[i];
        }

        for (int i = 0; i < client.client_num; i++) {
            if (i != i_star) {
                std::string recv_s;
                EncodedNumber *recv_encrypted_impurities;// = new EncodedNumber[impurities.size()];
                client.recv_long_messages(i, recv_s);
                int size;
                deserialize_sums_from_string(recv_encrypted_impurities, size, recv_s);
                for (int j = 0; j < impurities.size(); j++) {
                    djcs_t_aux_ee_add(client.m_pk, aggregate_enc_impurities[j],
                            aggregate_enc_impurities[j], recv_encrypted_impurities[j]);
                }

                delete [] recv_encrypted_impurities;
            }
        }

        struct timeval enhanced_1, enhanced_2;
        double enhanced_time = 0;
        gettimeofday(&enhanced_1, NULL);

        // TODO: here simulate the additional two steps for the enhanced solution, to be complete the entire code with SPDZ
        if (solution_type == Enhanced) {
            logger(logger_out, "Enhanced solution\n");
            // the first step is to private select an encrypted iv with size sample_num
            int cur_split_num = features[j_star].num_splits;
            EncodedNumber *selection_iv = new EncodedNumber[cur_split_num];
            for (int ss = 0; ss < cur_split_num; ss++) {
                if (ss == s_star) {
                    selection_iv[ss].set_integer(client.m_pk->n[0], 1);
                } else {
                    selection_iv[ss].set_integer(client.m_pk->n[0], 0);
                }
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, selection_iv[ss], selection_iv[ss]);
            }

            EncodedNumber *left_selection_result = new EncodedNumber[sample_num];
            EncodedNumber *right_selection_result = new EncodedNumber[sample_num];

            private_split_selection(client, left_selection_result, selection_iv, features[j_star].split_ivs_left, sample_num, cur_split_num);
            private_split_selection(client, right_selection_result, selection_iv, features[j_star].split_ivs_right, sample_num, cur_split_num);

            // the second step is to update the encrypted mask vector using selection_result and sample_iv
            update_sample_iv(client, i_star, left_selection_result, right_selection_result, node_index);

            delete [] selection_iv;
            delete [] left_selection_result;
            delete [] right_selection_result;
        }

        gettimeofday(&enhanced_2, NULL);
        enhanced_time += (double)((enhanced_2.tv_sec - enhanced_1.tv_sec) * 1000 + (double)(enhanced_2.tv_usec - enhanced_1.tv_usec) / 1000);
        logger(logger_out, "Enhance solution additional time: %'.3f ms\n", enhanced_time);

        // update current node index for prediction
        tree_nodes[node_index].is_self_feature = 1;
        tree_nodes[node_index].best_client_id = i_star;
        tree_nodes[node_index].best_feature_id = j_star;
        tree_nodes[node_index].best_split_id = s_star;

        tree_nodes[left_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[right_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[left_child_index].impurity = aggregate_enc_impurities[0];
        tree_nodes[right_child_index].impurity = aggregate_enc_impurities[1];
        tree_nodes[left_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[right_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[left_child_index].type = tree_nodes[node_index].type;
        tree_nodes[right_child_index].type = tree_nodes[node_index].type;
        tree_nodes[left_child_index].available_global_feature_num = tree_nodes[node_index].available_global_feature_num - 1;
        tree_nodes[right_child_index].available_global_feature_num = tree_nodes[node_index].available_global_feature_num - 1;

        for (int i = 0; i < tree_nodes[node_index].available_feature_ids.size(); i++) {
            int feature_id = tree_nodes[node_index].available_feature_ids[i];
            if (j_star != feature_id) {
                tree_nodes[left_child_index].available_feature_ids.push_back(feature_id);
                tree_nodes[right_child_index].available_feature_ids.push_back(feature_id);
            }
        }

        // compute between split_left_iv and sample_iv and update
        std::vector<int> split_left_iv = features[j_star].split_ivs_left[s_star];
        std::vector<int> split_right_iv = features[j_star].split_ivs_right[s_star];

        tree_nodes[left_child_index].sample_iv = new EncodedNumber[tree_nodes[left_child_index].sample_size];
        tree_nodes[right_child_index].sample_iv = new EncodedNumber[tree_nodes[right_child_index].sample_size];

        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for if((optimization_type == Parallelism) || (optimization_type == All))
        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            EncodedNumber left, right;
            left.set_integer(client.m_pk->n[0], split_left_iv[i]);
            right.set_integer(client.m_pk->n[0], split_right_iv[i]);
            djcs_t_aux_ep_mul(client.m_pk, tree_nodes[left_child_index].sample_iv[i], tree_nodes[node_index].sample_iv[i], left);
            djcs_t_aux_ep_mul(client.m_pk, tree_nodes[right_child_index].sample_iv[i], tree_nodes[node_index].sample_iv[i], right);
        }

        // serialize and send to the other clients
        std::string update_str;
        serialize_update_info(client.client_id, client.client_id, j_star, s_star, aggregate_enc_impurities[0], aggregate_enc_impurities[1],
                tree_nodes[left_child_index].sample_iv, tree_nodes[right_child_index].sample_iv,
                tree_nodes[node_index].sample_size, update_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, update_str);
            }
        }

        //test_sample_iv_update_correctness(client, node_index, i_star);
        delete [] aggregate_enc_impurities;
        split_left_iv.clear();
        split_right_iv.clear();
        split_left_iv.shrink_to_fit();
        split_right_iv.shrink_to_fit();
    } else {
        /** step 8: every client update the local tree model */
        // serialize encrypted impurities and send to i_star
        std::string s;
        serialize_batch_sums(encrypted_impurities, impurities.size(), s);
        client.send_long_messages(i_star, s);

        struct timeval enhanced_1, enhanced_2;
        double enhanced_time = 0;
        gettimeofday(&enhanced_1, NULL);

        // simulation
        if (solution_type == Enhanced) {
            logger(logger_out, "Enhanced solution\n");
            EncodedNumber * left_selection_result;// = new EncodedNumber[sample_num];
            EncodedNumber * right_selection_result;// = new EncodedNumber[sample_num];

            update_sample_iv(client, i_star, left_selection_result, right_selection_result, node_index);

            delete [] left_selection_result;
            delete [] right_selection_result;
        }

        gettimeofday(&enhanced_2, NULL);
        enhanced_time += (double)((enhanced_2.tv_sec - enhanced_1.tv_sec) * 1000 + (double)(enhanced_2.tv_usec - enhanced_1.tv_usec) / 1000);
        logger(logger_out, "Enhance solution additional time: %'.3f ms\n", enhanced_time);

        // receive from i_star client and update
        std::string recv_update_str;
        client.recv_long_messages(i_star, recv_update_str);

        // deserialize and update
        int recv_source_client_id, recv_best_client_id, recv_best_feature_id, recv_best_split_id;
        EncodedNumber recv_left_impurity, recv_right_impurity;
        EncodedNumber *recv_left_sample_iv, *recv_right_sample_iv;
        deserialize_update_info(recv_source_client_id, recv_best_client_id, recv_best_feature_id, recv_best_split_id,
                recv_left_impurity, recv_right_impurity, recv_left_sample_iv, recv_right_sample_iv, recv_update_str);

        // update tree nodes
        if (i_star != recv_best_client_id) {
            logger(logger_out, "Suspicious message\n");
        }

        // update current node index for prediction
        tree_nodes[node_index].is_self_feature = 0;
        tree_nodes[node_index].best_client_id = recv_best_client_id;
        tree_nodes[node_index].best_feature_id = recv_best_feature_id;
        tree_nodes[node_index].best_split_id = recv_best_split_id;

        for (int i = 0; i < tree_nodes[node_index].available_feature_ids.size(); i++) {
            int feature_id = tree_nodes[node_index].available_feature_ids[i];
            tree_nodes[left_child_index].available_feature_ids.push_back(feature_id);
            tree_nodes[right_child_index].available_feature_ids.push_back(feature_id);
        }

        tree_nodes[left_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[right_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[left_child_index].impurity = recv_left_impurity;
        tree_nodes[right_child_index].impurity = recv_right_impurity;
        tree_nodes[left_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[right_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[left_child_index].type = tree_nodes[node_index].type;
        tree_nodes[right_child_index].type = tree_nodes[node_index].type;
        tree_nodes[left_child_index].available_global_feature_num = tree_nodes[node_index].available_global_feature_num - 1;
        tree_nodes[right_child_index].available_global_feature_num = tree_nodes[node_index].available_global_feature_num - 1;

        tree_nodes[left_child_index].sample_iv = new EncodedNumber[tree_nodes[left_child_index].sample_size];
        tree_nodes[right_child_index].sample_iv = new EncodedNumber[tree_nodes[right_child_index].sample_size];

        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            tree_nodes[left_child_index].sample_iv[i] = recv_left_sample_iv[i];
            tree_nodes[right_child_index].sample_iv[i] = recv_right_sample_iv[i];
        }

        //test_sample_iv_update_correctness(client, node_index, i_star);
        delete [] recv_left_sample_iv;
        delete [] recv_right_sample_iv;
    }

    for (unsigned int i = 0; i < sockets.size(); i++) {
        close_client_socket(sockets[i]);
    }

    //logger(logger_out, "Tree node %d update finished\n", node_index);

    gettimeofday(&tree_node_2, NULL);
    tree_node_time += (double)((tree_node_2.tv_sec - tree_node_1.tv_sec) * 1000 + (double)(tree_node_2.tv_usec - tree_node_1.tv_usec) / 1000);
    logger(logger_out, "Build a tree node time: %'.3f ms\n", tree_node_time);

    /** free memory used before recursive function call to save memory */
    delete [] encrypted_labels;
    delete [] encrypted_left_branch_sample_nums;
    delete [] encrypted_right_branch_sample_nums;
    delete [] global_left_branch_sample_nums;
    delete [] global_right_branch_sample_nums;
    delete [] encrypted_impurities;

    for (int i = 0; i < used_classes_num; i++) {
        delete [] encrypted_label_vecs[i];
    }
    delete [] encrypted_label_vecs;

    if (local_splits_num != 0) {
        for (int i = 0; i < local_splits_num; i++) {
            delete [] encrypted_statistics[i];
        }
        delete [] encrypted_statistics;
    }

    if (client.client_id == 0) {
        for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
            delete [] global_encrypted_statistics[i];
        }
        delete [] global_encrypted_statistics;
    } else {
        for (int i = 0; i < global_split_num; i++) {
            delete [] global_encrypted_statistics[i];
        }
        delete [] global_encrypted_statistics;
    }

    left_sample_nums_shares.clear();
    left_sample_nums_shares.shrink_to_fit();
    right_sample_nums_shares.clear();
    right_sample_nums_shares.shrink_to_fit();
    stats_shares.clear();
    stats_shares.shrink_to_fit();
    client_split_nums.clear();
    client_split_nums.shrink_to_fit();

    // sample_iv of the current node will never used again, so free the memory
    //delete [] tree_nodes[node_index].sample_iv;

    /** step 9: recursively build the next child tree nodes */
    internal_node_num += 1;
    build_tree_node(client, left_child_index);
    build_tree_node(client, right_child_index);

    logger(logger_out, "End build tree node %d\n", node_index);
}


void DecisionTree::compute_encrypted_statistics(Client & client, int node_index,
        EncodedNumber ** & encrypted_statistics,
        EncodedNumber ** encrypted_label_vecs,
        EncodedNumber * & encrypted_left_sample_nums,
        EncodedNumber * & encrypted_right_sample_nums) {

    //struct timeval parallel_1, parallel_2;
    //double parallel_time = 0;
    //gettimeofday(&parallel_1, NULL);

    //logger(logger_out, "Begin compute encrypted statistics\n");

    int split_index = 0;
    int available_feature_num = tree_nodes[node_index].available_feature_ids.size();
    int sample_num = features[0].split_ivs_left[0].size();
    int used_classes_num = classes_num;

    /**
     * splits of features are flatted, classes_num * 2 are for left and right
     */

    omp_lock_t lock;
    omp_init_lock(&lock);

    // TODO: this function is a little difficult to read, should refine the code and add more explanations
    if ((optimization_type == CombiningSplits) || (optimization_type == All)) {

        // combining splits optimization
        //omp_set_num_threads(NUM_OMP_THREADS);
//#pragma omp parallel for if((optimization_type == Parallelism) || (optimization_type == All))
        for (int j = 0; j < available_feature_num; j++) {

            //omp_set_lock(&lock);

            int feature_id = tree_nodes[node_index].available_feature_ids[j];

            // in this method, the feature values are sorted, use the sorted indexes to re-organize
            // the encrypted mask vector, and compute num_splits + 1 bucket statistics by one traverse
            // then the encrypted statistics for the num_splits can be aggregated by num_splits homomorphic additions

            int split_num = features[feature_id].num_splits;
            std::vector<int> sorted_indices = features[feature_id].sorted_indexes;
            EncodedNumber * sorted_sample_iv = new EncodedNumber[sample_num];

            // copy the sample_iv
            for (int idx = 0; idx < sample_num; idx++) {
                sorted_sample_iv[idx] = tree_nodes[node_index].sample_iv[sorted_indices[idx]];
            }

            // compute the encrypted aggregation of split_num + 1 buckets
            EncodedNumber * left_sums = new EncodedNumber[split_num];
            EncodedNumber * right_sums = new EncodedNumber[split_num];
            EncodedNumber total_sum;
            total_sum.set_integer(client.m_pk->n[0], 0);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, total_sum, total_sum);
            for (int idx = 0; idx < split_num; idx++) {
                left_sums[idx].set_integer(client.m_pk->n[0], 0);
                right_sums[idx].set_integer(client.m_pk->n[0], 0);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_sums[idx], left_sums[idx]);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_sums[idx], right_sums[idx]);
            }

            // compute sample iv statistics by one traverse
            int split_iterator = 0;
            for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
                djcs_t_aux_ee_add(client.m_pk, total_sum, total_sum, sorted_sample_iv[sample_idx]);

                if (split_iterator == split_num) {
                    continue;
                }

                int sorted_idx = sorted_indices[sample_idx];
                float sorted_feature_value = features[feature_id].original_feature_values[sorted_idx];

                // find the first split value that larger than the current feature value, usually only step by 1
                // TODO: make sure this is correct?
                if (sorted_feature_value > features[feature_id].split_values[split_iterator]) {
                    split_iterator += 1;
                    if (split_iterator == split_num) continue;
                }

                djcs_t_aux_ee_add(client.m_pk, left_sums[split_iterator], left_sums[split_iterator], sorted_sample_iv[sample_idx]);
            }

            // compute the encrypted statistics for each class
            EncodedNumber ** left_stats = new EncodedNumber*[split_num];
            EncodedNumber ** right_stats = new EncodedNumber*[split_num];
            EncodedNumber * sums_stats = new EncodedNumber[used_classes_num];
            for (int k = 0; k < split_num; k++) {
                left_stats[k] = new EncodedNumber[used_classes_num];
                right_stats[k] = new EncodedNumber[used_classes_num];
            }
            for (int k = 0; k < split_num; k++) {
                for (int c = 0; c < used_classes_num; c++) {
                    left_stats[k][c].set_float(client.m_pk->n[0], 0);
                    right_stats[k][c].set_float(client.m_pk->n[0], 0);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_stats[k][c], left_stats[k][c]);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_stats[k][c], right_stats[k][c]);
                }
            }
            for (int c = 0; c < used_classes_num; c++) {
                sums_stats[c].set_float(client.m_pk->n[0], 0);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, sums_stats[c], sums_stats[c]);
            }

            split_iterator = 0;
            for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
                int sorted_idx = sorted_indices[sample_idx];
                for (int c = 0; c < used_classes_num; c++) {
                    djcs_t_aux_ee_add(client.m_pk, sums_stats[c], sums_stats[c], encrypted_label_vecs[c][sorted_idx]);
                }

                if (split_iterator == split_num) {
                    continue;
                }

                float sorted_feature_value = features[feature_id].original_feature_values[sorted_idx];

                // find the first split value that larger than the current feature value, usually only step by 1
                if (sorted_feature_value > features[feature_id].split_values[split_iterator]) {
                    split_iterator += 1;
                    if (split_iterator == split_num) continue;
                }

                for (int c = 0; c < used_classes_num; c++) {
                    djcs_t_aux_ee_add(client.m_pk, left_stats[split_iterator][c],
                            left_stats[split_iterator][c], encrypted_label_vecs[c][sorted_idx]);
                }
            }

            // write the left sums to encrypted_left_sample_nums and update the right sums
            EncodedNumber left_num_help, right_num_help, plain_constant_help;
            left_num_help.set_integer(client.m_pk->n[0], 0);
            right_num_help.set_integer(client.m_pk->n[0], 0);
            plain_constant_help.set_integer(client.m_pk->n[0], -1);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_num_help, left_num_help);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_num_help, right_num_help);

            EncodedNumber * left_stat_help = new EncodedNumber[used_classes_num];
            EncodedNumber * right_stat_help = new EncodedNumber[used_classes_num];
            for (int c = 0; c < used_classes_num; c++) {
                left_stat_help[c].set_float(client.m_pk->n[0], 0);
                right_stat_help[c].set_float(client.m_pk->n[0], 0);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_stat_help[c], left_stat_help[c]);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_stat_help[c], right_stat_help[c]);
            }

            // compute right sample num of the current split by total_sum + (-1) * left_sum_help
            for (int k = 0; k < split_num; k++) {
                djcs_t_aux_ee_add(client.m_pk, left_num_help, left_num_help, left_sums[k]);
                encrypted_left_sample_nums[split_index] = left_num_help;
                djcs_t_aux_ep_mul(client.m_pk, right_num_help, left_num_help, plain_constant_help);
                djcs_t_aux_ee_add(client.m_pk, encrypted_right_sample_nums[split_index], total_sum, right_num_help);

                for (int c = 0; c < used_classes_num; c++) {
                    djcs_t_aux_ee_add(client.m_pk, left_stat_help[c], left_stat_help[c], left_stats[k][c]);
                    djcs_t_aux_ep_mul(client.m_pk, right_stat_help[c], left_stat_help[c], plain_constant_help);
                    djcs_t_aux_ee_add(client.m_pk, right_stat_help[c], right_stat_help[c], sums_stats[c]);
                    encrypted_statistics[split_index][2 * c] = left_stat_help[c];
                    encrypted_statistics[split_index][2 * c + 1] = right_stat_help[c];
                }

                split_index += 1; // update the global split index
            }

            //omp_unset_lock(&lock);

            delete [] sorted_sample_iv;
            delete [] left_sums;
            delete [] right_sums;
            for (int k = 0; k < split_num; k++) {
                delete [] left_stats[k];
                delete [] right_stats[k];
            }
            delete [] left_stats;
            delete [] right_stats;
            delete [] sums_stats;
            delete [] left_stat_help;
            delete [] right_stat_help;
        }
    }
    else { // no combining splits optimization

        //omp_set_num_threads(NUM_OMP_THREADS);
//#pragma omp parallel for if((optimization_type == Parallelism) || (optimization_type == All))
        for (int j = 0; j < available_feature_num; j++) {

            int feature_id = tree_nodes[node_index].available_feature_ids[j];

            //omp_set_lock(&lock);

            for (int s = 0; s < features[feature_id].num_splits; s++) {

                // compute encrypted statistics (left branch and right branch) for the current split
                std::vector<int> left_iv = features[feature_id].split_ivs_left[s];
                std::vector<int> right_iv = features[feature_id].split_ivs_right[s];

                EncodedNumber left_sum, right_sum;
                left_sum.set_integer(client.m_pk->n[0], 0);
                right_sum.set_integer(client.m_pk->n[0], 0);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_sum, left_sum);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_sum, right_sum);

                for (int i = 0; i < left_iv.size(); i++) {
                    if (left_iv[i] == 1) {
                        djcs_t_aux_ee_add(client.m_pk, left_sum, left_sum, tree_nodes[node_index].sample_iv[i]);
                    }
                    if (right_iv[i] == 1) {
                        djcs_t_aux_ee_add(client.m_pk, right_sum, right_sum, tree_nodes[node_index].sample_iv[i]);
                    }
                }

                encrypted_left_sample_nums[split_index] = left_sum;
                encrypted_right_sample_nums[split_index] = right_sum;

                for (int c = 0; c < used_classes_num; c++) {
                    EncodedNumber left_stat, right_stat;
                    left_stat.set_float(client.m_pk->n[0], 0);
                    right_stat.set_float(client.m_pk->n[0], 0);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_stat, left_stat);
                    djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_stat, right_stat);

                    for (int k = 0; k < sample_num; k++) {
                        if (left_iv[k] == 1) {
                            djcs_t_aux_ee_add(client.m_pk, left_stat, left_stat, encrypted_label_vecs[c][k]);
                        }
                        if (right_iv[k] == 1) {
                            djcs_t_aux_ee_add(client.m_pk, right_stat, right_stat, encrypted_label_vecs[c][k]);
                        }
                    }

                    encrypted_statistics[split_index][2 * c] = left_stat;
                    encrypted_statistics[split_index][2 * c + 1] = right_stat;
                }
                split_index ++;
            }
            //omp_unset_lock(&lock);
        }
    }

    logger(logger_out, "Compute encrypted statistics finished\n");

    //gettimeofday(&parallel_2, NULL);
    //parallel_time += (double)((parallel_2.tv_sec - parallel_1.tv_sec) * 1000 + (double)(parallel_2.tv_usec - parallel_1.tv_usec) / 1000);
    //logger(logger_out, "Encrypted statistic computation time: %'.3f ms\n", parallel_time);
}


std::vector<int> DecisionTree::compute_binary_vector(int sample_id, std::map<int, int> node_index_2_leaf_index_map) {

    std::vector<float> sample_values = testing_data[sample_id];
    std::vector<int> binary_vector(internal_node_num + 1);

    // traverse the whole tree iteratively, and compute binary_vector
    std::stack<PredictionObj> traverse_prediction_objs;
    PredictionObj prediction_obj(tree_nodes[0].is_leaf, tree_nodes[0].is_self_feature, tree_nodes[0].best_client_id,
            tree_nodes[0].best_feature_id, tree_nodes[0].best_split_id, 1, 0);
    traverse_prediction_objs.push(prediction_obj);
    while (!traverse_prediction_objs.empty()) {
        PredictionObj pred_obj = traverse_prediction_objs.top();
        if (pred_obj.is_leaf == 1) {
            // find leaf index and record
            int leaf_index = node_index_2_leaf_index_map.find(pred_obj.index)->second;
            binary_vector[leaf_index] = pred_obj.mark;
            traverse_prediction_objs.pop();
        } else if (pred_obj.is_self_feature != 1) {
            // both left and right branches are marked as 1 * current_mark
            traverse_prediction_objs.pop();
            int left_node_index = pred_obj.index * 2 + 1;
            int right_node_index = pred_obj.index * 2 + 2;

            PredictionObj left(tree_nodes[left_node_index].is_leaf, tree_nodes[left_node_index].is_self_feature,
                    tree_nodes[left_node_index].best_client_id, tree_nodes[left_node_index].best_feature_id,
                    tree_nodes[left_node_index].best_split_id, pred_obj.mark, left_node_index);
            PredictionObj right(tree_nodes[right_node_index].is_leaf, tree_nodes[right_node_index].is_self_feature,
                    tree_nodes[right_node_index].best_client_id, tree_nodes[right_node_index].best_feature_id,
                    tree_nodes[right_node_index].best_split_id, pred_obj.mark, right_node_index);
            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        } else {
            // is self feature, retrieve split value and compare
            traverse_prediction_objs.pop();
            int feature_id = pred_obj.best_feature_id;
            int split_id = pred_obj.best_split_id;
            float split_value = features[feature_id].split_values[split_id];
            int left_mark, right_mark;
            if (sample_values[feature_id] <= split_value) {
                left_mark = pred_obj.mark * 1;
                right_mark = pred_obj.mark * 0;
            } else {
                left_mark = pred_obj.mark * 0;
                right_mark = pred_obj.mark * 1;
            }

            int left_node_index = pred_obj.index * 2 + 1;
            int right_node_index = pred_obj.index * 2 + 2;
            PredictionObj left(tree_nodes[left_node_index].is_leaf, tree_nodes[left_node_index].is_self_feature,
                    tree_nodes[left_node_index].best_client_id, tree_nodes[left_node_index].best_feature_id,
                    tree_nodes[left_node_index].best_split_id, left_mark, left_node_index);
            PredictionObj right(tree_nodes[right_node_index].is_leaf, tree_nodes[right_node_index].is_self_feature,
                    tree_nodes[right_node_index].best_client_id, tree_nodes[right_node_index].best_feature_id,
                    tree_nodes[right_node_index].best_split_id, right_mark, right_node_index);

            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        }
    }

    sample_values.clear();
    sample_values.shrink_to_fit();

    return binary_vector;
}


void DecisionTree::test_accuracy(Client &client, float &accuracy) {
    // call the designated function according to solution type
    if (solution_type == Basic) {
        test_accuracy_basic(client, accuracy);
    } else {
        test_accuracy_enhanced(client, accuracy);
    }
}


void DecisionTree::test_accuracy_basic(Client &client, float &accuracy) {

    struct timeval testing_1, testing_2;
    double testing_time = 0;
    gettimeofday(&testing_1, NULL);

    logger(logger_out, "Begin test accuracy with basic solution on testing dataset\n");

    /**
     * Testing procedure:
     *  1. Organize the leaf label vector, and record the map between tree node index and leaf index
     *  2. For each sample in the testing dataset, search the whole tree and do the following:
     *      2.1 if meet feature that not belong to self, mark 1, and iteratively search left and right branches with 1
     *      2.2 if meet feature that belongs to self, compare with the split value, mark satisfied branch with 1 while
     *          the other branch with 0, and iteratively search left and right branches
     *      2.3 if meet the leaf node, record the corresponding leaf index with current value
     *  3. After each client obtaining a 0-1 vector of leaf nodes, do the following:
     *      3.1 the "client_num-1"-th client element-wise multiply with leaf label vector, and encrypt the vector,
     *          send to the next client, i.e., client_num-2
     *      3.2 every client on the Robin cycle updates the vector by element-wise homomorphic multiplication, and send to the next
     *      3.3 the last client, i.e., client 0 get the final encrypted vector and homomorphic add together, call share decryption
     *  4. If label is matched, correct_num += 1, otherwise, continue
     *  5. Return the final test accuracy by correct_num / testing_dataset.size()
     */

    // step 1: organize the leaf label vector, compute the map
    logger(logger_out, "internal_node_num = %d\n", internal_node_num);
    EncodedNumber *label_vector = new EncodedNumber[internal_node_num + 1];

    std::map<int, int> node_index_2_leaf_index_map;
    int leaf_cur_index = 0;
    for (int i = 0; i < pow(2, max_depth + 1) - 1; i++) {
        if (tree_nodes[i].is_leaf == 1) {
            node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
            label_vector[leaf_cur_index] = tree_nodes[i].label;  // record leaf label vector
            leaf_cur_index ++;
        }
    }

    // init predicted_label_vector
    std::vector<float> predicted_label_vector;
    for (int i = 0; i < testing_data.size(); i++) {
        predicted_label_vector.push_back(0.0);
    }

    // for each sample
    for (int i = 0; i < testing_data.size(); i++) {
        // compute binary vector for the current sample
        std::vector<int> binary_vector = compute_binary_vector(i, node_index_2_leaf_index_map);
        EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
        EncodedNumber *updated_label_vector;// = new EncodedNumber[binary_vector.size()];

        // update in Robin cycle, from the last client to client 0
        if (client.client_id == client.client_num - 1) {
            updated_label_vector = new EncodedNumber[binary_vector.size()];
            for (int j = 0; j < binary_vector.size(); j++) {
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
                djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], label_vector[j], encoded_binary_vector[j]);
            }
            // send to the next client
            std::string send_s;
            serialize_batch_sums(updated_label_vector, binary_vector.size(), send_s);
            client.send_long_messages(client.client_id - 1, send_s);
        } else if (client.client_id > 0) {
            std::string recv_s;
            client.recv_long_messages(client.client_id + 1, recv_s);
            int recv_size; // should be same as binary_vector.size()
            deserialize_sums_from_string(updated_label_vector, recv_size, recv_s);
            for (int j = 0; j < binary_vector.size(); j++) {
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
                djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
            }
            std::string resend_s;
            serialize_batch_sums(updated_label_vector, binary_vector.size(), resend_s);
            client.send_long_messages(client.client_id - 1, resend_s);
        } else {
            // the super client update the last, and aggregate before calling share decryption
            std::string final_recv_s;
            client.recv_long_messages(client.client_id + 1, final_recv_s);
            int final_recv_size;
            deserialize_sums_from_string(updated_label_vector, final_recv_size, final_recv_s);
            for (int j = 0; j < binary_vector.size(); j++) {
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
                djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
            }
        }

        // aggregate and call share decryption
        if (client.client_id == 0) {
            EncodedNumber *encrypted_aggregation = new EncodedNumber[1];
            encrypted_aggregation[0].set_float(client.m_pk->n[0], 0, 2 * FLOAT_PRECISION);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_aggregation[0], encrypted_aggregation[0]);
            for (int j = 0; j < binary_vector.size(); j++) {
                djcs_t_aux_ee_add(client.m_pk, encrypted_aggregation[0], encrypted_aggregation[0], updated_label_vector[j]);
            }

            EncodedNumber *decrypted_label = new EncodedNumber[1];
            client.share_batch_decrypt(encrypted_aggregation, decrypted_label, 1);
            //float decoded_label;
            decrypted_label[0].decode(predicted_label_vector[i]);
            //logger(logger_out, "decoded_label = %f while true label = %f\n", predicted_label_vector[i], testing_data_labels[i]);

            delete [] encrypted_aggregation;
            delete [] decrypted_label;
        } else {
            std::string s, response_s;
            client.recv_long_messages(0, s);
            client.decrypt_batch_piece(s, response_s, 0);
        }

        binary_vector.clear();
        binary_vector.shrink_to_fit();
        delete [] encoded_binary_vector;
        delete [] updated_label_vector;
    }

    // compute accuracy by the super client
    if (client.client_id == 0) {
        if (type == 0) {
            int correct_num = 0;
            for (int i = 0; i < testing_data.size(); i++) {
                if (predicted_label_vector[i] == testing_data_labels[i]) {
                    correct_num += 1;
                }
            }
            logger(logger_out, "correct_num = %d, testing_data_size = %d\n", correct_num, testing_data_labels.size());
            accuracy = (float) correct_num / (float) testing_data_labels.size();
        } else {
            accuracy = mean_squared_error(predicted_label_vector, testing_data_labels);
        }
    }

    gettimeofday(&testing_2, NULL);
    testing_time += (double)((testing_2.tv_sec - testing_1.tv_sec) * 1000 + (double)(testing_2.tv_usec - testing_1.tv_usec) / 1000);
    logger(logger_out, "Total testing computation time: %'.3f ms\n", testing_time);
    logger(logger_out, "Average testing computation time: %'.3f ms\n", testing_time / testing_data_labels.size());

    delete [] label_vector;
    logger(logger_out, "End test accuracy with basic solution on testing dataset\n");
}


void DecisionTree::test_accuracy_enhanced(Client &client, float &accuracy) {

    if (testing_data.size() == 0) {
        logger(logger_out, "No testing data\n");
        return;
    }

    // Prepare the secret shared models
    // TODO: since we do not count the model decryption into testing computation time, here we only simulate the model preparation

    // organize the leaf label vector, compute the vectors
    EncodedNumber *label_vector = new EncodedNumber[internal_node_num + 1];

    std::vector<int> leaf_index_nodes;
    std::vector<int> self_feature_nodes;
    std::vector<float> self_feature_thresholds;
    int leaf_cur_index = 0;
    int self_features_used = 0;
    for (int i = 0; i < pow(2, max_depth + 1) - 1; i++) {
        if (tree_nodes[i].is_leaf == 1) {
            leaf_index_nodes.push_back(i);
            label_vector[leaf_cur_index] = tree_nodes[i].label;  // record leaf label vector
            leaf_cur_index ++;
        } else if (tree_nodes[i].is_self_feature == 1) {
            self_feature_nodes.push_back(i);
            self_features_used += 1;
            int feature_id = tree_nodes[i].best_feature_id;
            int split_id = tree_nodes[i].best_split_id;
            self_feature_thresholds.push_back(features[feature_id].split_values[split_id]);
        } else {
            continue;
        }
    }

    int leaf_num = leaf_index_nodes.size();
    if (leaf_num != internal_node_num + 1) {
        logger(logger_out, "Leaf node number is incorrect\n");
        return;
    }

    // the super client call decrypt the label vector and will be sent to SPDZ as private inputs
    EncodedNumber * enc_label_shares = new EncodedNumber[leaf_num];
    std::vector<float> label_shares;
    if (client.client_id == 0) {
        for (int i = 0; i < leaf_num; i++) {
            enc_label_shares[i] = label_vector[i];
        }
        // receive the other clients encrypted shares and add to its own encrypted shares
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string recv_s;
                client.recv_long_messages(i, recv_s);
                EncodedNumber * deserialized_label_shares;// = new EncodedNumber[leaf_num];
                deserialize_sums_from_string(deserialized_label_shares, leaf_num, recv_s);
                for (int j = 0; j < leaf_num; j++) {
                    djcs_t_aux_ee_add(client.m_pk, enc_label_shares[j], enc_label_shares[j], deserialized_label_shares[j]);
                }
                delete [] deserialized_label_shares;
            }
        }
        // call share decrypt and convert to secret shares
        EncodedNumber * decrypted_label_shares = new EncodedNumber[leaf_num];
        client.share_batch_decrypt(enc_label_shares, decrypted_label_shares, leaf_num,
                ((optimization_type == Parallelism) || (optimization_type == All)));
        for (int j = 0; j < leaf_num; j++) {
            float x;
            decrypted_label_shares[j].decode(x);
            label_shares.push_back(x);
        }
        delete [] decrypted_label_shares;
    } else {
        // generate secret shares
        for (int i = 0; i < leaf_num; i++) {
            float label_share = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
            label_shares.push_back(0 - label_share);

            EncodedNumber encoded_label_share;
            encoded_label_share.set_float(client.m_pk->n[0], label_share, 2 * FLOAT_PRECISION);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, enc_label_shares[i], encoded_label_share);
        }

        // serialize and send to the super client
        std::string s;
        serialize_batch_sums(enc_label_shares, leaf_num, s);
        client.send_long_messages(0, s);

        // share decrypt
        std::string recv_s, response_s;
        client.recv_long_messages(0, recv_s);
        client.decrypt_batch_piece(recv_s, response_s, 0, ((optimization_type == Parallelism) || (optimization_type == All)));
    }


    struct timeval testing_1, testing_2;
    double testing_time = 0;
    gettimeofday(&testing_1, NULL);

    logger(logger_out, "Begin test accuracy with enhanced solution on testing dataset\n");

    /**
     * (Notice that this function only applied in single decision tree, currently not integrated into RF and GBDT)
     * Test procedure:
     * 1. The clients jointly decrypt the split value on the internal nodes and the labels on the leaf nodes
     *  should notice that the model decryption time should not be counted in the model prediction time
     * 2. The super client transmits the secret shares of its local feature split values and the labels to SPDZ parties,
     *  while every other client transmits the secret shares of its local feature split values to SPDZ parties
     * 3. Every client sends private inputs (i.e., local testing data) to SPDZ parties with the same order as step 2
     * 4. SPDZ parties do the following computations:
     *  (4.1) The SPDZ parties receive feature split shares of all the internal nodes and map to the initialized structure
     *  (4.2) The SPDZ parties receive secret shares of testing data
     *  (4.3) The SPDZ parties compute secret shares of binary vector with size |T| (|T| is the number of leaf nodes) for each sample
     *  (4.4) The SPDZ parties compute secret shared dot product of binary vector and labels, obtaining the secret shared predicted label
     *  (4.5) The SPDZ parties return and reveal the predicted labels to the super client
     * 5. The super client receives the plaintext predicted label results from the SPDZ parties and computes the accuracy
     *  using ground truth labels
     */

    // 1. prepare the testing data
    std::vector< std::vector<float> > self_testing_data;
    for (int i = 0; i < testing_data.size(); i++) {
        std::vector<float> sample;
        for (int j = 0; j < self_features_used; j++) {
            int feature_id = tree_nodes[self_feature_nodes[j]].best_feature_id;
            sample.push_back(testing_data[i][feature_id]);
        }
        self_testing_data.push_back(sample);
    }

    // 1. init SPDZ related information
    // init static gfp
    string prep_data_prefix = get_prep_dir(NUM_SPDZ_PARTIES, 128, gf2n::default_degree());
    //logger(logger_out, "prep_data_prefix = %s \n", prep_data_prefix.c_str());
    initialise_fields(prep_data_prefix);
    bigint::init_thread();
    // setup sockets
    std::vector<int> sockets = setup_sockets(NUM_SPDZ_PARTIES, client.client_id, client.host_names, SPDZ_PORT_NUM_DT_ENHANCED);

    // 2. Super client send leaf_index_nodes vector as public input to SPDZ parties
    if (client.client_id == 0) {
        // send the leaf num
        std::vector<int> param1;
        param1.push_back(leaf_num);
        send_public_values(param1, sockets, NUM_SPDZ_PARTIES);

        logger(logger_out, "The leaf num has been sent to SPDZ\n");

        // send the leaf nodes
        for (int i = 0; i < leaf_num; i++) {
            std::vector<int> param2;
            param2.push_back(leaf_index_nodes[i]);
            send_public_values(param2, sockets, NUM_SPDZ_PARTIES);
        }

        logger(logger_out, "The leaf nodes index have been sent to SPDZ\n");
    }

    // 3. Every client send label_shares vector as private input to SPDZ parties
    for (int i = 0; i < leaf_num; i++) {
        vector<float> x;
        x.push_back(label_shares[i]);
        send_private_batch_shares(x, sockets, NUM_SPDZ_PARTIES);
    }

    logger(logger_out, "The leaf label shares have been sent to SPDZ\n");

    // 4. Every client send self_feature_nodes vector as public input to SPDZ parties
    vector<int> public_param3;
    public_param3.push_back(self_features_used);
    send_public_values(public_param3, sockets, NUM_SPDZ_PARTIES);

    logger(logger_out, "The self internal node num = %d, has been sent to SPDZ\n", self_features_used);

    for (int i = 0; i < self_features_used; i++) {
        vector<int> public_param4;
        public_param4.push_back(self_feature_nodes[i]);
        send_public_values(public_param4, sockets, NUM_SPDZ_PARTIES);
    }

    logger(logger_out, "The self internal node indexes have been sent to SPDZ\n");

    for (int i = 0; i < self_features_used; i++) {
        vector<float> x;
        x.push_back(self_feature_thresholds[i]);
        send_private_batch_shares(x, sockets, NUM_SPDZ_PARTIES);
    }

    logger(logger_out, "The self internal node threshold values have been sent to SPDZ\n");

    // 5. Every client send self_testing_data matrix as private input to SPDZ parties
    logger(logger_out, "self testing data size = %d\n", self_testing_data.size());
    logger(logger_out, "self testing data [0].size = %d\n", self_testing_data[0].size());
    for (int i = 0; i < self_testing_data.size(); i++) {
        for (int j = 0; j < self_testing_data[0].size(); j++) {
            vector<float> x;
            x.push_back(self_testing_data[i][j]);
            send_private_batch_shares(x, sockets, NUM_SPDZ_PARTIES);
        }
    }

    logger(logger_out, "The testing data has been sent to SPDZ\n");

    // 6. Super client receive the revealed labels and compare to the ground truth labels
    std::vector<float> predicted_labels = receive_result(sockets, NUM_SPDZ_PARTIES, testing_data.size());

    if (type == 0) {
        int correct_num = 0;
        for (int i = 0; i < testing_data.size(); i++) {
            //logger(logger_out, "predicted_labels[%d] = %f, testing_data_labels[%d] = %f\n",
            //       i, predicted_labels[i], i, testing_data_labels[i]);
            if (rounded_comparison(predicted_labels[i], testing_data_labels[i])) {
                correct_num += 1;
            }
        }

        logger(logger_out, "correct_num = %d, testing_data_size = %d\n", correct_num, testing_data_labels.size());
        accuracy = (float) correct_num / (float) testing_data_labels.size();
    } else {
        accuracy = mean_squared_error(predicted_labels, testing_data_labels);
    }

    gettimeofday(&testing_2, NULL);
    testing_time += (double)((testing_2.tv_sec - testing_1.tv_sec) * 1000 + (double)(testing_2.tv_usec - testing_1.tv_usec) / 1000);
    logger(logger_out, "Total testing computation time: %'.3f ms\n", testing_time);
    logger(logger_out, "Average testing computation time: %'.3f ms\n", testing_time / testing_data_labels.size());

    predicted_labels.clear();
    predicted_labels.shrink_to_fit();
    self_testing_data.clear();
    self_testing_data.shrink_to_fit();
    delete [] label_vector;
    delete [] enc_label_shares;

    for (unsigned int i = 0; i < sockets.size(); i++) {
        close_client_socket(sockets[i]);
    }

    logger(logger_out, "End test accuracy with enhanced solution on testing dataset\n");
}


void DecisionTree::private_split_selection(Client &client, EncodedNumber *&result_iv, EncodedNumber *selection_iv,
                                           std::vector<std::vector<int> > split_iv_matrix, int sample_num, int split_num) {

    // selection_iv size is equal to the split num, where only [1] exists, the others are all [0]
    // split_iv_matrix size is equal to split num, while split_iv_matrix[0] size is equal to sample num
    // result_iv size is equal to sample num

    if (split_iv_matrix.size() == 0 || split_iv_matrix[0].size() == 0) {
        logger(logger_out, "Invalid split ivs\n");
    }

    omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for if((optimization_type == Parallelism) || (optimization_type == All))
    for (int i = 0; i < sample_num; i++) {
        // compute private selection for result_iv[i]
        result_iv[i].set_integer(client.m_pk->n[0], 0);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, result_iv[i], result_iv[i]);
        for (int j = 0; j < split_num; j++) {
            // convert homomorphic multiplication to homomorphic addition
            if (split_iv_matrix[j][i] == 1) {
                djcs_t_aux_ee_add(client.m_pk, result_iv[i], result_iv[i], selection_iv[j]);
            }
        }
    }
}


void DecisionTree::update_sample_iv(Client &client, int i_star, EncodedNumber *left_selection_result,
                                    EncodedNumber *right_selection_result, int node_index) {
    // 1. send left_selection_result and right_selection_result to the other clients
    // 2. convert sample_iv into secret shares
    // 3. aggregate the shares
    // 4. broadcast the final selection_iv for left branch and right branch

    int sample_num = tree_nodes[node_index].sample_size;

    if (client.client_id == i_star) {
        // step 1
        std::string left_selection_str, right_selection_str;
        serialize_batch_sums(left_selection_result, sample_num, left_selection_str);
        serialize_batch_sums(right_selection_result, sample_num, right_selection_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, left_selection_str);
                client.send_long_messages(i, right_selection_str);
            }
        }

        // step 2
        std::vector<int> sample_iv_shares;
        EncodedNumber *aggregated_enc_sample_iv_shares = new EncodedNumber[sample_num];
        for (int j = 0; j < sample_num; j++) {
            aggregated_enc_sample_iv_shares[j] = tree_nodes[node_index].sample_iv[j];
        }

        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string recv_enc_sample_iv_shares_str;
                EncodedNumber * recv_enc_sample_iv_shares;// = new EncodedNumber[sample_num];
                client.recv_long_messages(i, recv_enc_sample_iv_shares_str);
                deserialize_sums_from_string(recv_enc_sample_iv_shares, sample_num, recv_enc_sample_iv_shares_str);

                for (int j = 0; j < sample_num; j++) {
                    djcs_t_aux_ee_add(client.m_pk, aggregated_enc_sample_iv_shares[j],
                            aggregated_enc_sample_iv_shares[j], recv_enc_sample_iv_shares[j]);
                }

                delete [] recv_enc_sample_iv_shares;
            }
        }

        EncodedNumber * decrypted_sample_iv_shares = new EncodedNumber[sample_num];
        client.share_batch_decrypt(aggregated_enc_sample_iv_shares, decrypted_sample_iv_shares, sample_num,
                                   (optimization_type == Parallelism || optimization_type == All));

        for (int j = 0; j < sample_num; j++) {
            long x;
            decrypted_sample_iv_shares[j].decode(x);
            sample_iv_shares.push_back(x);
        }

        // step 3
        EncodedNumber * aggregated_updated_sample_iv_left = new EncodedNumber[sample_num];
        EncodedNumber * aggregated_updated_sample_iv_right = new EncodedNumber[sample_num];
        EncodedNumber * tmps = new EncodedNumber[sample_num];
        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
        for (int j = 0; j < sample_num; j++) {
            tmps[j].set_integer(client.m_pk->n[0], sample_iv_shares[j]);
            djcs_t_aux_ep_mul(client.m_pk, aggregated_updated_sample_iv_left[j], left_selection_result[j], tmps[j]);
            djcs_t_aux_ep_mul(client.m_pk, aggregated_updated_sample_iv_right[j], right_selection_result[j], tmps[j]);
        }

        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                EncodedNumber * recv_updated_left;// = new EncodedNumber[sample_num];
                EncodedNumber * recv_updated_right;// = new EncodedNumber[sample_num];
                std::string recv_updated_left_str, recv_updated_right_str;
                client.recv_long_messages(i, recv_updated_left_str);
                client.recv_long_messages(i, recv_updated_right_str);
                deserialize_sums_from_string(recv_updated_left, sample_num, recv_updated_left_str);
                deserialize_sums_from_string(recv_updated_right, sample_num, recv_updated_right_str);
                for (int j = 0; j < sample_num; j++) {
                    djcs_t_aux_ee_add(client.m_pk, aggregated_updated_sample_iv_left[j], aggregated_updated_sample_iv_left[j], recv_updated_left[j]);
                    djcs_t_aux_ee_add(client.m_pk, aggregated_updated_sample_iv_right[j], aggregated_updated_sample_iv_right[j], recv_updated_right[j]);
                }
                delete [] recv_updated_left;
                delete [] recv_updated_right;
            }
        }

        // step 4
        std::string updated_sample_iv_left_str, updated_sample_iv_right_str;
        serialize_batch_sums(aggregated_updated_sample_iv_left, sample_num, updated_sample_iv_left_str);
        serialize_batch_sums(aggregated_updated_sample_iv_right, sample_num, updated_sample_iv_right_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(i, updated_sample_iv_left_str);
                client.send_long_messages(i, updated_sample_iv_right_str);
            }
        }

        delete [] aggregated_enc_sample_iv_shares;
        delete [] decrypted_sample_iv_shares;
        delete [] aggregated_updated_sample_iv_left;
        delete [] aggregated_updated_sample_iv_right;
        delete [] tmps;
    } else {
        // step 1
        std::string recv_left_selection_str, recv_right_selection_str;
        client.recv_long_messages(i_star, recv_left_selection_str);
        client.recv_long_messages(i_star, recv_right_selection_str);
        deserialize_sums_from_string(left_selection_result, sample_num, recv_left_selection_str);
        deserialize_sums_from_string(right_selection_result, sample_num, recv_right_selection_str);

        // step 2
        std::vector<int> sample_iv_shares;
        EncodedNumber *enc_sample_iv_shares = new EncodedNumber[sample_num];
        for (int i = 0; i < sample_num; i++) {
            int iv_share = static_cast<int> (rand() % MAXIMUM_RAND_VALUE);
            sample_iv_shares.push_back(0 - iv_share);

            EncodedNumber encoded_iv_share;
            encoded_iv_share.set_integer(client.m_pk->n[0], iv_share);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, enc_sample_iv_shares[i], encoded_iv_share);
        }

        std::string enc_sample_iv_shares_str;
        serialize_batch_sums(enc_sample_iv_shares, sample_num, enc_sample_iv_shares_str);
        client.send_long_messages(i_star, enc_sample_iv_shares_str);

        std::string recv_share_decrypt_str, response_share_decrypt_str;
        client.recv_long_messages(i_star, recv_share_decrypt_str);
        client.decrypt_batch_piece(recv_share_decrypt_str, response_share_decrypt_str, i_star);

        // step 3
        EncodedNumber * tmps = new EncodedNumber[sample_num];
        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
        for (int j = 0; j < sample_num; j++) {
            tmps[j].set_integer(client.m_pk->n[0], sample_iv_shares[j]);
            djcs_t_aux_ep_mul(client.m_pk, left_selection_result[j], left_selection_result[j], tmps[j]);
            djcs_t_aux_ep_mul(client.m_pk, right_selection_result[j], right_selection_result[j], tmps[j]);
        }

        //serialize left_selection_result and right_selection_result to i_star client
        std::string left_update_shares_str, right_update_shares_str;
        serialize_batch_sums(left_selection_result, sample_num, left_update_shares_str);
        serialize_batch_sums(right_selection_result, sample_num, right_update_shares_str);
        client.send_long_messages(i_star, left_update_shares_str);
        client.send_long_messages(i_star, right_update_shares_str);

        // step 4 receive the final two sample ivs for the two branches
        EncodedNumber * updated_sample_iv_left;// = new EncodedNumber[sample_num];
        EncodedNumber * updated_sample_iv_right;// = new EncodedNumber[sample_num];
        std::string recv_updated_sample_iv_left_str, recv_updated_sample_iv_right_str;
        client.recv_long_messages(i_star, recv_updated_sample_iv_left_str);
        client.recv_long_messages(i_star, recv_updated_sample_iv_right_str);
        deserialize_sums_from_string(updated_sample_iv_left, sample_num, recv_updated_sample_iv_left_str);
        deserialize_sums_from_string(updated_sample_iv_right, sample_num, recv_updated_sample_iv_right_str);

        delete [] enc_sample_iv_shares;
        delete [] updated_sample_iv_left;
        delete [] updated_sample_iv_right;
        delete [] tmps;
    }
}


//void DecisionTree::test_indicator_vector_correctness() {
//    int * class_sums = new int[classes_num];
//    for (int i = 0; i < classes_num; i++) {
//        class_sums[i] = 0;
//    }
//    for (int i = 0; i < classes_num; i++) {
//        for (int j = 0; j < indicator_class_vecs[0].size(); j++) {
//            if (indicator_class_vecs[i][j] == 1) class_sums[i] += 1;
//        }
//    }
//    int total_sum = 0;
//    for (int i = 0; i < classes_num; i++) {
//        total_sum += class_sums[i];
//    }
//    logger(logger_out, "Total_sum = %d\n", total_sum);
//}
//
//
//void DecisionTree::test_sample_iv_update_correctness(Client &client, int node_index, int i_star) {
//
//    if (node_index * 2 + 2 >= pow(2, max_depth + 1) - 1) { return;}
//
//    // compute public key size in encoded number
//    mpz_t n;
//    mpz_init(n);
//    mpz_sub_ui(n, client.m_pk->g, 1);
//
//    EncodedNumber parent_node_sum, left_node_sum, right_node_sum;
//    parent_node_sum.set_integer(n, 0);
//    left_node_sum.set_integer(n, 0);
//    right_node_sum.set_integer(n, 0);
//    djcs_t_aux_encrypt(client.m_pk, client.m_hr, parent_node_sum, parent_node_sum);
//    djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_node_sum, left_node_sum);
//    djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_node_sum, right_node_sum);
//
//    for (int i = 0; i < training_data.size(); i++) {
//        djcs_t_aux_ee_add(client.m_pk, parent_node_sum, parent_node_sum, tree_nodes[node_index].sample_iv[i]);
//        djcs_t_aux_ee_add(client.m_pk, left_node_sum, left_node_sum, tree_nodes[2 * node_index + 1].sample_iv[i]);
//        djcs_t_aux_ee_add(client.m_pk, right_node_sum, right_node_sum, tree_nodes[2 * node_index + 2].sample_iv[i]);
//    }
//
//    // decrypt for checking equality
//    EncodedNumber * encrypted_sums = new EncodedNumber[3];
//    encrypted_sums[0] = parent_node_sum;
//    encrypted_sums[1] = left_node_sum;
//    encrypted_sums[2] = right_node_sum;
//    if (client.client_id == 2) {
//        EncodedNumber * decrypted_sums = new EncodedNumber[3];
//        client.share_batch_decrypt(encrypted_sums, decrypted_sums, 3);
//        long parent_sum_count, left_sum_count, right_sum_count;
//        decrypted_sums[0].decode(parent_sum_count);
//        decrypted_sums[1].decode(left_sum_count);
//        decrypted_sums[2].decode(right_sum_count);
//
//        logger(logger_out, "The parent node available sample num = %d, while the left child sample num = %d and "
//                       "the right child sample num = %d\n", parent_sum_count, left_sum_count, right_sum_count);
//
//        delete [] decrypted_sums;
//    } else {
//        std::string recv_s, response_s;
//        client.recv_long_messages(client.channels[2].get(), recv_s);
//        client.decrypt_batch_piece(recv_s, response_s, 2);
//    }
//
//    delete [] encrypted_sums;
//}
//
//
//void DecisionTree::test_encrypted_statistics_correctness(Client &client, EncodedNumber *stats, int size, int split_index) {
//
//    if (client.client_id == 1) {
//        EncodedNumber * decrypted_statistics = new EncodedNumber[size];
//        client.share_batch_decrypt(stats, decrypted_statistics, size);
//        long left_sum, right_sum;
//        float left_0_stat, right_0_stat, left_1_stat, right_1_stat;
//        decrypted_statistics[0].decode(left_sum);
//        decrypted_statistics[1].decode(right_sum);
//        decrypted_statistics[2].decode(left_0_stat);
//        decrypted_statistics[3].decode(right_0_stat);
//        decrypted_statistics[4].decode(left_1_stat);
//        decrypted_statistics[5].decode(right_1_stat);
//        logger(logger_out, "left_sum[%d] = %d, right_sum[%d] = %d\n",
//               split_index, left_sum, split_index, right_sum);
//        logger(logger_out, "stat[%d][0] = %f, stat[%d][1] = %f, stat[%d][2] = %f, stat[%d][3] = %f\n",
//               split_index, left_0_stat, split_index, right_0_stat, split_index, left_1_stat, split_index, right_1_stat);
//        delete [] decrypted_statistics;
//    } else {
//        std::string recv_s, response_s;
//        client.recv_long_messages(client.channels[1].get(), recv_s);
//        client.decrypt_batch_piece(recv_s, response_s, 1);
//    }
//
//}


void DecisionTree::intermediate_memory_free() {

    if (training_data.size() != 0) {
        training_data.clear();
        training_data.shrink_to_fit();
    }

//    if (training_data_labels.size() != 0) {
//        training_data_labels.clear();
//        training_data_labels.shrink_to_fit();
//    }

    if (feature_types.size() != 0) {
        feature_types.clear();
        feature_types.shrink_to_fit();
    }

    if (split_num_each_client.size() != 0) {
        split_num_each_client.clear();
        split_num_each_client.shrink_to_fit();
    }

    if (testing_data_labels.size() != 0) {
        testing_data_labels.clear();
        testing_data_labels.shrink_to_fit();
    }

    if (indicator_class_vecs.size() != 0) {
        indicator_class_vecs.clear();
        indicator_class_vecs.shrink_to_fit();
    }

    if (variance_stat_vecs.size() != 0) {
        variance_stat_vecs.clear();
        variance_stat_vecs.shrink_to_fit();
    }

    for (int i = 0; i < local_feature_num; i++) {
        // free split_ivs
        if (features[i].split_ivs_left.size() != 0) {
            features[i].split_ivs_left.clear();
            features[i].split_ivs_left.shrink_to_fit();
        }

        if (features[i].split_ivs_right.size() != 0) {
            features[i].split_ivs_right.clear();
            features[i].split_ivs_right.shrink_to_fit();
        }

        // free sorted_indexes
        if (features[i].sorted_indexes.size() != 0) {
            features[i].sorted_indexes.clear();
            features[i].sorted_indexes.shrink_to_fit();
        }

        // free original_feature_values
        if (features[i].original_feature_values.size() != 0) {
            features[i].original_feature_values.clear();
            features[i].original_feature_values.shrink_to_fit();
        }
    }

}


DecisionTree::~DecisionTree() {

    // free local data
    if (training_data.size() != 0) {
        training_data.clear();
        training_data.shrink_to_fit();
    }

    if (testing_data.size() != 0) {
        testing_data.clear();
        testing_data.shrink_to_fit();
    }

    if (feature_types.size() != 0) {
        feature_types.clear();
        feature_types.shrink_to_fit();
    }

    if (split_num_each_client.size() != 0) {
        split_num_each_client.clear();
        split_num_each_client.shrink_to_fit();
    }

    delete [] tree_nodes;
    delete [] features;

    // free labels if not empty
    if (training_data_labels.size() != 0) {
        training_data_labels.clear();
        training_data_labels.shrink_to_fit();
    }

    if (testing_data_labels.size() != 0) {
        testing_data_labels.clear();
        testing_data_labels.shrink_to_fit();
    }

    if (indicator_class_vecs.size() != 0) {
        indicator_class_vecs.clear();
        indicator_class_vecs.shrink_to_fit();
    }

    if (variance_stat_vecs.size() != 0) {
        variance_stat_vecs.clear();
        variance_stat_vecs.shrink_to_fit();
    }
}