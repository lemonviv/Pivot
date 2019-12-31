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
#define PRUNE_SAMPLE_NUM 5
#define PRUNE_VARIANCE_THRESHOLD 0.01
#define SPDZ_PORT_NUM_DT 18000
#define NUM_SPDZ_PARTIES 3
#define NUM_OMP_THREADS 4
#define SPLIT_PERCENTAGE 0.6

enum SolutionType {Basic, Enhanced};
enum OptimizationType {Non, CombiningSplits, Packing, Parallelism, All};


#endif //COLLABORATIVEML_COMMON_H
