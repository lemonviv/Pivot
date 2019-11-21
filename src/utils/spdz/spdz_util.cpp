//
// Created by wuyuncheng on 20/11/19.
//

#include "spdz_util.h"
#include "math.h"

#define SPDZ_FIXED_PRECISION 16

void send_private_batch_shares(std::vector<float> shares, std::vector<int>& sockets, int n_parties) {

    int number_inputs = shares.size();
    std::vector<long> long_shares(number_inputs);

    // step 1: convert to int or long according to the fixed precision
    for (int i = 0; i < number_inputs; ++i) {
        long_shares[i] = static_cast<int>(round(shares[i] * pow(2, SPDZ_FIXED_PRECISION)));
    }

    // step 2: convert to the gfp value and call send_private_inputs
    // Map inputs into gfp
    vector<gfp> input_values_gfp(number_inputs);
    for (int i = 0; i < number_inputs; i++) {
        input_values_gfp[i].assign(long_shares[i]);
    }

    // Run the computation
    send_private_inputs(input_values_gfp, sockets, n_parties);
    cout << "Sent private inputs to each SPDZ engine, waiting for result..." << endl;
}


std::vector<int> setup_sockets(int n_parties, const std::string host_name, int port_base) {

    // Setup connections from this client to each party socket
    std::vector<int> sockets(n_parties);
    for (int i = 0; i < n_parties; i++)
    {
        set_up_client_socket(sockets[i], host_name.c_str(), port_base + i);
        cout << "set up for " << i << "-th party succeed" << endl;
    }
    cout << "Finish setup socket connections to SPDZ engines." << endl;
    return sockets;
}


void send_private_inputs(std::vector<gfp>& values, std::vector<int>& sockets, int n_parties)
{
    int num_inputs = values.size();
    octetStream os;
    std::vector< std::vector<gfp> > triples(num_inputs, vector<gfp>(3));
    std::vector<gfp> triple_shares(3);

    // Receive num_inputs triples from SPDZ
    for (int j = 0; j < n_parties; j++)
    {
        os.reset_write_head();
        os.Receive(sockets[j]);

        for (int j = 0; j < num_inputs; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                triple_shares[k].unpack(os);
                triples[j][k] += triple_shares[k];
            }
        }
    }

    // Check triple relations (is a party cheating?)
    for (int i = 0; i < num_inputs; i++)
    {
        if (triples[i][0] * triples[i][1] != triples[i][2])
        {
            cerr << "Incorrect triple at " << i << ", aborting\n";
            exit(1);
        }
    }

    // Send inputs + triple[0], so SPDZ can compute shares of each value
    os.reset_write_head();
    for (int i = 0; i < num_inputs; i++)
    {
        gfp y = values[i] + triples[i][0];
        y.pack(os);
    }
    for (int j = 0; j < n_parties; j++)
        os.Send(sockets[j]);
}



void initialise_fields(const string& dir_prefix)
{
    int lg2;
    bigint p;

    string filename = dir_prefix + "Params-Data";
    cout << "loading params from: " << filename << endl;

    ifstream inpf(filename.c_str());
    if (inpf.fail()) { throw file_error(filename.c_str()); }
    inpf >> p;
    inpf >> lg2;

    inpf.close();

    gfp::init_field(p);
    gf2n::init_field(lg2);
}


std::vector<float> receive_result(std::vector<int>& sockets, int n_parties, int size)
{
    std::vector<gfp> output_values(size);
    octetStream os;
    for (int i = 0; i < n_parties; i++)
    {
        os.reset_write_head();
        os.Receive(sockets[i]);
        for (int j = 0; j < size; j++)
        {
            gfp value;
            value.unpack(os);
            output_values[j] += value;
        }
    }

    std::vector<float> res_shares(size);

    for (int i = 0; i < size; i++) {
        gfp val = output_values[i];
        bigint aa;
        to_signed_bigint(aa, val);
        long t = aa.get_si();
        //cout<< "i = " << i << ", t = " << t <<endl;
        res_shares[i] = static_cast<float>(t * pow(2, -SPDZ_FIXED_PRECISION));
    }

    return res_shares;
}