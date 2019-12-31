//
// Created by wuyuncheng on 10/10/19.
//

#include "Client.h"
#include <fstream>
#include <sstream>
#include <string>
#include <fstream>
#include <random>
#include <stack>
#include <infra/ConfigFile.hpp>
#include "libhcs.h"
#include "gmp.h"
#include <comm/Comm.hpp>
#include "../utils/util.h"
#include "../utils/pb_converter.h"
#include "../include/protobuf/keys.pb.h"
#include "../models/cart_tree.h"


Client::Client() {}


Client::Client(int param_client_id, int param_client_num, int param_has_label,
                std::string param_network_config_file, std::string param_local_data_file)
{
    // copy params
    client_id = param_client_id;
    client_num = param_client_num;
    has_label = param_has_label == 1;

    // init threshold DamgardJurik keys
    m_pk = djcs_t_init_public_key();
    m_au = djcs_t_init_auth_server();
    m_hr = hcs_init_random();

    // read local dataset
    std::ifstream data_infile(param_local_data_file);

    if (!data_infile) {
        logger(stdout, "open %s error\n", param_local_data_file.c_str());
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(data_infile, line)) {
        std::vector<float> items;
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

            // add channel to the other client
            channels.push_back(std::move(channel));
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

            // add channel to the other client
            channels.push_back(std::move(channel));
        } else {
            // self communication
            shared_ptr<CommParty> channel = NULL;
            channels.push_back(std::move(channel));
        }
    }

    data_infile.close();
}


Client::Client(const Client &client) {

    feature_num = client.feature_num;
    sample_num = client.sample_num;
    client_id = client.client_id;
    client_num = client.client_num;
    has_label = client.has_label;
    local_data = client.local_data;
    labels = client.labels;
    channels = client.channels;

    m_pk = djcs_t_init_public_key();
    m_au = djcs_t_init_auth_server();
    m_hr = hcs_init_random();

    // copy values of pk
    m_pk->s = client.m_pk->s;
    m_pk->l = client.m_pk->l;
    m_pk->w = client.m_pk->w;
    mpz_set(m_pk->g, client.m_pk->g);
    mpz_set(m_pk->delta, client.m_pk->delta);
    m_pk->n = (mpz_t *) malloc(sizeof(mpz_t) * (m_pk->s+1));
    for (int i = 0; i < m_pk->s + 1; i++) {
        mpz_init_set(m_pk->n[i], client.m_pk->n[i]);
    }

    // copy values of hr
    gmp_randinit_set(m_hr->rstate, client.m_hr->rstate);

    // copay values of au
    m_au->i = client.m_au->i;
    mpz_init_set(m_au->si, client.m_au->si);

    //m_pk = client.m_pk;
    //m_au = client.m_au;
    //m_hr = client.m_hr;
}

void Client::split_datasets(float split) {
    logger(stdout, "Begin split dataset\n");

    int training_data_size = sample_num * split;
    int testing_data_size = sample_num - training_data_size;

    // store the indexes of the training dataset for random batch selection
    std::vector<int> data_indexes;
    for (int i = 0; i < sample_num; i++) {
        data_indexes.push_back(i);
    }

    auto rng = std::default_random_engine();
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);

    // select the former training data size as training data, and the latter as testing data
    for (int i = 0; i < data_indexes.size(); i++) {
        if (i < training_data_size) {
            // add to training dataset and labels
            training_data.push_back(local_data[data_indexes[i]]);
            if (has_label) {
                training_labels.push_back(labels[data_indexes[i]]);
            }
        } else {
            // add to testing dataset and labels
            testing_data.push_back(local_data[data_indexes[i]]);
            if (has_label) {
                testing_labels.push_back(labels[data_indexes[i]]);
            }
        }
    }

    int *new_indexes = new int[sample_num];
    for (int i = 0; i < sample_num; i++) {
        new_indexes[i] = data_indexes[i];
    }

    // send the data_indexes to the other client, and the other client splits in the same way
    for (int i = 0; i < client_num; i++) {
        if (i != client_id) {
            std::string s;
            serialize_batch_ids(new_indexes, sample_num, s);
            send_long_messages(channels[i].get(), s);
        }
    }

    delete [] new_indexes;


    logger(stdout, "End split dataset\n");
}

void Client::split_datasets_with_indexes(int new_indexes[], float split) {
    logger(stdout, "Begin split dataset with indexes\n");

    int training_data_size = sample_num * 0.8;
    int testing_data_size = sample_num - training_data_size;

    // select the former training data size as training data, and the latter as testing data
    for (int i = 0; i < sample_num; i++) {
        if (i < training_data_size) {
            // add to training dataset and labels
            training_data.push_back(local_data[new_indexes[i]]);
            if (has_label) {
                training_labels.push_back(labels[new_indexes[i]]);
            }
        } else {
            // add to testing dataset and labels
            testing_data.push_back(local_data[new_indexes[i]]);
            if (has_label) {
                testing_labels.push_back(labels[new_indexes[i]]);
            }
        }
    }

    logger(stdout, "End split dataset with indexes\n");
}

std::vector<int> Client::compute_binary_vector(DecisionTree* dt, int sample_id, std::map<int, int> node_index_2_leaf_index_map) {

    vector<float> sample_values = testing_data[sample_id];
    std::vector<int> binary_vector(dt->internal_node_num + 1);

    // traverse the whole tree iteratively, and compute binary_vector
    std::stack<PredictionObj> traverse_prediction_objs;
    PredictionObj prediction_obj(dt->tree_nodes[0].is_leaf, dt->tree_nodes[0].is_self_feature, dt->tree_nodes[0].best_client_id,
            dt->tree_nodes[0].best_feature_id, dt->tree_nodes[0].best_split_id, 1, 0);
    traverse_prediction_objs.push(prediction_obj);
    while (!traverse_prediction_objs.empty()) {
        PredictionObj pred_obj = traverse_prediction_objs.top();
        if (pred_obj.is_leaf == 1) {
            // find leaf index and record
            int leaf_index = node_index_2_leaf_index_map.find(pred_obj.index)->second;
            binary_vector[leaf_index] = pred_obj.mark;
            traverse_prediction_objs.pop();
        } else if (pred_obj.is_self_feature != 1) {
            // both left and right branches are marked as 1 * current_mark
            traverse_prediction_objs.pop();
            int left_node_index = pred_obj.index * 2 + 1;
            int right_node_index = pred_obj.index * 2 + 2;

            PredictionObj left(dt->tree_nodes[left_node_index].is_leaf, dt->tree_nodes[left_node_index].is_self_feature,
                    dt->tree_nodes[left_node_index].best_client_id, dt->tree_nodes[left_node_index].best_feature_id,
                    dt->tree_nodes[left_node_index].best_split_id, pred_obj.mark, left_node_index);
            PredictionObj right(dt->tree_nodes[right_node_index].is_leaf, dt->tree_nodes[right_node_index].is_self_feature,
                    dt->tree_nodes[right_node_index].best_client_id, dt->tree_nodes[right_node_index].best_feature_id,
                    dt->tree_nodes[right_node_index].best_split_id, pred_obj.mark, right_node_index);
            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        } else {
            // is self feature, retrieve split value and compare
            traverse_prediction_objs.pop();
            int feature_id = pred_obj.best_feature_id;
            int split_id = pred_obj.best_split_id;
            float split_value = dt->features[feature_id].split_values[split_id];
            int left_mark, right_mark;
            if (sample_values[feature_id] <= split_value) {
                left_mark = pred_obj.mark * 1;
                right_mark = pred_obj.mark * 0;
            } else {
                left_mark = pred_obj.mark * 0;
                right_mark = pred_obj.mark * 1;
            }

            int left_node_index = pred_obj.index * 2 + 1;
            int right_node_index = pred_obj.index * 2 + 2;
            PredictionObj left(dt->tree_nodes[left_node_index].is_leaf, dt->tree_nodes[left_node_index].is_self_feature,
                    dt->tree_nodes[left_node_index].best_client_id, dt->tree_nodes[left_node_index].best_feature_id,
                    dt->tree_nodes[left_node_index].best_split_id, left_mark, left_node_index);
            PredictionObj right(dt->tree_nodes[right_node_index].is_leaf, dt->tree_nodes[right_node_index].is_self_feature,
                    dt->tree_nodes[right_node_index].best_client_id, dt->tree_nodes[right_node_index].best_feature_id,
                    dt->tree_nodes[right_node_index].best_split_id, right_mark, right_node_index);

            traverse_prediction_objs.push(left);
            traverse_prediction_objs.push(right);
        }
    }

    return binary_vector;
}

void Client::test_accuracy(std::vector<DecisionTree>& random_forest, int num_trees, float &accuracy) {
    logger(stdout, "Begin test accuracy on testing dataset\n");

    // compute public key size in encoded number
    mpz_t n;
    mpz_init(n);
    mpz_sub_ui(n, m_pk->g, 1);

    int correct_num = 0;
    // for each sample
    for (int i = 0; i < testing_data.size(); ++i) {
        float cumulated_label = 0;
        //  for each decision tree
        for (int tree_index = 0; tree_index < num_trees; ++tree_index) {
            logger(stdout, "Processing tree[%d]:\n", tree_index);

            // step 1: organize the leaf label vector, compute the map
            logger(stdout, "internal_node_num = %d\n", random_forest[tree_index].internal_node_num);
            EncodedNumber *label_vector = new EncodedNumber[random_forest[tree_index].internal_node_num + 1];

            std::map<int, int> node_index_2_leaf_index_map;
            int leaf_cur_index = 0;
            for (int i = 0; i < pow(2, random_forest[tree_index].max_depth + 1) - 1; i++) {
                if (random_forest[tree_index].tree_nodes[i].is_leaf == 1) {
                    node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
                    label_vector[leaf_cur_index] = random_forest[tree_index].tree_nodes[i].label;  // record leaf label vector
                    leaf_cur_index ++;
                }
            }

            // compute binary vector for the current sample
            std::vector<int> binary_vector = compute_binary_vector(&random_forest[tree_index], i, node_index_2_leaf_index_map);
            EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
            EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];

            // update in Robin cycle, from the last client to client 0
            if (client_id == client_num - 1) {

                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(m_pk, updated_label_vector[j], label_vector[j], encoded_binary_vector[j]);
                }

                // send to the next client
                std::string send_s;
                serialize_batch_sums(updated_label_vector, binary_vector.size(), send_s);
                send_long_messages(channels[client_id - 1].get(), send_s);

            } else if (client_id > 0) {

                std::string recv_s;
                recv_long_messages(channels[client_id + 1].get(), recv_s);
                int recv_size; // should be same as binary_vector.size()
                deserialize_sums_from_string(updated_label_vector, recv_size, recv_s);
                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
                }

                std::string resend_s;
                serialize_batch_sums(updated_label_vector, binary_vector.size(), resend_s);
                send_long_messages(channels[client_id - 1].get(), resend_s);

            } else {

                // the super client update the last, and aggregate before calling share decryption
                std::string final_recv_s;
                recv_long_messages(channels[client_id + 1].get(), final_recv_s);
                int final_recv_size;
                deserialize_sums_from_string(updated_label_vector, final_recv_size, final_recv_s);
                for (int j = 0; j < binary_vector.size(); j++) {
                    encoded_binary_vector[j].set_integer(n, binary_vector[j]);
                    djcs_t_aux_ep_mul(m_pk, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
                }
            }

            // aggregate and call share decryption
            if (client_id == 0) {

                EncodedNumber *encrypted_aggregation = new EncodedNumber[1];
                encrypted_aggregation[0].set_float(n, 0, 2 * FLOAT_PRECISION);
                djcs_t_aux_encrypt(m_pk, m_hr, encrypted_aggregation[0], encrypted_aggregation[0]);
                for (int j = 0; j < binary_vector.size(); j++) {
                    djcs_t_aux_ee_add(m_pk, encrypted_aggregation[0], encrypted_aggregation[0], updated_label_vector[j]);
                }

                EncodedNumber *decrypted_label = new EncodedNumber[1];
                share_batch_decrypt(encrypted_aggregation, decrypted_label, 1);

                float decoded_label;
                decrypted_label->decode(decoded_label);

                float rounded_decoded_label;
                if (decoded_label >= 0.5) {
                    rounded_decoded_label = 1.0;
                } else {
                    rounded_decoded_label = 0.0;
                }

                logger(stdout, "Tree[%d] decoded_label = %f\n", tree_index, decoded_label);

                cumulated_label += rounded_decoded_label;

            } else {
                std::string s, response_s;
                recv_long_messages(channels[0].get(), s);
                decrypt_batch_piece(s, response_s, 0);
            }

            delete [] label_vector;
        }
        cumulated_label /= num_trees;
        if (cumulated_label >= 0.5) {
            cumulated_label = 1.0;
        } else {
            cumulated_label = 0.0;
        }
        if (cumulated_label == testing_labels[i]) {
            correct_num += 1;
        }
    }
    accuracy = (float) correct_num / (float) testing_labels.size();
    logger(stdout, "End test accuracy on testing dataset\n");
}

bool Client::generate_djcs_t_keys(int epsilon_s, int key_size, int client_num, int required_client_num)
{
    // Initialize data structures
    djcs_t_public_key *param_pk = djcs_t_init_public_key();
    djcs_t_private_key *param_vk = djcs_t_init_private_key();
    hcs_random *param_hr = hcs_init_random();

    // Generate a key pair with modulus of size key_size bits
    djcs_t_generate_key_pair(param_pk, param_vk, param_hr, epsilon_s,
            key_size, required_client_num, client_num);

    mpz_t *coeff = djcs_t_init_polynomial(param_vk, param_hr);
    djcs_t_auth_server **param_au =
            (djcs_t_auth_server **)malloc(client_num * sizeof(djcs_t_auth_server *));
    mpz_t *si = (mpz_t *)malloc(required_client_num * sizeof(mpz_t));
    for (int i = 0; i < client_num; i++) {
        mpz_init(si[i]);
        djcs_t_compute_polynomial(param_vk, coeff, si[i], i);
        // TODO: send to corresponding client via network (param_pk, param_hr, si, i)
        //param_au[i] = pcs_t_init_auth_server();
        //pcs_t_set_auth_server(param_au[i], si[i], i);
    }
}


void Client::set_keys(djcs_t_public_key *param_pk, hcs_random *param_hr, mpz_t si, unsigned long i)
{
    // set the keys when receiving from the trusted third party
    // copy values of pk
    m_pk->s = param_pk->s;
    m_pk->l = param_pk->l;
    m_pk->w = param_pk->w;
    mpz_set(m_pk->g, param_pk->g);
    mpz_set(m_pk->delta, param_pk->delta);
    m_pk->n = (mpz_t *) malloc(sizeof(mpz_t) * (m_pk->s+1));
    for (int i = 0; i < m_pk->s + 1; i++) {
        mpz_init_set(m_pk->n[i], param_pk->n[i]);
    }

    // copy values of hr
    gmp_randinit_set(m_hr->rstate, param_hr->rstate);

    //m_pk = param_pk;
    //m_hr = param_hr;
    djcs_t_set_auth_server(m_au, si, i);
}


void Client::recv_set_keys(std::string recv_keys) {

    com::collaborative::ml::PB_Keys deserialized_pb_keys;
    if (!deserialized_pb_keys.ParseFromString(recv_keys)) {
        logger(stdout, "Failed to parse PB_Keys from string\n");
        return;
    }

    // set public key
    m_pk->s = deserialized_pb_keys.public_key_s();
    m_pk->l = deserialized_pb_keys.public_key_l();
    m_pk->w = deserialized_pb_keys.public_key_w();
    mpz_t g, delta;
    mpz_init(g);
    mpz_init(delta);
    mpz_set_str(g, deserialized_pb_keys.public_key_g().c_str(), 10);
    mpz_set_str(delta, deserialized_pb_keys.public_key_delta().c_str(), 10);
    mpz_set(m_pk->g, g);
    mpz_set(m_pk->delta, delta);
    m_pk->n = (mpz_t *) malloc(sizeof(mpz_t) * (m_pk->s+1));
    for (int i = 0; i < deserialized_pb_keys.public_key_n_size(); i++) {
        mpz_t ni;
        mpz_init(ni);
        mpz_set_str(ni, deserialized_pb_keys.public_key_n(i).n().c_str(), 10);
        mpz_init_set(m_pk->n[i], ni);
        mpz_clear(ni);
    }

    // set auth server
    mpz_t si;
    mpz_init(si);
    mpz_set_str(si, deserialized_pb_keys.auth_server_si().c_str(), 10);
    djcs_t_set_auth_server(m_au, si, deserialized_pb_keys.auth_server_i());

//    gmp_printf("m_pk->g = %Zd\n", m_pk->g);
//    gmp_printf("m_pk->delta = %Zd\n", m_pk->delta);
//    gmp_printf("m_au->si = %Zd\n", m_au->si);

    mpz_clear(g);
    mpz_clear(delta);
    mpz_clear(si);
}


void Client::serialize_send_keys(std::string &send_keys, djcs_t_public_key *pk, mpz_t si, int i) {

    com::collaborative::ml::PB_Keys pb_keys;

    // serialize public key
    pb_keys.set_public_key_s(pk->s);
    pb_keys.set_public_key_w(pk->w);
    pb_keys.set_public_key_l(pk->l);
    std::string g_str, delta_str;
    g_str = mpz_get_str(NULL, 10, pk->g);
    delta_str = mpz_get_str(NULL, 10, pk->delta);
    pb_keys.set_public_key_g(g_str);
    pb_keys.set_public_key_delta(delta_str);
    if (pk->n != NULL) {
        for (int i = 0; i < pk->s + 1; i++) {
            com::collaborative::ml::PB_PublicN *pn = pb_keys.add_public_key_n();
            std::string ni_str;
            ni_str = mpz_get_str(NULL, 10, pk->n[i]);
            pn->set_n(ni_str);
            //pb_keys.set_public_key_n(i, ni_str);
        }
    }

    // serialize auth server
    pb_keys.set_auth_server_i(i);
    std::string si_str;
    si_str = mpz_get_str(NULL, 10, si);
    pb_keys.set_auth_server_si(si_str);

    pb_keys.SerializeToString(&send_keys);
}


void Client::share_batch_decrypt(EncodedNumber *ciphers, EncodedNumber *& decrypted_res, int size) {

    mpz_t **dec = (mpz_t **) malloc (size * sizeof(mpz_t *));
    for (int i = 0; i < size; i++) {
        dec[i] = (mpz_t *) malloc (client_num * sizeof(mpz_t));

        // copy ciphers attributes to decrypted_res
        mpz_set(decrypted_res[i].n, ciphers[i].n);
        decrypted_res[i].exponent = ciphers[i].exponent;
        decrypted_res[i].type = Plaintext;
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < client_num; j++) {
            mpz_init(dec[i][j]);
        }
    }

    // decrypt by its own
    for (int i = 0; i < size; i++) {
        djcs_t_share_decrypt(m_pk, m_au, dec[i][client_id], ciphers[i].value);
    }

    // serialize ciphers and send to the other clients
    std::string send_ciphers;
    serialize_batch_sums(ciphers, size, send_ciphers);
    for (int j = 0; j < client_num; j++) {
        if (j != client_id) {
            std::string recv_message_i;
            auto *recv_share_i = new EncodedNumber[size];
            send_long_messages(channels[j].get(), send_ciphers);

            // receive the response messages and deserialize to decrypted shares
            recv_long_messages(channels[j].get(), recv_message_i);

            int x;
            deserialize_sums_from_string(recv_share_i, x, recv_message_i);
            for (int i = 0; i < size; i++) {
                mpz_set(dec[i][j], recv_share_i[i].value);
            }
        }
    }

    // combine all the shares and get the final plaintexts
    for (int i = 0; i < size; i++) {
        mpz_t t;
        mpz_init(t);
        djcs_t_share_combine(m_pk, t, dec[i]);
        mpz_set(decrypted_res[i].value, t);
        mpz_clear(t);
    }
}


void Client::decrypt_batch_piece(std::string s, std::string & response_s, int src_client_id) {

    // deserialization
    EncodedNumber *ciphers;
    int size;
    deserialize_sums_from_string(ciphers, size, s);

    // share decrypt
    for (int i = 0; i < size; i++) {
        mpz_t t;
        mpz_init(t);
        djcs_t_share_decrypt(m_pk, m_au, t, ciphers[i].value);
        ciphers[i].type = Plaintext;
        mpz_set(ciphers[i].value, t);
        mpz_clear(t);
    }

    // serialization and return
    serialize_batch_sums(ciphers, size, response_s);

    send_long_messages(channels[src_client_id].get(), response_s);
}


void Client::write_random_shares(std::vector<float> shares, std::string path) {

    ofstream write_file;
    std::string file_name = path + "input/player" + std::to_string(client_id) + ".txt";
    write_file.open(file_name);

    if (!write_file.is_open()) {
        logger(stdout, "open file %s failed\n", file_name.c_str());
        return;
    }

    std::string str;
    for (int i = 0; i < shares.size(); i++) {
        std::ostringstream ss;
        ss << shares[i];
        string str(ss.str());
        if (i != shares.size() - 1) {
            write_file << str << "\n";
        } else {
            write_file << str;
        }
    }

    write_file.close();
}


std::vector<float> Client::read_random_shares(int size, std::string path) {

    ifstream read_file;
    std::string file_name = path + "output/output" + std::to_string(client_id) + ".txt";
    read_file.open(file_name);

    if (!read_file.is_open()) {
        logger(stdout, "open file %s failed\n", file_name.c_str());
        exit(1);
    }

    logger(stdout, "Starting read file\n");

    std::string line;
    std::vector<float> shares;
    while (getline(read_file, line)) {
        if (line != "") {
            float x = ::atof(line.c_str());
            shares.push_back(x);
        }
    }

    if (shares.size() != size) {
        logger(stdout, "Read share number is not equal to batch size\n");
        exit(1);
    }

    read_file.close();

    return shares;
}


void Client::send_messages(CommParty* comm_party, string message) {
    print_send_message(message);
    comm_party->write((const byte *) message.c_str(), message.size());
}


void Client::send_long_messages(CommParty *comm_party, string message) {
    //print_send_message(message);
    comm_party->writeWithSize(message);
}


void Client::recv_messages(CommParty* comm_party, string messages, byte * buffer, int expected_size) {
    comm_party->read(buffer, expected_size);
    // the size of all strings is 2. Parse the message to get the original strings
    auto s = string(reinterpret_cast<char const*>(buffer), expected_size);
    //print_recv_message(s);
    messages = s;
}


void Client::recv_long_messages(CommParty *comm_party, std::string &message) {
    vector<byte> resMsg;
    comm_party->readWithSizeIntoVector(resMsg);
    const byte * uc = &(resMsg[0]);
    string resMsgStr(reinterpret_cast<char const*>(uc), resMsg.size());
    message = resMsgStr;
}


void Client::print_send_message(const string  &s) {
    logger(stdout, "Sending message: %s\n", s.c_str());
}


void Client::print_recv_message(const string &s) {
    logger(stdout, "Receiving message: %s\n", s.c_str());
}


void Client::print_local_data() {
    for (int i = 0; i < sample_num; i++) {
        std::cout<<"sample "<<i<<" = ";
        for (int j = 0; j < feature_num; j++) {
            if (j != feature_num - 1) {
                std::cout<<local_data[i][j]<<", ";
            } else {
                std::cout<<local_data[i][j]<<std::endl;
            }
        }
    }
}


void Client::print_labels() {
    if (!has_label) return;

    for (int i = 0; i < sample_num; i++) {
        std::cout<<"sample "<<i<<" label = "<<labels[i]<<std::endl;
    }
}


Client::~Client() {

    // free local data
    local_data.clear();
    local_data.shrink_to_fit();

    // free labels if has_label == true
    if (has_label) {
        labels.clear();
        labels.shrink_to_fit();
    }

    // free channels
    //std::vector< shared_ptr<CommParty> >().swap(channels);

    for (int i = 0; i < channels.size(); i++) {
        channels[i].reset();
    }

    channels.clear();
    channels.shrink_to_fit();

    // free keys
    djcs_t_free_public_key(m_pk);
    djcs_t_free_auth_server(m_au);
    hcs_free_random(m_hr);
}
