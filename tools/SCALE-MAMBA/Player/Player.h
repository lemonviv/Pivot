#ifndef PLAYER_H_
#define PLAYER_H_

#include "Tools/ezOptionParser.h"
#include <atomic>
using namespace ez;

void Usage(ezOptionParser &opt);
int run_player(int n_iterations, int argc, const char *argv[]);

#endif  // PLAYER_H_