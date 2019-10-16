#include <iostream>
#include "tests/test_encoder.h"
#include "tests/test_djcs_t_aux.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    test_encoder();
    test_djcs_t_aux();
    //test_djcs_t();
    return 0;
}