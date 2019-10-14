//
// Created by wuyuncheng on 10/10/19.
//

#include "Client.h"
#include <fstream>
#include <sstream>
#include <string>
#include <infra/ConfigFile.hpp>
#include "libhcs.h"
#include "gmp.h"
#include <comm/Comm.hpp>
#include "../utils/util.h"


Client::Client() {}


Client::Client(int param_client_id, int param_client_num, int param_has_label,
                std::string param_network_config_file,std::string param_local_data_file)
{
    // copy params
    client_id = param_client_id;
    client_num = param_client_num;
    has_label = param_has_label == 1;

    // read local dataset
    std::ifstream data_infile(param_local_data_file);

    if (!data_infile) {
        logger(stdout, "open %s error\n", param_local_data_file.c_str());
        exit(EXIT_FAILURE);
    }

    std::string line;
    std::vector<float> items;
    while (std::getline(data_infile, line)) {
        std::istringstream ss(line);
        std::string item;
        // split line with delimiter, default ','
        while(getline(ss, item,','))
        {
            items.push_back(::atof(item.c_str()));
        }
        local_data.push_back(items);
    }
    sample_num = local_data.size();
    feature_num = local_data[0].size();

    if (has_label) {
        // slice the last item as label
        for (int i = 0; i < sample_num; i++) {
            labels.push_back((int) local_data[i][feature_num-1]);
            local_data[i].pop_back();
        }
        feature_num = feature_num - 1;
    }

    // read network config file
    ConfigFile config_file(param_network_config_file);
    string portString, ipString;
    vector<int> ports(client_num);
    vector<string> ips(client_num);
    for (int i = 0; i < client_num; i++) {
        portString = "party_" + to_string(i) + "_port";
        ipString = "party_" + to_string(i) + "_ip";
        //get parties ips and ports
        ports[i] = stoi(config_file.Value("",portString));
        ips[i] = config_file.Value("",ipString);
    }

    // establish connections
    SocketPartyData me, other;
    boost::asio::io_service io_service;

    for (int  i = 0;  i < client_num; ++ i) {
        if (i < client_id) {
            // this party will be the receiver in the protocol
            me = SocketPartyData(boost_ip::address::from_string(ips[client_id]), ports[client_id] + i);
            logger(stdout, "my port = %d\n", ports[client_id] + i);
            other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + client_id - 1);
            logger(stdout, "other port = %d\n", ports[i] + client_id - 1);
            shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

            // connect to the other party
            channel->join(500,5000);
            logger(stdout, "channel established\n");
            channel->write("hello party " + i);

            // add channel to the other client
            channels.push_back(channel);
        } else if (i > client_id) {
            // this party will be the sender in the protocol
            me = SocketPartyData(boost_ip::address::from_string(ips[client_id]), ports[client_id] + i - 1);
            logger(stdout, "my port = %d\n", ports[client_id] + i - 1);
            other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + client_id);
            logger(stdout, "other port = %d\n", ports[i] + client_id);
            shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

            // connect to the other party
            channel->join(500,5000);
            logger(stdout, "channel established\n");
            channel->write("hello party " + i);

            // add channel to the other client
            channels.push_back(channel);
        } else {
            // self communication
            shared_ptr<CommParty> channel = NULL;
            channels.push_back(channel);
        }
    }

    // init DamgardJurik keys
    pk = pcs_t_init_public_key();
    au = pcs_t_init_auth_server();
    hr = hcs_init_random();
}


bool Client::generate_pcs_t_keys(int epsilon_s, int key_size, int client_num, int required_client_num)
{
    // Initialize data structures
    pcs_t_public_key *param_pk = pcs_t_init_public_key();
    pcs_t_private_key *param_vk = pcs_t_init_private_key();
    hcs_random *param_hr = hcs_init_random();

    // Generate a key pair with modulus of size key_size bits
    pcs_t_generate_key_pair(param_pk, param_vk, param_hr, key_size, required_client_num, client_num);

    pcs_t_polynomial *coeff = pcs_t_init_polynomial(param_vk, param_hr);
    pcs_t_auth_server **param_au =
            (pcs_t_auth_server **)malloc(client_num * sizeof(pcs_t_auth_server *));
    mpz_t *si = (mpz_t *)malloc(required_client_num * sizeof(mpz_t));
    for (int i = 0; i < client_num; i++) {
        mpz_init(si[i]);
        pcs_t_compute_polynomial(param_vk, coeff, si[i], i);
        // TODO: send to corresponding client via network (param_pk, param_hr, si, i)
        //param_au[i] = pcs_t_init_auth_server();
        //pcs_t_set_auth_server(param_au[i], si[i], i);
    }
}


void Client::set_keys(pcs_t_public_key *param_pk, hcs_random *param_hr, __mpz_struct *si, unsigned long i)
{
    // set the keys when receiving from the trusted third party
    pk = param_pk;
    hr = param_hr;
    pcs_t_set_auth_server(au, si, i);
}


Client::~Client()
{
    // free local data
    std::vector< std::vector<float> >().swap(local_data);

    // free labels if has_label == true
    if (has_label) {
        std::vector<int>().swap(labels);
    }

    // free channels
    std::vector< shared_ptr<CommParty> >().swap(channels);

    // free keys
    pcs_t_free_public_key(pk);
    pcs_t_free_auth_server(au);
    hcs_free_random(hr);
}
