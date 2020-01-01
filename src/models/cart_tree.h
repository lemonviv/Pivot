//
// Created by wuyuncheng on 26/11/19.
//

#ifndef COLLABORATIVEML_CART_TREE_H
#define COLLABORATIVEML_CART_TREE_H

#include "tree_node.h"
#include "feature.h"
#include "../client/Client.h"
#include "libhcs.h"
#include "gmp.h"
#include <vector>
#include "../utils/util.h"
#include "../utils/encoder.h"
#include "../include/common.h"

class DecisionTree {

public:
    SolutionType solution_type;                      // denote basic solution and enhanced solution, 0 for basic and 1 for enhanced
    OptimizationType optimization_type;              // denote optimization types, 0 non, 1 combining splits, 2 packing, 3 parallelism, 4 all
    int global_feature_num;                          // total features in the global dataset
    int local_feature_num;                           // local feature number in local dataset
    int internal_node_num;                           // total internal node number, leaf_num = internal_node_num + 1
    int type;                                        // type = 0, classification; type = 1, regression
    int classes_num;                                 // number of classes if classification
    int max_depth;                                   // maximum tree depth
    int max_bins;                                    // maximum bins
    int prune_sample_num;                            // when a node has samples less than this number, stop as a leaf node
    float prune_threshold;                           // when a node's variance is less than this threshold, stop as a leaf node
    TreeNode* tree_nodes;                            // array of TreeNode
    Feature* features;                               // array of Feature
    std::vector< std::vector<float> > training_data;   // training dataset
    std::vector< std::vector<float> > testing_data;    // training dataset
    std::vector<int> feature_types;                    // stores the feature types, 0: continuous, 1: categorical
    std::vector<float> training_data_labels;           // labels of training dataset
    std::vector<float> testingg_data_labels;           // labels of testing dataset
    std::vector< std::vector<int> > indicator_class_vecs; // binary vectors of classes, if classification
    std::vector< std::vector<float> > variance_stat_vecs; // variance vectors of labels, y and y^2, if regression
    std::vector<int> split_num_each_client;               // store the split_num of each client

public:
    /**
     * default constructor
     */
    DecisionTree();


    /**
     * constructor
     *
     * @param m_global_feature_num
     * @param m_local_feature_num
     * @param m_internal_node_num
     * @param m_type
     * @param m_classes_num
     * @param m_max_depth
     * @param m_max_bins
     * @param m_prune_sample_num
     * @param m_prune_threshold
     * @param m_solution_type
     * @param m_optimization_type
     */
    DecisionTree(int m_global_feature_num,
            int m_local_feature_num,
            int m_internal_node_num,
            int m_type,
            int m_classes_num,
            int m_max_depth,
            int m_max_bins,
            int m_prune_sample_num,
            float m_prune_threshold,
            int m_solution_type,
            int m_optimization_type);


    /**
     * destructor
     */
    ~DecisionTree();


    /**
     * init training data and test data according to split fraction
     * call by client 0
     *
     * @param client
     * @param split
     */
    void init_datasets(Client & client, float split);


    /**
     * init training data and test data according to new indexes received
     *
     * @param client
     * @param new_indexes
     * @param split
     */
    void init_datasets_with_indexes(Client & client, int new_indexes[], float split);


    /**
     * pre-compute feature information
     */
    void init_features();


    /**
     * init root node before recursively building tree
     *
     * @param client
     */
    void init_root_node(Client & client);


    /**
     * check whether pruning conditions are satisfied
     *
     * @param client
     * @param node_index
     * @return
     */
    bool check_pruning_conditions_revise(Client & client, int node_index);


    /**
     * compute encrypted impurity gain for each feature and each split
     *
     * @param client
     * @param node_index
     * @param encrypted_statistics
     * @param encrypted_label_vecs
     * @param encrypted_left_sample_nums
     * @param encrypted_right_sample_nums
     */
    void compute_encrypted_statistics(Client & client, int node_index,
            EncodedNumber ** & encrypted_statistics,
            EncodedNumber ** encrypted_label_vecs,
            EncodedNumber * & encrypted_left_sample_nums,
            EncodedNumber * & encrypted_right_sample_nums);


    /**
     * build a tree recursively
     *
     * @param client
     * @param node_index
     */
    void build_tree_node(Client & client, int node_index);


    /**
     * predict a result given a sample id
     *
     * @param sample_id
     * @param node_index_2_leaf_index_map
     */
    std::vector<int> compute_binary_vector(int sample_id, std::map<int,int> node_index_2_leaf_index_map);


    /**
     * test the accuracy on the test data
     *
     * @param client
     * @param accuracy
     */
    void test_accuracy(Client & client, float & accuracy);


    /**
     * enhanced solution: private selection given the encrypted iv of the best split index
     * return the encrypted split vector of the two branches
     *
     * @param best_feature_id
     * @param selection_iv_size
     * @param encrypted_selection_iv
     * @param encrypted_left_split_iv
     * @param encrypted_right_split_iv
     */
    void enhanced_private_selection(int best_feature_id, int selection_iv_size, EncodedNumber * encrypted_selection_iv,
            EncodedNumber * & encrypted_left_split_iv, EncodedNumber * & encrypted_right_split_iv);


    /**
     * update the encrypted sample iv given the two encrypted vectors
     *
     * @param client
     * @param encrypted_split_iv
     * @param encrypted_sample_iv
     * @param updated_encrypted_sample_iv
     */
    void enhanced_encrypted_sample_iv_update(Client & client, EncodedNumber * encrypted_split_iv, EncodedNumber * encrypted_sample_iv,
            EncodedNumber * & updated_encrypted_sample_iv);


    /**
     * test accuracy of the enhanced solution
     *
     * @param client
     * @param accuracy
     */
    void enhanced_test_accuracy(Client & client, float & accuracy);


    /**
     * private select the split iv for a private split num
     *
     * @param client
     * @param result_iv
     * @param selection_iv
     * @param split_iv_matrix
     * @param sample_num
     * @param split_num
     */
    void private_split_selection(Client & client, EncodedNumber * & result_iv,
            EncodedNumber * selection_iv, std::vector< std::vector<int> > split_iv_matrix, int sample_num, int split_num);

    /**
     * update the encrypted mask vector
     *
     * @param client
     * @param i_star
     * @param left_selection_result
     * @param right_selection_result
     * @param node_index
     */
    void update_sample_iv(Client & client, int i_star, EncodedNumber * left_selection_result,
            EncodedNumber * right_selection_result, int node_index);
};

#endif //COLLABORATIVEML_CART_TREE_H
