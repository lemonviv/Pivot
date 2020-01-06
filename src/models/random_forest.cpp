#include "random_forest.h"
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

#include "../utils/spdz/spdz_util.h"

RandomForest::RandomForest() {
    num_trees = NUM_TREES;
}

RandomForest::RandomForest(int m_tree_num, int m_global_feature_num, int m_local_feature_num, int m_internal_node_num, int m_type, int m_classes_num,
                           int m_max_depth, int m_max_bins, int m_prune_sample_num, float m_prune_threshold, int solution_type, int optimization_type) {
    num_trees = m_tree_num;
    forest.reserve(num_trees);
    for (int i = 0; i < num_trees; ++i) {
        forest.emplace_back(m_global_feature_num, m_local_feature_num, m_internal_node_num, m_type, m_classes_num,
                           m_max_depth, m_max_bins, m_prune_sample_num, m_prune_threshold, solution_type, optimization_type);
    }
    logger(stdout, "Init %d trees in the random forest\n", num_trees);
}


void RandomForest::init_datasets(Client & client, float split) {

    logger(stdout, "Begin init dataset\n");

    int training_data_size = client.sample_num * split;

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    for (int i = 0; i < client.sample_num; i++) {
        data_indexes.push_back(i);
    }

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
            client.send_long_messages(client.channels[i].get(), s);
        }
    }
    logger(stdout, "End init dataset\n");
}


void RandomForest::init_datasets_with_indexes(Client & client, int new_indexes[], float split) {
    logger(stdout, "Begin init dataset with indexes\n");

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

    logger(stdout, "End init dataset with indexes\n");
}


void RandomForest::shuffle_and_assign_training_data(int tree_id, Client & client, float sample_rate) {
    logger(stdout, "Begin shuffle training dataset for Tree[%d]\n", tree_id);

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    for (int i = 0; i < training_data.size(); i++) {
        data_indexes.push_back(i);
    }

    std::random_device rd;
    std::default_random_engine rng(rd());
    //auto rng = std::default_random_engine();
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);

    // sample training data for the decision tree
    int sampled_training_data_size = data_indexes.size() * sample_rate;
    data_indexes.resize(sampled_training_data_size);

    // assign the training dataset and labels
    for (int i = 0; i < data_indexes.size(); i++) {
        forest[tree_id].training_data.push_back(training_data[data_indexes[i]]);
        if (client.has_label) {
            forest[tree_id].training_data_labels.push_back(training_data_labels[data_indexes[i]]);
        }
    }

    int *new_indexes = new int[data_indexes.size()];
    for (int i = 0; i < data_indexes.size(); i++) {
        new_indexes[i] = data_indexes[i];
    }

    logger(stdout, "new_indexes size = %d\n", data_indexes.size());

    // send the data_indexes to the other client, and the other client shuffles the training data in the same way
    for (int i = 0; i < client.client_num; i++) {
        if (i != client.client_id) {
            std::string s;
            serialize_batch_ids(new_indexes, data_indexes.size(), s);
            client.send_long_messages(client.channels[i].get(), s);
        }
    }

    delete [] new_indexes;

    // pre-compute indicator vectors or variance vectors for labels
    // here already assume that client_id == 0 (super client)
    if (forest[tree_id].type == 0) {

        // classification, compute binary vectors and store
        for (int i = 0; i < forest[tree_id].classes_num; i++) {
            std::vector<int> indicator_vec;
            for (int j = 0; j < forest[tree_id].training_data_labels.size(); j++) {
                if (forest[tree_id].training_data_labels[j] == (float) i) {
                    indicator_vec.push_back(1);
                } else {
                    indicator_vec.push_back(0);
                }
            }
            forest[tree_id].indicator_class_vecs.push_back(indicator_vec);
        }

    } else {
        // regression, compute variance necessary stats
        std::vector<float> label_square_vec;
        for (int j = 0; j < forest[tree_id].training_data_labels.size(); j++) {
            label_square_vec.push_back(forest[tree_id].training_data_labels[j] * forest[tree_id].training_data_labels[j]);
        }
        forest[tree_id].variance_stat_vecs.push_back(forest[tree_id].training_data_labels); // the first vector is the actual label vector
        forest[tree_id].variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector
    }

    logger(stdout, "End shuffle training dataset\n");
}


void RandomForest::shuffle_and_assign_training_data_with_indexes(int tree_id, Client & client, int new_indexes[], float sample_rate) {

    logger(stdout, "Begin shuffle training dataset with indexes for Tree[%d]\n", tree_id);

    int sampled_training_data_size = training_data.size() * sample_rate;

    // assign the training dataset and labels
    for (int i = 0; i < sampled_training_data_size; i++) {
        forest[tree_id].training_data.push_back(training_data[new_indexes[i]]);
        if (client.has_label) {
            forest[tree_id].training_data_labels.push_back(training_data_labels[new_indexes[i]]);
        }
    }

    logger(stdout, "End shuffle training dataset with indexes\n");
}


void RandomForest::build_forest(Client & client, float sample_rate) {
    logger(stdout, "Begin build forest\n");
    for (int i = 0; i < num_trees; ++i) {
        if (client.client_id == 0) {
            shuffle_and_assign_training_data(i, client, sample_rate);
        } else {
            int sampled_size = training_data.size() * sample_rate;
            int *new_indexes = new int[sampled_size];
            std::string recv_s;
            client.recv_long_messages(client.channels[0].get(), recv_s);
            deserialize_ids_from_string(new_indexes, recv_s);
            shuffle_and_assign_training_data_with_indexes(i, client, new_indexes, sample_rate);

            delete [] new_indexes;
        }
        forest[i].init_features();
        forest[i].init_root_node(client);
        forest[i].build_tree_node(client, 0);
    }

    logger(stdout, "End build forest\n");
}


std::vector<int> RandomForest::compute_binary_vector(int tree_id, int sample_id, std::map<int, int> node_index_2_leaf_index_map) {

    vector<float> sample_values = testing_data[sample_id];
    std::vector<int> binary_vector(forest[tree_id].internal_node_num + 1);

    // traverse the whole tree iteratively, and compute binary_vector
    std::stack<PredictionObj> traverse_prediction_objs;
    PredictionObj prediction_obj(forest[tree_id].tree_nodes[0].is_leaf, forest[tree_id].tree_nodes[0].is_self_feature, forest[tree_id].tree_nodes[0].best_client_id,
            forest[tree_id].tree_nodes[0].best_feature_id, forest[tree_id].tree_nodes[0].best_split_id, 1, 0);
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

            PredictionObj left(forest[tree_id].tree_nodes[left_node_index].is_leaf, forest[tree_id].tree_nodes[left_node_index].is_self_feature,
                    forest[tree_id].tree_nodes[left_node_index].best_client_id, forest[tree_id].tree_nodes[left_node_index].best_feature_id,
                    forest[tree_id].tree_nodes[left_node_index].best_split_id, pred_obj.mark, left_node_index);
            PredictionObj right(forest[tree_id].tree_nodes[right_node_index].is_leaf, forest[tree_id].tree_nodes[right_node_index].is_self_feature,
                    forest[tree_id].tree_nodes[right_node_index].best_client_id, forest[tree_id].tree_nodes[right_node_index].best_feature_id,
                    forest[tree_id].tree_nodes[right_node_index].best_split_id, pred_obj.mark, right_node_index);
            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        } else {
            // is self feature, retrieve split value and compare
            traverse_prediction_objs.pop();
            int feature_id = pred_obj.best_feature_id;
            int split_id = pred_obj.best_split_id;
            float split_value = forest[tree_id].features[feature_id].split_values[split_id];
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
            PredictionObj left(forest[tree_id].tree_nodes[left_node_index].is_leaf, forest[tree_id].tree_nodes[left_node_index].is_self_feature,
                    forest[tree_id].tree_nodes[left_node_index].best_client_id, forest[tree_id].tree_nodes[left_node_index].best_feature_id,
                    forest[tree_id].tree_nodes[left_node_index].best_split_id, left_mark, left_node_index);
            PredictionObj right(forest[tree_id].tree_nodes[right_node_index].is_leaf, forest[tree_id].tree_nodes[right_node_index].is_self_feature,
                    forest[tree_id].tree_nodes[right_node_index].best_client_id, forest[tree_id].tree_nodes[right_node_index].best_feature_id,
                    forest[tree_id].tree_nodes[right_node_index].best_split_id, right_mark, right_node_index);

            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        }
    }

    return binary_vector;
}


void RandomForest::test_accuracy(Client & client, float & accuracy) {
    logger(stdout, "Begin test accuracy on testing dataset\n");

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, client.m_pk->g, 1);

    int correct_num = 0;
    // for each sample
    for (int i = 0; i < testing_data.size(); ++i) {
        float cumulated_label = 0;
        //  for each decision tree
        for (int tree_index = 0; tree_index < num_trees; ++tree_index) {
            // logger(stdout, "Processing tree[%d]:\n", tree_index);

            // step 1: organize the leaf label vector, compute the map
            EncodedNumber *label_vector = new EncodedNumber[forest[tree_index].internal_node_num + 1];

            std::map<int, int> node_index_2_leaf_index_map;
            int leaf_cur_index = 0;
            for (int j = 0; j < pow(2, forest[tree_index].max_depth + 1) - 1; j++) {
                if (forest[tree_index].tree_nodes[j].is_leaf == 1) {
                    node_index_2_leaf_index_map.insert(std::make_pair(j, leaf_cur_index));
                    label_vector[leaf_cur_index] = forest[tree_index].tree_nodes[j].label;  // record leaf label vector
                    leaf_cur_index ++;
                }
            }
            // compute binary vector for the current sample
            std::vector<int> binary_vector = compute_binary_vector(tree_index, i, node_index_2_leaf_index_map);
            EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
            EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];
            // update in Robin cycle, from the last client to client 0
            if (client.client_id == client.client_num - 1) {

                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], label_vector[j], encoded_binary_vector[j]);
                }
                // send to the next client
                std::string send_s;
                serialize_batch_sums(updated_label_vector, binary_vector.size(), send_s);
                client.send_long_messages(client.channels[client.client_id - 1].get(), send_s);

            } else if (client.client_id > 0) {

                std::string recv_s;
                client.recv_long_messages(client.channels[client.client_id + 1].get(), recv_s);
                int recv_size; // should be same as binary_vector.size()
                deserialize_sums_from_string(updated_label_vector, recv_size, recv_s);
                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
                }

                std::string resend_s;
                serialize_batch_sums(updated_label_vector, binary_vector.size(), resend_s);
                client.send_long_messages(client.channels[client.client_id - 1].get(), resend_s);

            } else {

                // the super client update the last, and aggregate before calling share decryption
                std::string final_recv_s;
                client.recv_long_messages(client.channels[client.client_id + 1].get(), final_recv_s);
                int final_recv_size;
                deserialize_sums_from_string(updated_label_vector, final_recv_size, final_recv_s);
                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
                }
            }
            // aggregate and call share decryption
            if (client.client_id == 0) {

                EncodedNumber *encrypted_aggregation = new EncodedNumber[1];
                encrypted_aggregation[0].set_float(n, 0, 2 * FLOAT_PRECISION);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_aggregation[0], encrypted_aggregation[0]);
                for (int j = 0; j < binary_vector.size(); j++) {
                    djcs_t_aux_ee_add(client.m_pk, encrypted_aggregation[0], encrypted_aggregation[0], updated_label_vector[j]);
                }

                EncodedNumber *decrypted_label = new EncodedNumber[1];
                client.share_batch_decrypt(encrypted_aggregation, decrypted_label, 1);

                float decoded_label;
                decrypted_label->decode(decoded_label);

                float rounded_decoded_label;
                if (decoded_label >= 0.5) {
                    rounded_decoded_label = 1.0;
                } else {
                    rounded_decoded_label = 0.0;
                }

                // logger(stdout, "Tree[%d] decoded_label = %f\n", tree_index, decoded_label);

                cumulated_label += rounded_decoded_label;

                delete [] encrypted_aggregation;
                delete [] decrypted_label;

            } else {
                std::string s, response_s;
                client.recv_long_messages(client.channels[0].get(), s);
                client.decrypt_batch_piece(s, response_s, 0);
            }
            delete [] encoded_binary_vector;
            delete [] updated_label_vector;
            delete [] label_vector;
        }
        if (client.client_id == 0) {
            cumulated_label /= num_trees;
            if (cumulated_label >= 0.5) {
                cumulated_label = 1.0;
            } else {
                cumulated_label = 0.0;
            }
            if (cumulated_label == (float) testing_data_labels[i]) {
                correct_num += 1;
            }
        }
    }
    logger(stdout, "correct_num: %d\n", correct_num);
    logger(stdout, "size: %d\n", testing_data_labels.size());
    accuracy = (float) correct_num / (float) testing_data_labels.size();
    logger(stdout, "End test accuracy on testing dataset\n");
}


RandomForest::~RandomForest() {

    // free local data
    forest.clear();
    forest.shrink_to_fit();

    training_data.clear();
    training_data.shrink_to_fit();

    testing_data.clear();
    testing_data.shrink_to_fit();

    // free labels if not empty
    if (training_data_labels.size() != 0) {
        training_data_labels.clear();
        training_data_labels.shrink_to_fit();
    }

    if (testing_data_labels.size() != 0) {
        testing_data_labels.clear();
        testing_data_labels.shrink_to_fit();
    }
}
