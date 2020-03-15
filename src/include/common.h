//
// Created by wuyuncheng on 29/12/19.
//

#ifndef PIVOT_COMMON_H
#define PIVOT_COMMON_H

/*************************** Party parameters *************************/

#define TOTAL_CLIENT_NUM 3
#define SPDZ_PORT_NUM_DT 18000
#define NUM_SPDZ_PARTIES 3
#define SPDZ_PORT_NUM_DT_ENHANCED 19000
#define SPDZ_PORT_NUM_RF_CLASSIFICATION_PREDICTION 20000
#define ROUNDED_PRECISION 1e-3

/********************* Decision tree parameters ***********************/

#define FLOAT_PRECISION 8
#define PRECISION_THRESHOLD 1e-6
#define GLOBAL_FEATURE_NUM 35
#define MAX_IMPURITY 2.0
#define MAX_VARIANCE 10000.0
#define MAX_GLOBAL_SPLIT_NUM 6000
#define MAX_DEPTH 3
#define MAX_BINS 32
#define DEFAULT_CLASSES_NUM 3
#define TREE_TYPE 0  // 0: classification tree, 1: regression tree
#define PRUNE_SAMPLE_NUM 5
#define PRUNE_VARIANCE_THRESHOLD 0.0001
#define MAXIMUM_RAND_VALUE 32767
enum SolutionType {Basic, Enhanced};
enum OptimizationType {Non, CombiningSplits, Parallelism, All};

/********************* Random forest parameters ***********************/

#define NUM_TREES 3
#define RF_SAMPLE_RATE 0.8

/************************** GBDT parameters ***************************/

#define GBDT_FLAG 0
#define GBDT_LEARNING_RATE 0.1

/************************* Program parameters *************************/

#define SPLIT_PERCENTAGE 0.8
#define NUM_TRIALS 10
#define DEFAULT_NETWORK_FILE "/home/wuyuncheng/Documents/projects/Pivot/data/networks/Parties.txt"
#define DEFAULT_DATA_FILE_PREFIX "/home/wuyuncheng/Documents/projects/Pivot/data/"
#define DEFAULT_HOST_NAME "localhost"
#define DEFAULT_PARAM_DATA_FILE "/home/wuyuncheng/Documents/projects/Pivot-SPDZ/Player-Data/3-128-128/Params-Data"
#define PROGRAM_HOME "/home/wuyuncheng/Documents/projects/Pivot/"
#define LOGGER_HOME "/home/wuyuncheng/Documents/projects/Pivot/log/"
#define NUM_OMP_THREADS 4


#endif //PIVOT_COMMON_H
