//
// Created by wuyuncheng on 10/10/19.
//

#ifndef COLLABORATIVEML_CLIENT_H
#define COLLABORATIVEML_CLIENT_H

#include "gmp.h"
#include "libhcs.h"
#include <comm/Comm.hpp>

/**
 * This class is the client in distributed collaborative machine learning
 */
class Client {

private:
    int client_id;                                     // id for each client
    int client_num;                                    // total clients in the system
    bool has_label;                                    // only one client has label, default client 0
    std::vector< std::vector<float> > local_data;      // local data
    std::vector<int> labels;                           // if has_label == true, then has labels
    int sample_num;                                    // number of samples
    int feature_num;                                   // number of features
    std::vector< shared_ptr<CommParty> > channels;     // established communication channels with the other clients
    djcs_public_key* public_key;                       // public key of DamgardJurik cryptosystem
    djcs_private_key* private_key;                     // private key of DamgardJurik cryptosystem (not threhsold)
    hcs_random* hr;                                    // random value of DamgardJurik cryptosystem

public:
    /**
     * default constructor
     */
    Client();

    /**
     * constructor with parameters
     * (1) copy input params
     * (2) read local data
     * (3) read network configs and establish connections
     * (4) initialize paillier keys
     *
     * @param param_client_id
     * @param param_client_num
     * @param param_has_label
     * @param param_network_config_file
     * @param param_local_data_file
     */
    Client(int param_client_id, int param_client_num, int param_has_label,
            std::string param_network_config_file, std::string param_local_data_file);

    /**
     * destructor
     */
    ~Client();

    /**
     * generate DamgardJurik cryptosystem keys (currently the client who
     * owns labels)
     * @param epsilon : layered cryptosystem with default epsilon_s = 1
     * @param key_size : security parameter of cryptosystem
     * @return
     */
    bool generate_djcs_keys(int epsilon_s, int key_size);
};


#endif //COLLABORATIVEML_CLIENT_H
