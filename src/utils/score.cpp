//
// Created by wuyuncheng on 10/1/20.
//

#include <cmath>
#include "score.h"
#include "util.h"
#include "../include/common.h"

float mean_squared_error(std::vector<float> a, std::vector<float> b) {
    if (a.size() != b.size()) {
        logger(stdout, "Mean squared error computation wrong: sizes of the two vectors not same\n");
    }

    int num = a.size();
    float squared_error = 0.0;
    for (int i = 0; i < num; i++) {
        squared_error = squared_error + (a[i] - b[i]) * (a[i] - b[i]);
    }

    float mean_squared_error = squared_error / num;
    return mean_squared_error;
}

bool rounded_comparison(float a, float b) {
    if ((a >= b - ROUNDED_PRECISION) && (a <= b + ROUNDED_PRECISION)) return true;
    else return false;
}