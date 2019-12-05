//
// Created by wuyuncheng on 26/11/19.
//

#include "tree_node.h"

TreeNode::TreeNode() {
    is_leaf = -1;
    depth = -1;
    is_self_feature = -1;
    best_client_id = -1;
    best_feature_id = -1;
    best_split_id = -1;
    type = -1;
    sample_size = -1;
    left_child = -1;
    right_child = -1;
    available_global_feature_num = -1;
}

TreeNode::TreeNode(int m_depth, int m_type, int m_sample_size, EncodedNumber *m_sample_iv) {
    is_leaf = -1;
    depth = m_depth;
    type = m_type;
    is_self_feature = -1;
    best_client_id = -1;
    best_feature_id = -1;
    best_split_id = -1;
    sample_size = m_sample_size;
    sample_iv = m_sample_iv;
    left_child = -1;
    right_child = -1;
    available_global_feature_num = -1;
}

TreeNode::~TreeNode() {
    //should free EncodedNumber?
    std::vector<int>().swap(available_feature_ids);
}