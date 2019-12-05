#include <iostream>
#include <string>
#include <random>
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
    hr = hcs_init_random();
    mpz_t *coeff = djcs_t_init_polynomial(vk, hr);

    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
        mpz_init(si[i]);
        djcs_t_compute_polynomial(vk, coeff, si[i], i);
        au[i] = djcs_t_init_auth_server();
        djcs_t_set_auth_server(au[i], si[i], i);
    }
    mpz_init(n);
    mpz_init(positive_threshold);
    mpz_init(negative_threshold);
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

void test_share_decrypt(Client client) {

    for (int i = 0; i < client.client_num; i++) {
        if (i != client.client_id){
            std::string send_message = "Hello world from client " + std::to_string(client.client_id);
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

    if (client.client_id == 0) {
        // client with label
        EncodedNumber a;
        a.set_float(n, 0.123456);
        //djcs_t_aux_encrypt(pk, hr, a, a);
        a.print_encoded_number();

        for (int i = 1; i < client.client_num; i++) {
            std::string s;
            serialize_encoded_number(a, s);
            //std::string s1 = "hello world" + std::to_string(i);
            client.channels[i].get()->writeWithSize(s);
            //client.send_messages(client.channels[i].get(), s);
            //logger(stdout, "send message: %s\n", s.c_str());
        }
    } else {
        vector<byte> resMsg;
        client.channels[0].get()->readWithSizeIntoVector(resMsg);
        const byte * uc = &(resMsg[0]);
        std::string s(reinterpret_cast<char const*>(uc), resMsg.size());
        EncodedNumber t, tt;
        deserialize_number_from_string(t, s);
        t.print_encoded_number();
        //client.recv_messages(client.channels[0].get(), s, buffer, 999);
        //logger(stdout, "receive message: %s\n", s.c_str());
    }


    // test share decrypt
    if (client.client_id == 0) {

        EncodedNumber *ciphers = new EncodedNumber[2];
        ciphers[0].set_float(n, 0.123456);
        ciphers[1].set_float(n, 0.654321);

        djcs_t_aux_encrypt(client.m_pk, client.m_hr, ciphers[0], ciphers[0]);
        djcs_t_aux_encrypt(client.m_pk, client.m_hr, ciphers[1], ciphers[1]);

        ciphers[0].print_encoded_number();

        // decrypt using global key
        EncodedNumber dec_t0, dec_t1;
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, dec_t0, ciphers[0]);
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, dec_t1, ciphers[1]);
        float x0, x1;
        dec_t0.decode(x0);
        dec_t1.decode(x1);
        logger(stdout, "The decrypted value of ciphers[0] using global keys = %f\n", x0);
        logger(stdout, "The decrypted value of ciphers[1] using global keys = %f\n", x1);

        EncodedNumber *share_decrypted_res = new EncodedNumber[2];
        client.share_batch_decrypt(ciphers, share_decrypted_res, 2);

        float share_x0, share_x1;
        share_decrypted_res[0].decode(share_x0);
        share_decrypted_res[1].decode(share_x1);
        logger(stdout, "The decrypted value of ciphers[0] using share decryption = %f\n", share_x0);
        logger(stdout, "The decrypted value of ciphers[1] using share decryption = %f\n", share_x1);
    } else {

        std::string s, response_s;
        client.recv_long_messages(client.channels[0].get(), s);
        client.decrypt_batch_piece(s, response_s, 0);
    }
}


int main(int argc, char *argv[]) {

    int client_num = TOTAL_CLIENT_NUM;
    int client_id = atoi(argv[1]);
    bool has_label = (client_id == 0);
    std::string network_file = "/home/wuyuncheng/Documents/projects/CollaborativeML/data/networks/Parties.txt";
    std::string s1("/home/wuyuncheng/Documents/projects/CollaborativeML/data/datasets/");
    std::string s2 = std::to_string(client_id);
    std::string data_file = s1 + "client_" + s2 + ".txt";

    //const char *args[3] = {"dummy", argv[1], argv[2]};
    //run_player(3, args);

    if (client_id == 0) {
        system_setup();
    }

    test_pb();

    Client client(client_id, client_num, has_label, network_file, data_file);
    LogisticRegression model(BATCH_SIZE, MAX_ITERATION, CONVERGENCE_THRESHOLD, ALPHA, client.feature_num);

    logger(stdout, "init finished\n");

    // set up keys
    if (client.client_id == 0) {
        client.set_keys(pk, hr, si[client.client_id], client.client_id);

        // send keys
        for (int i = 0; i < client.client_num; i++) {
            if (i != client.client_id) {
                std::string keys_i;
                client.serialize_send_keys(keys_i, pk, si[i], i);
                client.send_long_messages(client.channels[i].get(), keys_i);
            }
        }
    } else {
        // receive keys from client 0
        std::string recv_keys;
        client.recv_long_messages(client.channels[0].get(), recv_keys);
        client.recv_set_keys(recv_keys);
        mpz_init(n);
        mpz_init(positive_threshold);
        mpz_init(negative_threshold);
        compute_thresholds(client.m_pk, n, positive_threshold, negative_threshold);
    }

    float split = 0.8;
    if (client.client_id == 0) {
        model.init_datasets(client, split);
    } else {
        int *new_indexes = new int[client.sample_num];
        std::string recv_s;
        client.recv_long_messages(client.channels[0].get(), recv_s);
        deserialize_ids_from_string(new_indexes, recv_s);
        model.init_datasets_with_indexes(client, new_indexes, split);
    }

    model.train(client);

    int test_size = client.sample_num * 0.2;
    int *sample_ids = new int[test_size];
    for (int i = 0; i < test_size; i++) {
        sample_ids[i] = client.sample_num - test_size + i;
    }

    float accuracy = 0.0;
    model.test(client, 1, accuracy);
    logger(stdout, "Testing accuracy = %f \n", accuracy);

//    test_share_decrypt(client);

    /**NOTE: the following test is only for single client test with client_id = 0
     * Need to make the test codes independent with the main function
     * */
//    test_encoder();
//    test_djcs_t_aux();
//    test_lr();
//    test_pb();

    if (client_id == 0) {
        system_free();
    } else {
        mpz_clear(n);
        mpz_clear(positive_threshold);
        mpz_clear(negative_threshold);
    }

    return 0;
}