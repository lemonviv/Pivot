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


void TreeNode::print_node() {

    logger(stdout, "Node depth = %d\n", depth);
    logger(stdout, "Is leaf = %d\n", is_leaf);
    logger(stdout, "Is self feature = %d\n", is_self_feature);
    logger(stdout, "Best client id = %d\n", best_client_id);
    if (is_self_feature) {
        logger(stdout, "Best feature id = %d\n", best_feature_id);
        logger(stdout, "Best split id = %d\n", best_split_id);
    }
    if (is_leaf) {
        float f_label;
        label.decode(f_label);
        logger(stdout, "Label = %f\n", f_label);
    }

}


TreeNode::~TreeNode() {
    //should free EncodedNumber?
    std::vector<int>().swap(available_feature_ids);
    delete [] sample_iv;
}