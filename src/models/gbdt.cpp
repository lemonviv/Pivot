//
// Created by wuyuncheng on 12/1/20.
//

#include "gbdt.h"
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
#include "../utils/score.h"
#include "../utils/spdz/spdz_util.h"

GBDT::GBDT() {}

GBDT::GBDT(int m_tree_num, int m_global_feature_num, int m_local_feature_num, int m_internal_node_num, int m_type,
           int m_classes_num, int m_max_depth, int m_max_bins, int m_prune_sample_num, float m_prune_threshold,
           int solution_type, int optimization_type) {

    num_trees = m_tree_num;
    gbdt_type = m_type;
    if (gbdt_type == 1) {
        classes_num = 1;
    } else {
        classes_num = m_classes_num;
    }
    learning_rates.reserve(num_trees);
    for (int i = 0; i < num_trees; i++) {
        learning_rates.emplace_back(1.0);
    }
    forest_size = classes_num * num_trees;
    forest.reserve(forest_size);
    for (int i = 0; i < forest_size; ++i) {
        forest.emplace_back(m_global_feature_num, m_local_feature_num, m_internal_node_num, 1, m_classes_num,
                m_max_depth, m_max_bins, m_prune_sample_num, m_prune_threshold, solution_type, optimization_type);
    }

    logger(stdout, "GBDT_type = %d, init %d trees in the GBDT\n", gbdt_type, forest_size);
}


void GBDT::init_datasets(Client & client, float split) {

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
    delete [] new_indexes;
}


void GBDT::init_datasets_with_indexes(Client & client, int new_indexes[], float split) {
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


void GBDT::init_single_tree_data(Client &client, int class_id, int tree_id, std::vector<float> cur_predicted_labels) {

    int real_tree_id = class_id * num_trees + tree_id;

    forest[real_tree_id].training_data = training_data;
    forest[real_tree_id].testing_data = testing_data;
    forest[real_tree_id].classes_num = 2;  // for regression, the classes num is set to 2 for y and y^2

    if (client.client_id == 0) {
        if (tree_id == 0) { // just copy the original labels
            if (gbdt_type == 1) {
                forest[real_tree_id].training_data_labels = training_data_labels;
            } else {
                // one-hot label encoder
                for (int i = 0; i < training_data.size(); i++) {
                    if ((float) training_data_labels[i] == class_id) {
                        forest[real_tree_id].training_data_labels.push_back(1.0);
                    } else {
                        forest[real_tree_id].training_data_labels.push_back(0.0);
                    }
                }
            }
        } else { // should use the predicted labels of first tree
            for (int i = 0; i < training_data.size(); i++) {
                forest[real_tree_id].training_data_labels.push_back(forest[class_id * num_trees].training_data_labels[i] - cur_predicted_labels[i]);
            }
        }

        // pre-compute indicator vectors or variance vectors for labels
        // here already assume that client_id == 0 (super client)
        if (forest[real_tree_id].type == 0) {
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
                forest[real_tree_id].indicator_class_vecs.push_back(indicator_vec);

                indicator_vec.clear();
                indicator_vec.shrink_to_fit();
            }
            for (int i = 0; i < classes_num; i++) {
                logger(stdout, "Class %d sample num = %d\n", i, sample_num_per_class[i]);
            }

        } else {
            // regression, compute variance necessary stats
            std::vector<float> label_square_vec;
            for (int j = 0; j < training_data_labels.size(); j++) {
                label_square_vec.push_back(training_data_labels[j] * training_data_labels[j]);
            }
            forest[real_tree_id].variance_stat_vecs.push_back(training_data_labels); // the first vector is the actual label vector
            forest[real_tree_id].variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector

            label_square_vec.clear();
            label_square_vec.shrink_to_fit();
        }
    }

}


void GBDT::build_gbdt(Client &client) {
    /**
     * 1. For regression, build as follows:
     *  (1) from tree 0 to tree max, init a decision tree; if client id == 0, init with difference label
     *  (2) build a decision tree using building blocks in cart_tree.h
     *  (3) after building the current tree, compute the predicted labels for the current training dataset for the next tree
     *
     * 2. For classification, build as follows:
     *  (1) for each class, convert to the one-hot encoding dataset, init classes_num forests, each forest init the first tree
     *  (2) from tree 0 to tree max, build iteratively using building blocks in cart_tree.h
     *  (3) after building trees in the current iteration, compute the predicted distribution for the training dataset, and compute
     *      the losses for init the difference of training labels in the trees of the next iteration
     */

    logger(stdout, "Begin to build GBDT model\n");

    // this vector is to store the predicted labels of current iteration
    std::vector< std::vector<float> > cur_predicted_labels;
    for (int class_id = 0; class_id < classes_num; class_id++) {
        std::vector<float> t;
        for (int i = 0; i < training_data.size(); i++) {
            t.push_back(0.0);
        }
        cur_predicted_labels.push_back(t);
    }

    // build trees iteratively
    for (int tree_id = 0; tree_id < num_trees; tree_id++) {

        logger(stdout, "------------------- build the %d-th tree ----------------------\n", tree_id);

        std::vector< std::vector<float> > softmax_predicted_labels;
        for (int class_id = 0; class_id < classes_num; class_id++) {
            std::vector<float> t;
            for (int i = 0; i < training_data.size(); i++) {
                t.push_back(0.0);
            }
            softmax_predicted_labels.push_back(t);
        }

        logger(stdout, "Init softmax labels\n");

        if (gbdt_type == 0 && tree_id != 0) {
            // compute predicted labels for classification
            for (int i = 0; i < training_data.size(); i++) {
                std::vector<float> prob_distribution_i;
                for (int class_id = 0; class_id < classes_num; class_id++) {
                    prob_distribution_i.push_back(cur_predicted_labels[class_id][i]);
                }
                std::vector<float> softmax_prob_distribution_i = softmax(prob_distribution_i);
                for (int class_id = 0; class_id < classes_num; class_id++) {
                    softmax_predicted_labels[class_id][i] = softmax_prob_distribution_i[class_id];
                }
            }
        }

        for (int class_id = 0; class_id < classes_num; class_id++) {
            // init single tree dataset
            if (gbdt_type == 0) {
                init_single_tree_data(client, class_id, tree_id, softmax_predicted_labels[class_id]);
            } else {
                init_single_tree_data(client, class_id, tree_id, cur_predicted_labels[class_id]);
            }

            //build the current tree
            int real_tree_id = class_id * num_trees + tree_id;
            forest[real_tree_id].init_features();
            forest[real_tree_id].init_root_node(client);
            forest[real_tree_id].build_tree_node(client, 0);

            // after the tree has been built, compute the predicted labels for the current tree
            std::vector<float> predicted_training_labels = compute_predicted_labels(client, class_id, tree_id, 0);
            for (int i = 0; i < training_data.size(); i++) {
                cur_predicted_labels[class_id][i] += predicted_training_labels[i];
            }
        }
    }

    logger(stdout, "End to build GBDT model\n");
}


std::vector<float> GBDT::compute_predicted_labels(Client &client, int class_id, int tree_id, int flag) {

    int real_tree_id = class_id * num_trees + tree_id;
    std::vector<float> predicted_label_vector;
    std::vector< std::vector<float> > input_dataset;
    int size = 0;
    if (flag == 0) { // training dataset
        input_dataset = training_data;
        size = training_data.size();
    } else {
        input_dataset = testing_data;
        size = testing_data.size();
    }

    for (int i = 0; i < size; i++) {
        predicted_label_vector.push_back(0.0);
    }

    // for each sample
    for (int i = 0; i < input_dataset.size(); ++i) {
        // step 1: organize the leaf label vector, compute the map
        EncodedNumber *label_vector = new EncodedNumber[forest[real_tree_id].internal_node_num + 1];

        std::map<int, int> node_index_2_leaf_index_map;
        int leaf_cur_index = 0;
        for (int j = 0; j < pow(2, forest[real_tree_id].max_depth + 1) - 1; j++) {
            if (forest[real_tree_id].tree_nodes[j].is_leaf == 1) {
                node_index_2_leaf_index_map.insert(std::make_pair(j, leaf_cur_index));
                label_vector[leaf_cur_index] = forest[real_tree_id].tree_nodes[j].label;  // record leaf label vector
                leaf_cur_index++;
            }
        }
        // compute binary vector for the current sample
        std::vector<float> sample_values = input_dataset[i];
        std::vector<int> binary_vector = compute_binary_vector(class_id, tree_id, sample_values, node_index_2_leaf_index_map);
        EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
        EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];
        // update in Robin cycle, from the last client to client 0
        if (client.client_id == client.client_num - 1) {

            for (int j = 0; j < binary_vector.size(); j++) {
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
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
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
                djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j],
                                  encoded_binary_vector[j]);
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
                encoded_binary_vector[j].set_integer(client.m_pk->n[0], binary_vector[j]);
                djcs_t_aux_ep_mul(client.m_pk, updated_label_vector[j], updated_label_vector[j],
                                  encoded_binary_vector[j]);
            }
        }
        // aggregate and call share decryption
        if (client.client_id == 0) {

            EncodedNumber *encrypted_aggregation = new EncodedNumber[1];
            encrypted_aggregation[0].set_float(client.m_pk->n[0], 0, 2 * FLOAT_PRECISION);
            djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_aggregation[0], encrypted_aggregation[0]);
            for (int j = 0; j < binary_vector.size(); j++) {
                djcs_t_aux_ee_add(client.m_pk, encrypted_aggregation[0], encrypted_aggregation[0],
                                  updated_label_vector[j]);
            }

            float label;
            EncodedNumber *decrypted_label = new EncodedNumber[1];
            client.share_batch_decrypt(encrypted_aggregation, decrypted_label, 1);
            decrypted_label->decode(label);

            predicted_label_vector[i] = label;
            delete[] encrypted_aggregation;
        } else {
            std::string s, response_s;
            client.recv_long_messages(client.channels[0].get(), s);
            client.decrypt_batch_piece(s, response_s, 0);
        }

        delete[] encoded_binary_vector;
        delete[] updated_label_vector;
        delete[] label_vector;
    }

    return predicted_label_vector;
}


std::vector<int> GBDT::compute_binary_vector(int class_id, int tree_id, std::vector<float> sample_values,
                                             std::map<int, int> node_index_2_leaf_index_map) {

    int real_tree_id = class_id * num_trees + tree_id;
    std::vector<int> binary_vector(forest[real_tree_id].internal_node_num + 1);

    // traverse the whole tree iteratively, and compute binary_vector
    std::stack<PredictionObj> traverse_prediction_objs;
    PredictionObj prediction_obj(forest[real_tree_id].tree_nodes[0].is_leaf, forest[real_tree_id].tree_nodes[0].is_self_feature,
            forest[real_tree_id].tree_nodes[0].best_client_id,
            forest[real_tree_id].tree_nodes[0].best_feature_id,
            forest[real_tree_id].tree_nodes[0].best_split_id, 1, 0);
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

            PredictionObj left(forest[real_tree_id].tree_nodes[left_node_index].is_leaf, forest[real_tree_id].tree_nodes[left_node_index].is_self_feature,
                               forest[real_tree_id].tree_nodes[left_node_index].best_client_id, forest[real_tree_id].tree_nodes[left_node_index].best_feature_id,
                               forest[real_tree_id].tree_nodes[left_node_index].best_split_id, pred_obj.mark, left_node_index);
            PredictionObj right(forest[real_tree_id].tree_nodes[right_node_index].is_leaf, forest[real_tree_id].tree_nodes[right_node_index].is_self_feature,
                                forest[real_tree_id].tree_nodes[right_node_index].best_client_id, forest[real_tree_id].tree_nodes[right_node_index].best_feature_id,
                                forest[real_tree_id].tree_nodes[right_node_index].best_split_id, pred_obj.mark, right_node_index);
            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        } else {
            // is self feature, retrieve split value and compare
            traverse_prediction_objs.pop();
            int feature_id = pred_obj.best_feature_id;
            int split_id = pred_obj.best_split_id;
            float split_value = forest[real_tree_id].features[feature_id].split_values[split_id];
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
            PredictionObj left(forest[real_tree_id].tree_nodes[left_node_index].is_leaf, forest[real_tree_id].tree_nodes[left_node_index].is_self_feature,
                               forest[real_tree_id].tree_nodes[left_node_index].best_client_id, forest[real_tree_id].tree_nodes[left_node_index].best_feature_id,
                               forest[real_tree_id].tree_nodes[left_node_index].best_split_id, left_mark, left_node_index);
            PredictionObj right(forest[real_tree_id].tree_nodes[right_node_index].is_leaf, forest[real_tree_id].tree_nodes[right_node_index].is_self_feature,
                                forest[real_tree_id].tree_nodes[right_node_index].best_client_id, forest[real_tree_id].tree_nodes[right_node_index].best_feature_id,
                                forest[real_tree_id].tree_nodes[right_node_index].best_split_id, right_mark, right_node_index);

            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        }
    }

    return binary_vector;
}


void GBDT::test_accuracy(Client & client, float & accuracy) {

    std::vector<float> predicted_label_vector;

    std::vector< std::vector<float> > predicted_forest_labels;
    for (int class_id = 0; class_id < classes_num; class_id++) {
        std::vector<float> t;
        for (int i = 0; i < testing_data.size(); i++) {
            t.push_back(0.0);
        }
        predicted_forest_labels.push_back(t);
    }

    for (int class_id = 0; class_id < classes_num; class_id++) {
        for (int tree_id = 0; tree_id < num_trees; tree_id++) {
            std::vector<float> labels = compute_predicted_labels(client, class_id, tree_id, 1);
            for (int i = 0; i < testing_data.size(); i++) {
                predicted_forest_labels[class_id][i] += labels[i];
            }
        }
    }

    if (gbdt_type == 0) {
        for (int i = 0; i < testing_data.size(); i++) {
            std::vector<float> prediction_prob_i;
            for (int class_id = 0; class_id < classes_num; class_id++) {
                prediction_prob_i.push_back(predicted_forest_labels[class_id][i]);
            }
            float label = argmax(prediction_prob_i);
            predicted_label_vector.push_back(label);
        }
    } else {
        for (int i = 0; i < testing_data.size(); i++) {
            predicted_label_vector[i] = predicted_forest_labels[0][i];
        }
    }

    // compute accuracy by the super client
    if (client.client_id == 0) {
        if (gbdt_type == 0) {
            int correct_num = 0;
            for (int i = 0; i < testing_data.size(); i++) {
                if (rounded_comparison(predicted_label_vector[i], testing_data_labels[i])) {
                    correct_num += 1;
                }
            }
            logger(stdout, "correct_num = %d, testing_data_size = %d\n", correct_num, testing_data_labels.size());
            accuracy = (float) correct_num / (float) testing_data_labels.size();
        } else {
            accuracy = mean_squared_error(predicted_label_vector, testing_data_labels);
        }
    }

    logger(stdout, "End test accuracy on testing dataset\n");
}


GBDT::~GBDT() {

    // free local data
    forest.clear();
    forest.shrink_to_fit();

    learning_rates.clear();
    learning_rates.shrink_to_fit();

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