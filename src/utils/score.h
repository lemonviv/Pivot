//
// Created by wuyuncheng on 10/1/20.
//

#ifndef COLLABORATIVEML_SCORE_H
#define COLLABORATIVEML_SCORE_H

#include <vector>

float mean_squared_error(std::vector<float> a, std::vector<float> b);
bool rounded_comparison(float a, float b);

#endif //COLLABORATIVEML_SCORE_H
