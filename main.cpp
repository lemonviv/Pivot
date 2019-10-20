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

void train(Client client, LogisticRegression model) {

    logger(stdout, "****** Training begin ******\n");

    // init encrypted local weights
    model.init_encrypted_local_weights(client.m_pk, client.m_hr);

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    if (client.client_id == 0) {
        for (int i = 0; i < client.sample_num; i++) {
            data_indexes.push_back(i);
        }
    }


    // training
    for (int iter = 0; iter < MAX_ITERATION; iter++) {

        logger(stdout, "iteration %d start\n", iter);

        // step 1: random select a batch with batch size and encode the batch samples
        int *batch_ids = new int[model.batch_size];
        if (client.client_id == 0) {
            auto rng = std::default_random_engine();
            std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
            for (int i = 0; i < model.batch_size; i++) {
                batch_ids[i] = data_indexes[i];
                logger(stdout, "selected batch ids by client 0: %d\n", batch_ids[i]);
            }

            // send to the other clients
            for (int i = 0; i < client.client_num; i++) {
                if (i != client.client_id) {
                    std::string s;
                    serialize_batch_ids(batch_ids, model.batch_size, s);
                    client.send_long_messages(client.channels[i].get(), s);
                }
            }
        } else {
            std::string recv_s;
            client.recv_long_messages(client.channels[0].get(), recv_s);
            deserialize_ids_from_string(batch_ids, recv_s);

            for (int i = 0; i < model.batch_size; i++) {
                logger(stdout, "received batch ids from client 0: %d\n", batch_ids[i]);
            }
        }

        EncodedNumber **batch_data = new EncodedNumber*[model.batch_size];
        for (int i = 0; i < model.batch_size; i++) {
            batch_data[i] = new EncodedNumber[model.feature_num];
        }

        // select the former batch_size data
        for (int i = 0; i < model.batch_size; i++) {
            for (int j = 0; j < model.feature_num; j++) {
                batch_data[i][j].set_float(n, client.local_data[batch_ids[i]][j]);
            }
        }

        logger(stdout, "step 1 computed succeed\n");

        // step 2: every client locally compute partial sum and send to client 0
        EncodedNumber *batch_partial_sums = new EncodedNumber[model.batch_size];
        for (int i = 0; i < model.batch_size; i++) {
            model.instance_partial_sum(client.m_pk, client.m_hr, batch_data[i], batch_partial_sums[i]);
        }

        EncodedNumber **batch_partial_sums_array = new EncodedNumber*[model.batch_size];
        for (int i = 0; i < model.batch_size; i++) {
            batch_partial_sums_array[i] = new EncodedNumber[client.client_num];
        }
        if (client.client_id == 0) {
            for (int j = 0; j < client.client_num; j++) {
                if (j == 0) {
                    for (int i = 0; i < model.batch_size; i++) {
                        batch_partial_sums_array[i][j] = batch_partial_sums[i];
                    }
                } else {
                    // receive from the other clients
                    std::string recv_s;
                    EncodedNumber *recv_batch_partial_sums = new EncodedNumber[model.batch_size];
                    client.recv_long_messages(client.channels[j].get(), recv_s);
                    deserialize_sums_from_string(recv_batch_partial_sums, model.batch_size, recv_s);
                    for (int i = 0; i < model.batch_size; i++) {
                        batch_partial_sums_array[i][j] = recv_batch_partial_sums[i];
                    }

                    delete [] recv_batch_partial_sums;
                }
            }

        } else {
            std::string s;
            serialize_batch_sums(batch_partial_sums, model.batch_size, s);
            client.send_long_messages(client.channels[0].get(), s);
        }

        logger(stdout, "step 2 computed succeed\n");

        // step 3: client 0 aggregate the sums and call share decrypt (call mpc when mpc is ready)
        EncodedNumber *batch_aggregated_sums = new EncodedNumber[model.batch_size];
        EncodedNumber *decrypted_batch_aggregated_sums = new EncodedNumber[model.batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < model.batch_size; i++) {
                model.aggregate_partial_sum_instance(client.m_pk, client.m_hr,
                        batch_partial_sums_array[i], batch_aggregated_sums[i], client.client_num);
            }

            // call share decrypt
            client.share_batch_decrypt(batch_aggregated_sums, decrypted_batch_aggregated_sums, model.batch_size);

        } else {
            std::string s, response_s;
            client.recv_long_messages(client.channels[0].get(), s);
            client.decrypt_batch_piece(s, response_s, 0);
        }

        logger(stdout, "step 3 computed succeed\n");

        int correct_num = 0;

        // step 4: client 0 compute the logistic function and encrypt the result (should truncation)
        EncodedNumber *reencrypted_batch_sums = new EncodedNumber[model.batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < model.batch_size; i++) {
                float t;
                decrypted_batch_aggregated_sums[i].decode(t);
                t = 1.0 / (1 + exp(0 - t));
                int predict_label = t >= 0.5 ? 1 : 0;
                if (predict_label == client.labels[batch_ids[i]]) correct_num += 1;
                // truncate to FLOAT_PRECISION, same as the labels
                reencrypted_batch_sums[i].set_float(decrypted_batch_aggregated_sums[i].n, t);
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, reencrypted_batch_sums[i], reencrypted_batch_sums[i]);
            }
        }

        // add temporary code for viewing the training accuracy, now is full training dataset sgd
        logger(stdout, "Iteration %d accuracy = %f\n", iter, (float) correct_num / (float) client.sample_num);
        logger(stdout, "step 4 computed succeed\n");

        // step 5: client 0 compute the losses of the batch, and send to every other client
        EncodedNumber *encrypted_losses = new EncodedNumber[model.batch_size];
        if (client.client_id == 0) {
            for (int i = 0; i < model.batch_size; i++) {
                encrypted_losses[i].set_float(n, (float) (0 - client.labels[batch_ids[i]]));
                djcs_t_aux_encrypt(client.m_pk, client.m_hr, encrypted_losses[i], encrypted_losses[i]);
                djcs_t_aux_ee_add(client.m_pk, encrypted_losses[i], encrypted_losses[i], reencrypted_batch_sums[i]);
            }

            model.update_local_weights(client.m_pk, client.m_hr, batch_data, encrypted_losses, 1.0);

            for (int j = 0; j < client.client_num; j++) {
                if (j != client.client_id){
                    std::string s;
                    serialize_batch_losses(encrypted_losses, model.batch_size, s);
                    client.send_long_messages(client.channels[j].get(), s);
                }
            }
        } else {
            std::string recv_s;
            EncodedNumber *recv_encrypted_losses = new EncodedNumber[model.batch_size];
            client.recv_long_messages(client.channels[0].get(), recv_s);
            deserialize_sums_from_string(recv_encrypted_losses, model.batch_size, recv_s);

            // step 6: every client update its local weights
            model.update_local_weights(client.m_pk, client.m_hr, batch_data, recv_encrypted_losses, 1.0);

            delete [] recv_encrypted_losses;
        }

        logger(stdout, "step 5 and step 6 computed succeed\n");


        // free temporary memories
        delete [] batch_ids;
        delete [] batch_partial_sums;
        delete [] batch_aggregated_sums;
        delete [] decrypted_batch_aggregated_sums;
        delete [] reencrypted_batch_sums;
        delete [] encrypted_losses;
        for (int i = 0; i < model.batch_size; i++) {
            delete [] batch_data[i];
            delete [] batch_partial_sums_array[i];
        }
        delete [] batch_data;
        delete [] batch_partial_sums_array;
    }

    // share decrypt the local weights
    EncodedNumber *decrypted_weights = new EncodedNumber[model.feature_num];

    for (int i = 0; i < client.client_num; i++) {
        if (i == client.client_id) {
            client.share_batch_decrypt(model.local_weights, decrypted_weights, model.feature_num);
            for (int j = 0; j < model.feature_num; j++) {
                float weight;
                decrypted_weights[j].decode(weight);
                logger(stdout, "The decrypted weight %d = %f\n", j, weight);
            }
        } else {
            // decrypt piece
            std::string recv_s, response_s;
            client.recv_long_messages(client.channels[i].get(), recv_s);
            client.decrypt_batch_piece(recv_s, response_s, i);
        }
    }

    logger(stdout, "****** Training end ******\n");
}


void test(Client client, LogisticRegression model, int *sample_ids) {
    //TODO: extract the main process in train function as test
    logger(stdout, "****** Test accuracy ******\n");
}


int main(int argc, char *argv[]) {

    int client_num = TOTAL_CLIENT_NUM;
    int client_id = atoi(argv[1]);
    bool has_label = (client_id == 0);
    std::string network_file = "/home/wuyuncheng/Documents/projects/CollaborativeML/data/networks/Parties.txt";
    std::string s1("/home/wuyuncheng/Documents/projects/CollaborativeML/data/datasets/");
    std::string s2 = std::to_string(client_id);
    std::string data_file = s1 + "client_" + s2 + ".txt";

    if (client_id == 0) {
        system_setup();
    }

    Client client(client_id, client_num, has_label, network_file, data_file);
    LogisticRegression model(BATCH_SIZE, MAX_ITERATION, CONVERGENCE_THRESHOLD, client.feature_num);

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

        gmp_printf("the gmp n is %Zd\n", n);
    } else {
        // receive keys from client 0
        std::string recv_keys;
        client.recv_long_messages(client.channels[0].get(), recv_keys);
        client.recv_set_keys(recv_keys);
        mpz_init(n);
        mpz_init(positive_threshold);
        mpz_init(negative_threshold);
        compute_thresholds(client.m_pk, n, positive_threshold, negative_threshold);

        gmp_printf("the gmp n is %Zd\n", n);
    }


    train(client, model);

    int test_size = client.sample_num * 0.2;
    int *sample_ids = new int[test_size];
    for (int i = 0; i < test_size; i++) {
        sample_ids[i] = client.sample_num - test_size + i;
    }
    test(client, model, sample_ids);


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