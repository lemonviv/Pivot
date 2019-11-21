//
// Created by wuyuncheng on 20/11/19.
//

#ifndef COLLABORATIVEML_SPDZ_UTIL_H
#define COLLABORATIVEML_SPDZ_UTIL_H

// header files from MP-SPDZ

#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Networking/sockets.h"
#include "Tools/int.h"
#include "Math/Setup.h"
#include "Protocols/fake-stuff.h"

#include <sodium.h>
#include <iostream>
#include <sstream>
#include <fstream>



/**
 * send the private partial sums of the current batch to the spdz parties
 *
 * @param shares
 * @param sockets
 * @param n_parties
 */
void send_private_batch_shares(std::vector<float> shares, std::vector<int>& sockets, int n_parties);


/**
 * setup sockets to communicate with spdz parties
 *
 * @param n_parties
 * @param host_name
 * @param port_base
 * @return
 */
std::vector<int> setup_sockets(int n_parties, const std::string host_name, int port_base);


/**
 * Send the private inputs masked with a random value.
 * Receive shares of a preprocessed triple from each SPDZ engine, combine and check the triples are valid.
 * Send to each spdz engine.
 *
 * @param values
 * @param sockets
 * @param n_parties
 */
void send_private_inputs(std::vector<gfp>& values, std::vector<int>& sockets, int n_parties);

/**
 * Assumes that Scripts/setup-online.sh has been run to compute prime
 *
 * @param dir_prefix the path that stores the prime number
 */
void initialise_fields(const string& dir_prefix);

/**
 * Receive the shares and post-process for the following computations.
 *
 * @param sockets the sockets of the spdz parties
 * @param nparties the number of parties
 * @param size
 */
std::vector<float> receive_result(std::vector<int>& sockets, int n_parties, int size);



#endif //COLLABORATIVEML_SPDZ_UTIL_H
