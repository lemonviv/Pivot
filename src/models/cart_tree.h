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


#define GLOBAL_FEATURE_NUM 10
#define MAX_IMPURITY 2.0
#define MAX_VARIANCE 10000.0
#define MAX_GLOBAL_SPLIT_NUM 1000
#define MAX_DEPTH 10
#define MAX_BINS 8
#define PRUNE_SAMPLE_NUM 5
#define PRUNE_VARIANCE_THRESHOLD 0.01

class DecisionTree {

public:
    int global_feature_num;                          // total features in the global dataset
    int local_feature_num;                           // local feature number in local dataset
    int internal_node_num;                           // total internal node number, leaf_num = internal_node_num + 1
    int type;                                        // type = 0, classification; type = 1, regression    int classes_num;                                 //
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
     */
    DecisionTree(int m_global_feature_num,
            int m_local_feature_num,
            int m_internal_node_num,
            int m_type,
            int m_classes_num,
            int m_max_depth,
            int m_max_bins,
            int m_prune_sample_num,
            float m_prune_threshold);


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
     * check whether specific conditions are satisfied
     *
     * @param client
     * @param node_index
     * @param encrypted_label_vecs
     * @param label
     */
    bool check_pruning_conditions(Client & client, int node_index, EncodedNumber ** & encrypted_label_vecs, EncodedNumber & label);


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
            EncodedNumber ** & encrypted_statistics, EncodedNumber ** encrypted_label_vecs,
            EncodedNumber * & encrypted_left_sample_nums, EncodedNumber * & encrypted_right_sample_nums);


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
     * @param client
     * @param sample_id
     */
    void predict(Client & client, int sample_id);
};

#endif //COLLABORATIVEML_CART_TREE_H
