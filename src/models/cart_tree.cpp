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

#include "../utils/spdz/spdz_util.h"

DecisionTree::DecisionTree() {}

DecisionTree::DecisionTree(int m_global_feature_num, int m_local_feature_num, int m_internal_node_num, int m_type, int m_classes_num,
                           int m_max_depth, int m_max_bins, int m_prune_sample_num, float m_prune_threshold) {

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
}


void DecisionTree::init_datasets(Client client, float split) {

    logger(stdout, "Begin init dataset\n");

    int training_data_size = client.sample_num * split;
    int testing_data_size = client.sample_num - training_data_size;

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    for (int i = 0; i < client.sample_num; i++) {
        data_indexes.push_back(i);
    }

    auto rng = std::default_random_engine();
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
                testingg_data_labels.push_back(client.labels[data_indexes[i]]);
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
            client.send_long_messages(client.channels[i].get(), s);
        }
    }


    // pre-compute indicator vectors or variance vectors for labels
    // here already assume that client_id == 0 (super client)
    if (type == 0) {

        // classification, compute binary vectors and store
        for (int i = 0; i < classes_num; i++) {
            std::vector<int> indicator_vec;
            for (int j = 0; j < training_data_labels.size(); j++) {
                if (training_data_labels[j] == (float) i) {
                    indicator_vec.push_back(1);
                } else {
                    indicator_vec.push_back(0);
                }
            }
            indicator_class_vecs.push_back(indicator_vec);
        }

    } else {

        // regression, compute variance necessary stats
        std::vector<float> label_square_vec;
        for (int j = 0; j < training_data_labels.size(); j++) {
            label_square_vec.push_back(training_data_labels[j] * training_data_labels[j]);
        }
        variance_stat_vecs.push_back(training_data_labels); // the first vector is the actual label vector
        variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector

    }

    logger(stdout, "End init dataset\n");
}


void DecisionTree::init_features() {

    logger(stdout, "Begin init features\n");

    for (int i = 0; i < local_feature_num; i++) {

        // 1. extract feature values of the i-th feature, compute samples_num
        // 2. check if distinct values number <= max_bins, if so, update splits_num as distinct number
        // 3. init feature, and assign to features[i]
        std::vector<float> feature_values;
        for (int j = 0; j < training_data.size(); j++) {
            feature_values.push_back(training_data[j][i]);
        }

        features[i] = new Feature(i, feature_types[i], max_bins, max_bins, feature_values, training_data.size());
    }

    logger(stdout, "End init features\n");
}


void DecisionTree::init_root_node(Client client) {

    // Note that for the root node, every client can init the encrypted sample mask vector
    // but the label vectors need to be received from the super client
    // assume that the global feature number is known beforehand
    tree_nodes[0].is_leaf = -1;
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

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, client.m_pk->g, 1);

    EncodedNumber tmp;
    tmp.set_integer(n, 1);

    // init encrypted mask vector on the root node
    for (int i = 0; i < training_data.size(); i++) {
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].sample_iv[i], tmp);
    }

    if (type == 0) {
        EncodedNumber max_impurity;
        max_impurity.set_float(n, MAX_IMPURITY);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].impurity, max_impurity);
    } else {
        EncodedNumber max_variance;
        max_variance.set_float(n, MAX_VARIANCE);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, tree_nodes[0].impurity, max_variance);
    }

    mpz_clear(n);
}


bool DecisionTree::check_pruning_conditions(Client client, int node_index,
        EncodedNumber ** & encrypted_label_vecs, EncodedNumber & label) {

    logger(stdout, "Check pruning conditions\n");

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, client.m_pk->g, 1);

    int is_satisfied = 0;
    std::string result_str;
    int recv_node_index;

    if (client.client_id == 0) {

        // super client check the pruning conditions
        // 1. available global feature num is 0
        // 2. the number of samples is less than a threshold
        // 3. the number of class is 1 (impurity == 0) or variance less than a threshold
        long available_num;

        EncodedNumber available_samples_num;
        available_samples_num.set_integer(n, 0);
        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            djcs_t_aux_ee_add(client.m_pk, available_samples_num, available_samples_num, tree_nodes[node_index].sample_iv[i]);
        }

        // check if available_samples_num is less than a threshold (prune_sample_num), together with impurity
        // TODO: currently assume decryption on the sum and impurity, should convert to shares for computation

        auto *encrypted_conditions = new EncodedNumber[2];
        auto *decrypted_conditions = new EncodedNumber[2];
        encrypted_conditions[0] = available_samples_num;
        encrypted_conditions[1] = tree_nodes[node_index].impurity;

        client.share_batch_decrypt(encrypted_conditions, decrypted_conditions, 2);

        decrypted_conditions[0].decode(available_num);

        if ((tree_nodes[node_index].depth == max_depth) ||
            (tree_nodes[node_index].available_global_feature_num == 0)) {
            // case 1
            is_satisfied = 1;
        } else {

            // communicate with SPDZ parties, to receive the label
            // or check if the pruning condition is satisfied

            // case 2
            if ((int) available_num < prune_sample_num) {
                is_satisfied = 1;
            } else {
                // case 3
                if (type == 0) {
                    // check if impurity is equal to 0
                    float impurity;
                    decrypted_conditions[1].decode(impurity);
                    if (impurity == 0.0) {
                        is_satisfied = 1;
                    }
                } else {
                    // check if variance is less than a threshold
                    float variance;
                    decrypted_conditions[1].decode(variance);
                    if (variance <= prune_threshold) {
                        is_satisfied = 1;
                    }
                }
            }
        }

        // pack pruning condition result pb string and sends to the other clients
        if (is_satisfied) {

            // compute label information
            // TODO: here should find majority class as label for classification, should modify latter
            // for classification, the label is djcs_t_aux_dot_product(labels, sample_ivs) / available_num
            // for regression, the label is djcs_t_aux_dot_product(labels, sample_ivs) / available_num
            float inv_available_num = 1.0 / (float) available_num;
            EncodedNumber inv_encoded;
            inv_encoded.set_float(n, inv_available_num);

            auto *encoded_labels = new EncodedNumber[training_data_labels.size()];
            for (int i = 0; i < training_data_labels.size(); i++) {
                encoded_labels[i].set_float(n, training_data_labels[i]);
            }

            EncodedNumber dot_product_res;
            djcs_t_aux_inner_product(client.m_pk, client.m_hr, dot_product_res,
                    tree_nodes[node_index].sample_iv, encoded_labels, training_data_labels.size());

            djcs_t_aux_ep_mul(client.m_pk, label, dot_product_res, inv_encoded);

            // send encrypted impurity and plaintext label
            serialize_pruning_condition_result(node_index, is_satisfied, encrypted_label_vecs, classes_num,
                    training_data_labels.size(), label, result_str);

            delete [] encoded_labels;

        } else {

            // not satisfied, send encrypted label vectors
            // tree_nodes[node_index].is_leaf = 0;
            for (int i = 0; i < classes_num; i++) {
                for (int j = 0; j < training_data_labels.size(); j++) {
                    EncodedNumber tmp;
                    tmp.set_integer(n, indicator_class_vecs[i][j]);
                    djcs_t_aux_ep_mul(client.m_pk, encrypted_label_vecs[i][j], tree_nodes[node_index].sample_iv[j], tmp);
                }
            }

            serialize_pruning_condition_result(node_index, is_satisfied, encrypted_label_vecs, classes_num,
                    training_data_labels.size(), label, result_str);
        }

        // send to the other client
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(client.channels[i].get(), result_str);
            }
        }

        delete [] encrypted_conditions;
        delete [] decrypted_conditions;

    } else {
        // decrypt required information for checking pruning conditions
        std::string s, response_s;
        client.recv_long_messages(client.channels[0].get(), s);
        client.decrypt_batch_piece(s, response_s, 0);

        // receive the result of the pruning conditions check
        // leave update to the outside
        client.recv_long_messages(client.channels[0].get(), result_str);

        deserialize_pruning_condition_result(recv_node_index, is_satisfied, encrypted_label_vecs, label, result_str);

        if (recv_node_index != node_index) {
            logger(stdout, "Tree node index does not match\n");
            exit(1);
        }
    }

    logger(stdout, "Pruning conditions not satisfied\n");

    return is_satisfied;
}


void DecisionTree::build_tree_node(Client client, int node_index) {

    logger(stdout, "Begin build tree\n");


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
     *  // 9. recursively build the next two tree node
     *
     * */

    if (node_index >= pow(2, max_depth + 1) - 1) {
        logger(stdout, "Node exceeds the maximum tree depth\n");
        exit(1);
    }

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, client.m_pk->g, 1);

    /** check pruning conditions are update tree node accordingly */
    // if pruning conditions are not satisfied (note that if satisfied, the handle is in the function)
    EncodedNumber label;
    auto **encrypted_label_vecs = new EncodedNumber*[classes_num];
    for (int i = 0; i < classes_num; i++) {
        encrypted_label_vecs[i] = new EncodedNumber[training_data_labels.size()];
    }

    if (check_pruning_conditions(client, node_index, encrypted_label_vecs, label)) {
        // satisfied
        tree_nodes[node_index].is_leaf = 1;
        tree_nodes[node_index].label = label;
        return;
    } else {
        // not satisfied
        internal_node_num += 1;
    }

    /** pruning conditions not satisfied, compute encrypted statistics locally */

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

    // note for regression, classes_num should be 2
    if (type == 1) {
        classes_num = 2;
    }

    EncodedNumber **global_encrypted_statistics;
    EncodedNumber *global_left_branch_sample_nums;
    EncodedNumber *global_right_branch_sample_nums;
    int global_split_num = 0;


    EncodedNumber **encrypted_statistics;
    EncodedNumber *encrypted_left_branch_sample_nums;
    EncodedNumber *encrypted_right_branch_sample_nums;

    std::vector< std::vector<float> > stats_shares;
    std::vector<float> left_sample_nums_shares;
    std::vector<float> right_sample_nums_shares;

    std::vector<int> client_split_nums;

    if (client.client_id == 0) {

        // compute local encrypted statistics
        if (local_splits_num == 0) {
            // do nothing
        } else {
            encrypted_statistics = new EncodedNumber*[local_splits_num];
            for (int i = 0; i < local_splits_num; i++) {
                encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
            }
            encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
            encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];

            // call compute function
            compute_encrypted_statistics(client, node_index, encrypted_statistics,
                    encrypted_label_vecs, encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums);
        }

        global_encrypted_statistics = new EncodedNumber*[MAX_GLOBAL_SPLIT_NUM];
        for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
            global_encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
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
                for (int j = 0; j < 2 * classes_num; j++) {
                    global_encrypted_statistics[i][j] = encrypted_statistics[i][j];
                }
            }
        }

        global_split_num += local_splits_num;

        // receive from the other clients of the encrypted statistics
        for (int i = 0; i < client.client_num; i++) {
            std::string recv_encrypted_statistics_str;
            if (i != client.client_id) {
                client.recv_long_messages(client.channels[i].get(), recv_encrypted_statistics_str);
            }

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
                    for (int k = 0; k < 2 * classes_num; k++) {
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
        }

        logger(stdout, "The global_split_num = %d\n", global_split_num);

        // send the total number of splits for the other clients to generate secret shares
        logger(stdout, "Send global split num to the other clients\n");
        std::string split_info_str;
        serialize_split_info(global_split_num, split_num_each_client, split_info_str);
        for (int i = 0; i < client.client_id; i++) {
            if (i != client.client_id) {
                client.send_long_messages(client.channels[i].get(), split_info_str);
            }
        }

    } else {

        if (local_splits_num == 0) {

            logger(stdout, "Local feature used up\n");

            std::string s;
            serialize_encrypted_statistics(client.client_id, node_index, local_splits_num, classes_num,
                    encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums, encrypted_statistics, s);
            client.send_long_messages(client.channels[0].get(), s);

        } else {

            encrypted_statistics = new EncodedNumber*[local_splits_num];
            for (int i = 0; i < local_splits_num; i++) {
                encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
            }
            encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
            encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];

            // call compute function
            compute_encrypted_statistics(client, node_index, encrypted_statistics,
                    encrypted_label_vecs, encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums);

            // send encrypted statistics to the super client
            std::string s;
            serialize_encrypted_statistics(client.client_id, node_index, local_splits_num, classes_num,
                    encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums, encrypted_statistics, s);

            client.send_long_messages(client.channels[0].get(), s);
        }

        logger(stdout, "Receive global split num from the super client\n");

        std::string recv_split_info_str;
        client.recv_long_messages(client.channels[0].get(), recv_split_info_str);
        deserialize_split_info(global_split_num, split_num_each_client, recv_split_info_str);

        logger(stdout, "The global_split_num = %d\n", global_split_num);

    }

    /** encrypted statistics computed finished, convert the encrypted values to secret shares */

    if (client.client_id == 0) {

        // receive encrypted shares from the other clients
        for (int i = 0; i < client.client_num; i++) {

            if (i != client.client_id) {

                std::string recv_s;
                EncodedNumber **recv_other_client_enc_shares;
                EncodedNumber *recv_left_shares;
                EncodedNumber *recv_right_shares;
                int recv_client_id, recv_node_index, recv_split_num, recv_classes_num;
                client.recv_long_messages(client.channels[i].get(), recv_s);
                deserialize_encrypted_statistics(recv_client_id, recv_node_index, recv_split_num, recv_classes_num,
                        recv_left_shares, recv_right_shares, recv_other_client_enc_shares, recv_s);

                // aggregate the data into global encrypted vectors
                for (int j = 0; j < global_split_num; j++) {

                    djcs_t_aux_ee_add(client.m_pk, global_left_branch_sample_nums[j],
                            global_left_branch_sample_nums[j], recv_left_shares[j]);
                    djcs_t_aux_ee_add(client.m_pk, global_right_branch_sample_nums[j],
                            global_right_branch_sample_nums[j], recv_right_shares[j]);

                    for (int k = 0; k < 2 * classes_num; k++) {
                        djcs_t_aux_ee_add(client.m_pk, global_encrypted_statistics[j][k],
                                global_encrypted_statistics[j][k], recv_other_client_enc_shares[j][k]);
                    }
                }

                delete [] recv_left_shares;
                delete [] recv_right_shares;
                for (int xx = 0; xx < global_split_num; xx++) {
                    delete [] recv_other_client_enc_shares[xx];
                }
            }
        }

        logger(stdout, "The global encrypted statistics are already aggregated\n");

        // call share decryption and convert to shares
        auto **decrypted_global_statistics = new EncodedNumber*[global_split_num];
        for (int i = 0; i < global_split_num; i++) {
            decrypted_global_statistics[i] = new EncodedNumber[2 * classes_num];
        }
        auto *decrypted_left_shares = new EncodedNumber[global_split_num];
        auto *decrypted_right_shares = new EncodedNumber[global_split_num];

        // decrypt left shares and set to shares vector
        client.share_batch_decrypt(global_left_branch_sample_nums, decrypted_left_shares, global_split_num);
        for (int i = 0; i < global_split_num; i++) {
            float x;
            decrypted_left_shares[i].decode(x);
            left_sample_nums_shares.push_back(x);
        }

        // decrypt right shares and set to shares vector
        client.share_batch_decrypt(global_right_branch_sample_nums, decrypted_right_shares, global_split_num);
        for (int i = 0; i < global_split_num; i++) {
            float x;
            decrypted_right_shares[i].decode(x);
            right_sample_nums_shares.push_back(x);
        }

        // decrypt encrypted statistics and set to shares vector
        for (int i = 0; i < global_split_num; i++) {
            std::vector<float> tmp;
            client.share_batch_decrypt(global_encrypted_statistics[i], decrypted_global_statistics[i], 2 * classes_num);
            for (int j = 0; j < 2 * classes_num; j++) {
                float x;
                decrypted_global_statistics[i][j].decode(x);
                tmp.push_back(x);
            }
            stats_shares.push_back(tmp);
        }

        delete [] decrypted_left_shares;
        delete [] decrypted_right_shares;
        for (int i = 0; i < global_split_num; i++) {
            delete [] decrypted_global_statistics[i];
        }

        logger(stdout, "Shares decrypt succeed, currently all the client shares are obtained\n");

    } else {

        // generate random shares, encrypt, and send to the super client

        global_encrypted_statistics = new EncodedNumber*[global_split_num];
        for (int i = 0; i < global_split_num; i++) {
            global_encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
        }
        global_left_branch_sample_nums = new EncodedNumber[global_split_num];
        global_right_branch_sample_nums = new EncodedNumber[global_split_num];

        for (int i = 0; i < global_split_num; i++) {

            float r_left = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
            float r_right = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
            left_sample_nums_shares.push_back(0 - r_left);
            right_sample_nums_shares.push_back(0 - r_right);

            EncodedNumber a_left, a_right;
            a_left.set_float(n, r_left);
            a_right.set_float(n, r_right);

            djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_left_branch_sample_nums[i], a_left);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_right_branch_sample_nums[i], a_right);


            std::vector<float> tmp;

            for (int j = 0; j < 2 * classes_num; j++) {
                float r_stat = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
                tmp.push_back(0 - r_stat);

                EncodedNumber a_stat;
                a_stat.set_float(n, r_stat);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, global_encrypted_statistics[i][j], a_stat);
            }

            stats_shares.push_back(tmp);
        }

        // serialize encrypted statistics and send to the super client
        std::string s_enc_shares;
        serialize_encrypted_statistics(client.client_id, node_index, global_split_num, classes_num,
                global_left_branch_sample_nums, global_right_branch_sample_nums, global_encrypted_statistics, s_enc_shares);
        client.send_long_messages(client.channels[0].get(), s_enc_shares);


        // receive share decrypt information, and decrypt the corresponding information
        std::string s_left_shares, response_s_left_shares, s_right_shares, response_s_right_shares;
        client.recv_long_messages(client.channels[0].get(), s_left_shares);
        client.decrypt_batch_piece(s_left_shares, response_s_left_shares, 0);

        client.recv_long_messages(client.channels[0].get(), s_right_shares);
        client.decrypt_batch_piece(s_right_shares, response_s_right_shares, 0);

        for (int i = 0; i < global_split_num; i++) {
            std::string s_stat_shares, response_s_stat_shares;
            client.recv_long_messages(client.channels[0].get(), s_stat_shares);
            client.decrypt_batch_piece(s_stat_shares, response_s_stat_shares, 0);
        }
    }

    logger(stdout, "Conversion to secret shares succeed\n");

    /** secret shares conversion finished, talk to SPDZ parties for MPC computations */

    // receive result from the SPDZ parties
    // TODO: communicate with SPDZ parties
    int index_in_global_split_num;
    float impurity_left_branch, impurity_right_branch;



    /** update tree nodes, including sample iv for the next tree node computation */

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
            } else {
                index_star_tmp = index_star_tmp - features[feature_id].num_splits;
            }
        }

        // now we have (i_*, j_*, s_*), retrieve s_*-th split ivs and update sample_ivs of two branches

        EncodedNumber left_impurity, right_impurity;
        left_impurity.set_float(n, impurity_left_branch);
        right_impurity.set_float(n, impurity_right_branch);

        // update current node index for prediction
        tree_nodes[node_index].is_self_feature = 1;
        tree_nodes[node_index].best_client_id = i_star;
        tree_nodes[node_index].best_feature_id = j_star;
        tree_nodes[node_index].best_split_id = s_star;

        tree_nodes[left_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[right_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[left_child_index].impurity = left_impurity;
        tree_nodes[right_child_index].impurity = right_impurity;
        tree_nodes[left_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[right_child_index].sample_size = tree_nodes[node_index].sample_size;

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

        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            EncodedNumber left, right;
            left.set_integer(n, split_left_iv[i]);
            right.set_integer(n, split_right_iv[i]);
            djcs_t_aux_ep_mul(client.m_pk, tree_nodes[left_child_index].sample_iv[i], tree_nodes[node_index].sample_iv[i], left);
            djcs_t_aux_ep_mul(client.m_pk, tree_nodes[right_child_index].sample_iv[i], tree_nodes[node_index].sample_iv[i], right);
        }

        // serialize and send to the other clients
        std::string update_str;
        serialize_update_info(client.client_id, client.client_id, j_star, s_star, left_impurity, right_impurity,
                tree_nodes[left_child_index].sample_iv, tree_nodes[right_child_index].sample_iv,
                tree_nodes[node_index].sample_size, update_str);
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                client.send_long_messages(client.channels[i].get(), update_str);
            }
        }

    } else {

        // receive from i_star client and update
        std::string recv_update_str;
        client.recv_long_messages(client.channels[i_star].get(), recv_update_str);

        // deserialize and update
        int recv_source_client_id, recv_best_client_id, recv_best_feature_id, recv_best_split_id;
        EncodedNumber recv_left_impurity, recv_right_impurity;
        EncodedNumber *recv_left_sample_iv, *recv_right_sample_iv;
        deserialize_update_info(recv_source_client_id, recv_best_client_id, recv_best_feature_id, recv_best_split_id,
                recv_left_impurity, recv_right_impurity, recv_left_sample_iv, recv_right_sample_iv, recv_update_str);

        // update tree nodes

        if (i_star != recv_best_client_id) {
            logger(stdout, "Suspicious message\n");
        }

        // update current node index for prediction
        tree_nodes[node_index].is_self_feature = 0;
        tree_nodes[node_index].best_client_id = recv_best_client_id;
        tree_nodes[node_index].best_feature_id = recv_best_feature_id;
        tree_nodes[node_index].best_split_id = recv_best_split_id;

        tree_nodes[left_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[right_child_index].depth = tree_nodes[node_index].depth + 1;
        tree_nodes[left_child_index].impurity = recv_left_impurity;
        tree_nodes[right_child_index].impurity = recv_right_impurity;
        tree_nodes[left_child_index].sample_size = tree_nodes[node_index].sample_size;
        tree_nodes[right_child_index].sample_size = tree_nodes[node_index].sample_size;

        tree_nodes[left_child_index].sample_iv = new EncodedNumber[tree_nodes[left_child_index].sample_size];
        tree_nodes[right_child_index].sample_iv = new EncodedNumber[tree_nodes[right_child_index].sample_size];

        for (int i = 0; i < tree_nodes[node_index].sample_size; i++) {
            tree_nodes[left_child_index].sample_iv[i] = recv_left_sample_iv[i];
            tree_nodes[right_child_index].sample_iv[i] = recv_right_sample_iv[i];
        }

        delete [] recv_left_sample_iv;
        delete [] recv_right_sample_iv;
    }

    /** recursively build the next child tree nodes */

    build_tree_node(client, left_child_index);
    build_tree_node(client, right_child_index);

    /** free memory used */

    delete [] encrypted_left_branch_sample_nums;
    delete [] encrypted_right_branch_sample_nums;
    delete [] global_left_branch_sample_nums;
    delete [] global_right_branch_sample_nums;

    if (local_splits_num != 0) {
        for (int i = 0; i < local_splits_num; i++) {
            delete [] encrypted_statistics[i];
        }
    }

    for (int i = 0; i < global_split_num; i++) {
        delete [] global_encrypted_statistics[i];
    }

    std::vector<float>().swap(left_sample_nums_shares);
    std::vector<float>().swap(right_sample_nums_shares);
    std::vector< std::vector<float> >().swap(stats_shares);

    logger(stdout, "End build tree\n");
}

// TODO: this function could be optimized
void DecisionTree::compute_encrypted_statistics(Client client, int node_index,
        EncodedNumber ** & encrypted_statistics, EncodedNumber ** encrypted_label_vecs,
        EncodedNumber * & encrypted_left_sample_nums, EncodedNumber * & encrypted_right_sample_nums) {

    logger(stdout, "Begin compute encrypted statistics\n");

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, client.m_pk->g, 1);

    int split_index = 0;
    int available_feature_num = tree_nodes[node_index].available_feature_ids.size();
    int sample_num = features[0].split_ivs_left[0].size();

    /**
     * splits of features are flatted, classes_num * 2 are for left and right
     */

    for (int j = 0; j < available_feature_num; j++) {

        int feature_id = tree_nodes[node_index].available_feature_ids[j];

        for (int s = 0; s < features[feature_id].num_splits; s++) {

            // compute encrypted statistics (left branch and right branch) for the current split
            std::vector<int> left_iv = features[feature_id].split_ivs_left[s];
            std::vector<int> right_iv = features[feature_id].split_ivs_right[s];

            // compute encrypted branch sample nums for this split by dot product between split iv and sample_iv

            for (int c = 0; c < classes_num; c++) {

                EncodedNumber left_stat, right_stat;
                left_stat.set_integer(n, 0);
                right_stat.set_integer(n, 0);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, left_stat, left_stat);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, right_stat, right_stat);

                //int sample_num = left_iv.size();
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
    }

    logger(stdout, "End compute encrypted impurity\n");
}


void DecisionTree::predict(Client client, int sample_id) {

    logger(stdout, "Begin predict a sample id = %d\n", sample_id);

    logger(stdout, "End predict a sample id = %d\n", sample_id);
}


DecisionTree::~DecisionTree() {

    // free local data
    std::vector< std::vector<float> >().swap(training_data);
    std::vector< std::vector<float> >().swap(testing_data);
    std::vector<int>().swap(feature_types);
    std::vector<int>().swap(split_num_each_client);

    // free labels if not empty
    if (training_data_labels.size() != 0) {
        std::vector<float>().swap(training_data_labels);
    }

    if (testingg_data_labels.size() != 0) {
        std::vector<float>().swap(testingg_data_labels);
    }

    if (indicator_class_vecs.size() != 0) {
        std::vector< std::vector<int> >().swap(indicator_class_vecs);
    }

    if (variance_stat_vecs.size() != 0) {
        std::vector< std::vector<float> >().swap(variance_stat_vecs);
    }
}