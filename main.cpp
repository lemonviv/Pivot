#include <iostream>
#include "tests/test_encoder.h"
#include "tests/test_djcs_t_aux.h"
#include "tests/test_logistic_regression.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    test_encoder();
    test_djcs_t_aux();
    test_lr();
    return 0;
}