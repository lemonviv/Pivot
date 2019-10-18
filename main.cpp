#include <iostream>
#include <string>
#include "src/utils/util.h"
#include "src/utils/encoder.h"
#include "src/utils/djcs_t_aux.h"
#include "src/utils/pb_converter.h"
#include "src/models/logistic_regression.h"
#include "src/client/Client.h"

#include "tests/test_encoder.h"
#include "tests/test_djcs_t_aux.h"
#include "tests/test_logistic_regression.h"
#include "tests/test_pb_converter.h"

hcs_random *hr;
djcs_t_public_key *pk;
djcs_t_private_key *vk;
mpz_t n, positive_threshold, negative_threshold;
djcs_t_auth_server **au = (djcs_t_auth_server **)malloc(TOTAL_CLIENT_NUM * sizeof(djcs_t_auth_server *));
mpz_t *si = (mpz_t *)malloc(TOTAL_CLIENT_NUM * sizeof(mpz_t));

void system_setup() {
    hr = hcs_init_random();
    pk = djcs_t_init_public_key();
    vk = djcs_t_init_private_key();

    djcs_t_generate_key_pair(pk, vk, hr, 1, 1024, TOTAL_CLIENT_NUM, TOTAL_CLIENT_NUM);
    mpz_t *coeff = djcs_t_init_polynomial(vk, hr);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_init(si[i]);
        djcs_t_compute_polynomial(vk, coeff, si[i], i);
        au[i] = djcs_t_init_auth_server();
        djcs_t_set_auth_server(au[i], si[i], i);
    }
    compute_thresholds(pk, n, positive_threshold, negative_threshold);

    djcs_t_free_polynomial(vk, coeff);
}

void system_free() {

    // free memory
    hcs_free_random(hr);
    djcs_t_free_public_key(pk);
    djcs_t_free_private_key(vk);

    mpz_clear(n);
    mpz_clear(positive_threshold);
    mpz_clear(negative_threshold);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_clear(si[i]);
        djcs_t_free_auth_server(au[i]);
    }

    free(si);
    free(au);
}

int main(int argc, char *argv[]) {

    system_setup();

    test_encoder();
    test_djcs_t_aux();
    test_lr();
    test_pb();

    int client_num = TOTAL_CLIENT_NUM;
    int client_id = atoi(argv[1]);
    bool has_label = (client_id == 0);
    std::string network_file = "/home/wuyuncheng/Documents/projects/CollaborativeML/data/networks/Parties.txt";
    std::string s1("/home/wuyuncheng/Documents/projects/CollaborativeML/data/datasets/");
    std::string s2 = std::to_string(client_id);
    std::string data_file = s1 + "client_" + s2 + ".txt";

    Client client(client_id, client_num, has_label, network_file, data_file);
    //client.set_keys(pk, hr, si[client_id], (unsigned long) client_id);

    // print client's data
    // client.print_local_data();
    // client.print_labels();

    for (int i = 0; i < client.client_num; i++) {
        if (i != client_id){
            std::string send_message = "Hello world from client " + std::to_string(client_id);
            int size = send_message.size();
            std::string recv_message;
            byte buffer[100];
            client.send_messages(client.channels[i].get(), send_message);
            client.recv_messages(client.channels[i].get(), recv_message, buffer, size);

            string longMessage = "Hi, this is a long message to test the writeWithSize approach";
            client.channels[i].get()->writeWithSize(longMessage);

            vector<byte> resMsg;
            client.channels[i].get()->readWithSizeIntoVector(resMsg);
            const byte * uc = &(resMsg[0]);
            string resMsgStr(reinterpret_cast<char const*>(uc), resMsg.size());
            string eq = (resMsgStr == longMessage)? "yes" : "no";
            cout << "Got long message: " << resMsgStr << ".\nequal? " << eq << "!" << endl;
        }
    }

//    if (client_id == 0) {
//        // client with label
//        mpz_t x;
//        mpz_init(x);
//        mpz_set_si(x, 9);
//        char *t = mpz_get_str(NULL, 10, x);
//        std::string s = t;
//        djcs_t_encrypt(pk, hr, x, x);
//        for (int i = 0; i < client_num; i++) {
//
//            int size = s.size();
//            std::string recv_message;
//            byte buffer[100];
//            client.send_messages(client.channels[i].get(), s);
//            client.recv_messages(client.channels[i].get(), recv_message, buffer, size);
//        }
//    }



    system_free();

    return 0;
}