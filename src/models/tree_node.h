//
// Created by wuyuncheng on 26/11/19.
//

#ifndef COLLABORATIVEML_TREE_NODE_H
#define COLLABORATIVEML_TREE_NODE_H

#include "../utils/encoder.h"
#include "../utils/djcs_t_aux.h"
#include "../utils/util.h"
#include <vector>

class TreeNode {
public:
    int is_leaf;                                // 0: not leaf, 1: leaf node, -1: not decided
    int depth;                                  // the depth of the current tree node, root node is 0, -1: not decided
    int is_self_feature;                        // 0: not self feature, 1: self feature, -1: not decided
    int best_client_id;                         // -1: not decided
    int best_feature_id;                        // -1: not self feature, 0 -- d_i: self feature id
    int best_split_id;                          // split_id, -1: not decided
    std::vector<int> available_feature_ids;     // the available local feature ids
    int available_global_feature_num;           // the number of global features globally
    int type;                                   // 0: classification, 1: regression
    int sample_size;                            // the number of samples on the node
    EncodedNumber impurity;                     // the impurity of the current tree node, Gini index for classification, variance for regression
    EncodedNumber *sample_iv;                   // the encrypted indicator vector of which samples are available on this node
    EncodedNumber label;                        // if is_leaf is true, a label is assigned; if type = 0, T = int, else T = float
    int left_child;                             // left branch id of the current node, if it is not a leaf node, -1: not decided
    int right_child;                            // right branch id of the current node, if it is not a leaf node, -1: not decided


public:
    TreeNode();
    TreeNode(int m_depth, int type, int sample_size, EncodedNumber *sample_iv);
    ~TreeNode();

    void print_node();
};

#endif //COLLABORATIVEML_TREE_NODE_H
