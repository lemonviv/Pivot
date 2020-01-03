//
// Created by wuyuncheng on 10/10/19.
//

#ifndef COLLABORATIVEML_CLIENT_H
#define COLLABORATIVEML_CLIENT_H

#include "gmp.h"
#include "libhcs.h"
#include "../utils/encoder.h"
#include <comm/Comm.hpp>
#include "../include/common.h"

class DecisionTree;

/**
 * This class is the client in distributed collaborative machine learning
 */
class Client {

public:
    int client_id;                                     // id for each client
    int client_num;                                    // total clients in the system
    bool has_label;                                    // only one client has label, default client 0
    std::vector< std::vector<float> > local_data;      // local data
    std::vector<int> labels;                           // if has_label == true, then has labels
    int sample_num;                                    // number of samples
    int feature_num;                                   // number of features
    std::vector< shared_ptr<CommParty> > channels;     // established communication channels with the other clients
    djcs_t_public_key* m_pk;                           // public key of threshold Paillier
    djcs_t_auth_server* m_au;                          // private share (auth server) of threshold Paillier
    hcs_random* m_hr;                                  // random value of threshold Paillier

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
     * copy constructor
     *
     * @param client
     */
    Client(const Client & client);

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
    bool generate_djcs_t_keys(int epsilon_s, int key_size, int client_num, int required_client_num);


    /**
     * set the client key when receiving messages from the trusted third party
     *
     * @param param_pk
     * @param param_hr
     * @param si
     * @param i
     */
    void set_keys(djcs_t_public_key *param_pk, hcs_random* param_hr, mpz_t si, unsigned long i);

    /**
     * set the client keys when receiving from the trusted third party
     *
     * @param recv_keys
     */
    void recv_set_keys(std::string recv_keys);


    /**
     * serialize the generated keys for other clients
     *
     * @param send_keys
     * @param pk
     * @param si
     * @param i
     */
    void serialize_send_keys(std::string & send_keys, djcs_t_public_key *pk, mpz_t si, int i);


    /**
     * share decrypt a batch ciphertexts
     * when size = 1, is a prediction request
     *
     * @param ciphers
     * @param decrypted_res
     * @param size
     * @param parallel 0 no parallelism, 1 parallelism
     */
    void share_batch_decrypt(EncodedNumber *ciphers, EncodedNumber *& decrypted_res, int size = 1, int parallel = 0);


    /**
     * decrypt batch pieces and return
     *
     * @param s
     * @param response_s
     * @param src_client_id
     * @param parallel 0 no parallelism, 1 parallelism
     */
    void decrypt_batch_piece(std::string s, std::string & response_s, int src_client_id, int parallel = 0);


    /**
     * conversion from homomorphic encryption to secret shares
     * for input to SCALE-MAMBA library, Robin circle computation
     * or directly send encrypted random numbers to the super client,
     * super client call the decryption to get the final random piece.
     * When size = 1, is a prediction request
     *
     * @param path
     * @param src_ciphers
     * @param dest_shares
     * @param size
     */
    void djcs_t_2_secret_shares_batch(std::string path,
            EncodedNumber *src_ciphers, EncodedNumber *& dest_shares, int size = 1);


    /**
     * After the mpc program is finished, call this function.
     * conversion from secret shares to homomorphic encryption
     * for local computations.
     * When size = 1, is a prediction request
     *
     * @param path
     * @param src_shares
     * @param dest_ciphers
     * @param size
     */
    void secret_shares_2_djcs_t_batch(std::string path,
            EncodedNumber *src_shares, EncodedNumber *& dest_ciphers, int size = 1);


    /**
     * Before sending the partial sums to the super client, add random shares
     * to each element in the batch, and write these shares to designated file
     *
     * @param shares
     * @param path
     */
    void write_random_shares(std::vector<float> shares, std::string path);


    /**
     * After mpc computation, read shares from designated file, and later
     * encrypted before sending to the super client for aggregation
     *
     * @param size
     * @param path
     *
     * @return shares
     */
    std::vector<float> read_random_shares(int size, std::string path);

    /**
     * send message via channel commParty
     *
     * @param comm_party
     * @param message
     */
    void send_messages(CommParty* comm_party, std::string message);


    /**
     * send long message via commParty
     *
     * @param comm_party
     * @param message
     */
    void send_long_messages(CommParty* comm_party, string message);


    /**
     * receive message from channel comm_party
     *
     * @param comm_party
     * @param message
     * @param buffer
     * @param expected_size
     */
    void recv_messages(CommParty* comm_party, std::string message, byte * buffer, int expected_size);

    /**
     * receive message from channel comm_party
     *
     * @param comm_party
     * @param message
     */
    void recv_long_messages(CommParty* comm_party, std::string & message);

    /**
     * print send message s
     *
     * @param s
     */
    void print_send_message(const string  &s);

    /**
     * print received message s
     *
     * @param s
     */
    void print_recv_message(const string &s);


    /**
     * print for debugging
     */
    void print_local_data();

    /**
     * print for debugging
     */
    void print_labels();
};


#endif //COLLABORATIVEML_CLIENT_H
