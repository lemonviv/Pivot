//
// Created by wuyuncheng on 24/10/19.
//

#include "Input_Output_File_Sfix.h"
#include "Exceptions/Exceptions.h"

// #include <chrono>
// #include <thread>
#include <unistd.h>
#include <string>
#include "math.h"


long Input_Output_File_Sfix::open_channel(unsigned int channel)
{
    cout << "Opening channel " << channel << endl;

    string filename;
    if (channel == 0) {
        cout << "Reserved in channel 0" << endl;
    } else if (channel > 0 && channel <= 10) {
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/player0.txt";
        inpf->open(filename);
    } else if (channel > 10 && channel <= 20){
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/player1.txt";
        inpf->open(filename);
    } else if (channel > 20 && channel <= 30) {
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/player2.txt";
        inpf->open(filename);
    } else if (channel > 30 && channel <= 40) {
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/output0.txt";
        outf->open(filename);
    } else if (channel > 40 && channel <= 50) {
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/output1.txt";
        outf->open(filename);
    } else if (channel > 50 && channel <= 60) {
        filename = "~/Documents/projects/CollaborativeML/tools/SCALE-MAMBA/batch_sfix/mpc_files/output2.txt";
        outf->open(filename);
    } else {
        cout << "Channel is not used " << endl;
    }

    if (0 < channel <= 30) {
        if (inpf->fail()) {
            cout << "Could not find input file!" << endl;
        } else {
            cout << "Found file " << filename << endl;
        }
    } else {
        if (outf->fail()) {
            cout << "Could not find output file!" << endl;
        } else {
            cout << "Found file " << filename << endl;
        }
    }

    return 0;
}

void Input_Output_File_Sfix::close_channel(unsigned int channel)
{
    cout << "Closing channel " << channel << endl;
    inpf->close();
    outf->close();
}


gfp Input_Output_File_Sfix::private_input_gfp(unsigned int channel)
{
    gfp y;
    if (channel == 0) {
        int i = 2;
        cout << "Sleeps for 540s now.." << endl;
        sleep(540);
        // this_thread::sleep_for(chrono::milliseconds(12000));
        cout << "Continue.." << endl;
        y.assign(i);
    } else {
        int NONE = -2147483648;  // INT_MIN
        float x;
        int var = -1;
        if (inpf->good()) {
            if (!inpf->eof()) {
                (*inpf) >> x;
                fix_float_v = static_cast<int>(round(x * pow(2, FIXED_PRECISION)));
                y.assign(fix_float_v);
            } else {
                var = NONE;
                y.assign(var);
            }
        } else {
            var = NONE;
            y.assign(var);
        }
    }
    return y;
}

void Input_Output_File_Sfix::private_output_gfp(const gfp &output, unsigned int channel)
{
    cout << "Output channel " << channel << endl;

    //int v, p, s, z;
    bigint aa;
    int t;
    if(channel % 2 == 0) { // negative
        to_bigint(aa, output);
        //gmp_printf("aa = %Zd\n", aa.get_mpz_t());
        //gmp_printf("output.pr() = %Zd\n", output.pr().get_mpz_t());
        mpz_t x;
        mpz_init(x);
        mpz_sub(x, aa.get_mpz_t(), output.pr().get_mpz_t());
        //gmp_printf("x = %Zd\n", x);
        t = mpz_get_si(x);
        //cout << "t = " << t << endl;
        mpz_clear(x);
        //t = aa.get_si();
    } else { //positive
        to_bigint(aa, output);
        t = aa.get_si();
    }

    // construct float and flush
    // representing fix_float_v * pow(2, - fix_float_f).
    fix_float_v = t;
    float x = 0.0;
    x = fix_float_v * pow(2, 0 - FIXED_PRECISION);
    cout<< "res = " << x << endl;
    std::ostringstream ss;
    ss << x;
    string str(ss.str());
    str += "\n";
    *outf << str;
}


gfp Input_Output_File_Sfix::public_input_gfp(unsigned int channel)
{
    cout << "Enter value on channel " << channel << " : ";
    word x;
    cin >> x;
    gfp y;
    y.assign(x);

    // Important to have this call in each version of public_input_gfp
    Update_Checker(y, channel);

    return y;
}

void Input_Output_File_Sfix::public_output_gfp(const gfp &output, unsigned int channel)
{
    cout << "Output channel " << channel << " : ";
    output.output(cout, true);
    cout << endl;
}

long Input_Output_File_Sfix::public_input_int(unsigned int channel)
{
    cout << "Enter value on channel " << channel << " : ";
    long x;
    cin >> x;

    // Important to have this call in each version of public_input_gfp
    Update_Checker(x, channel);

    return x;
}

void Input_Output_File_Sfix::public_output_int(const long output, unsigned int channel)
{
    cout << "Output channel " << channel << " : " << output << endl;
}

void Input_Output_File_Sfix::output_share(const Share &S, unsigned int channel)
{
    (*outf) << "Output channel " << channel << " : ";
    S.output(*outf, human);
}

Share Input_Output_File_Sfix::input_share(unsigned int channel)
{
    cout << "Enter value on channel " << channel << " : ";
    Share S;
    S.input(*inpf, human);
    return S;
}

void Input_Output_File_Sfix::trigger(Schedule &schedule)
{
    printf("Restart requested: Enter a number to proceed\n");
    int i;
    cin >> i;

    // Load new schedule file program streams, using the original
    // program name
    //
    // Here you could define programatically what the new
    // programs you want to run are, by directly editing the
    // public variables in the schedule object.
    unsigned int nthreads= schedule.Load_Programs();
    if (schedule.max_n_threads() < nthreads)
    {
        throw Processor_Error("Restart requires more threads, cannot do this");
    }
}

void Input_Output_File_Sfix::debug_output(const stringstream &ss)
{
    printf("%s", ss.str().c_str());
    fflush(stdout);
}

void Input_Output_File_Sfix::crash(unsigned int PC, unsigned int thread_num)
{
    printf("Crashing in thread %d at PC value %d\n", thread_num, PC);
    throw crash_requested();
}