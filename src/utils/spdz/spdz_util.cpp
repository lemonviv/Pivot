//
// Created by wuyuncheng on 20/11/19.
//

#include "spdz_util.h"
#include "math.h"

#define SPDZ_FIXED_PRECISION 16

void send_private_batch_shares(std::vector<float> shares, std::vector<int>& sockets, int n_parties) {

    int number_inputs = shares.size();
    std::vector<long> long_shares;

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


std::vector<int> setup_sockets(int n_parties, std::string host_name, int port_base) {

    // Setup connections from this client to each party socket
    std::vector<int> sockets(n_parties);
    for (int i = 0; i < n_parties; i++)
    {
        set_up_client_socket(sockets[i], host_name.c_str(), port_base + i);
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


// Receive shares of the result and sum together.
// Also receive authenticating values.
gfp receive_result(std::vector<int>& sockets, int n_parties)
{
    std::vector<gfp> output_values(3);
    octetStream os;
    for (int i = 0; i < n_parties; i++)
    {
        os.reset_write_head();
        os.Receive(sockets[i]);
        for (unsigned int j = 0; j < 3; j++)
        {
            gfp value;
            value.unpack(os);
            output_values[j] += value;
        }
    }

    if (output_values[0] * output_values[1] != output_values[2])
    {
        cerr << "Unable to authenticate output value as correct, aborting." << endl;
        exit(1);
    }
    return output_values[0];
}