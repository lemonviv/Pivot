//
// Created by wuyuncheng on 29/12/19.
//

#ifndef COLLABORATIVEML_COMMON_H
#define COLLABORATIVEML_COMMON_H


#define GLOBAL_FEATURE_NUM 35
#define MAX_IMPURITY 2.0
#define MAX_VARIANCE 10000.0
#define MAX_GLOBAL_SPLIT_NUM 1000
#define MAX_DEPTH 1
#define MAX_BINS 8
#define CLASSES_NUM 3
#define TREE_TYPE 0  // 0: classification tree, 1: regression tree
#define PRUNE_SAMPLE_NUM 5
#define PRUNE_VARIANCE_THRESHOLD 0.001
#define SPDZ_PORT_NUM_DT 18000
#define NUM_SPDZ_PARTIES 3
#define HOST_NAME "localhost"
#define NUM_OMP_THREADS 4
#define SPLIT_PERCENTAGE 0.8
#define RF_SAMPLE_RATE 0.8
#define SPDZ_PORT_NUM_DT_ENHANCED 19000
#define SPDZ_PORT_NUM_RF_CLASSIFICATION_PREDICTION 20000
#define MAXIMUM_RAND_VALUE 1000000
#define ROUNDED_PRECISION 1e-3
#define MAX_PACKING_CAPACITY 6.0
#define RADIX_FOR_PACKING 2000 // should make sure that the dot product is less than this number, otherwise would be biased
#define NUM_TREES 4
#define GBDT_FLAG 0

enum SolutionType {Basic, Enhanced};
enum OptimizationType {Non, CombiningSplits, Packing, Parallelism, All};


#endif //COLLABORATIVEML_COMMON_H
