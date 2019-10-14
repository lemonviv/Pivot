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
    djcs_t_public_key* pk;                             // public key of threshold Paillier
    djcs_t_auth_server* au;                            // private share (auth server) of threshold Paillier
    hcs_random* hr;                                    // random value of threshold Paillier

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
     * generate paillier keys (currently the client who owns labels)
     * @param epsilon : layered cryptosystem with default epsilon_s = 1 (current Paillier)
     * @param key_size : security parameter of cryptosystem
     * @param client_num
     * @param required_client_num
     * @return
     */
    bool generate_pcs_t_keys(int epsilon_s, int key_size, int client_num, int required_client_num);


    /**
     * set the client key when receiving messages from the trusted third party
     *
     * @param param_pk
     * @param param_hr
     * @param si
     * @param i
     */
    void set_keys(djcs_t_public_key *param_pk, hcs_random* param_hr, mpz_t si, unsigned long i);
};


#endif //COLLABORATIVEML_CLIENT_H
