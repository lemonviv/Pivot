### Guidelines
To communicate with MP-SPDZ programs, the following steps should be done.
0. Go to MP-SPDZ directory, make sure that "MY_CFLAGS = -DINSECURE" is in the CONFIG.mine file;
1. Go to MP-SPDZ directory, bash fast-make.sh to generate pre-requisite programs and parameters;
2. Go to MP-SPDZ directory, make mpir to generate required mpir lib for using together with CollaborativeML;
3. Go to MP-SPDZ directory, ./compile.py Programs/Source/vfl_logistic_func.mpc, to compile the mpc program;
4. Go to the spdz_util.cpp file, change the filename in initialise_fields function to the file path in ${SPDZ_HOME}/Player-Data/3-128-128/Params-Data;
5. build CollaborativeML;
6. Go to MP-SPDZ directory, open three terminals, execute the following commands, respectively:
    (1) ./semi-party.x -F -N 3 -I -p 0 vfl_logistic_func
    (2) ./semi-party.x -F -N 3 -I -p 1 vfl_logistic_func
    (3) ./semi-party.x -F -N 3 -I -p 2 vfl_logistic_func
    If running decision tree:
    (1) ./semi-party.x -F -N 3 -I -p 0 vfl_decision_tree
    (2) ./semi-party.x -F -N 3 -I -p 1 vfl_decision_tree
    (3) ./semi-party.x -F -N 3 -I -p 2 vfl_decision_tree
    If running enhanced solution prediction, use another port for listening (default 5000):
    (1) ./semi-party.x -F -N 3 -I -p 0 -pn 6000 vfl_dt_enhanced_prediction
    (2) ./semi-party.x -F -N 3 -I -p 1 -pn 6000 vfl_dt_enhanced_prediction
    (3) ./semi-party.x -F -N 3 -I -p 2 -pn 6000 vfl_dt_enhanced_prediction
7. Open three terminals in CollaborativeML/build, execute the following commands, respectively:
    (1) ./CollaborativeML 0
    (2) ./CollaborativeML 1
    (3) ./CollaborativeML 2
    For enhanced solution (where the 1st param is client id, 2nd param is client num, 3rd param is solution type, 4th param is optimization type):
    (1) ./CollaborativeML 0 3 1 0
    (2) ./CollaborativeML 1 3 1 0
    (3) ./CollaborativeML 2 3 1 0
8. For random forest with classification, should run additional majority find program in SPDZ
    (1) ./semi-party.x -F -N 3 -I -p 0 vfl_mode_computation
    (2) ./semi-party.x -F -N 3 -I -p 1 vfl_mode_computation
    (3) ./semi-party.x -F -N 3 -I -p 2 vfl_mode_computation

About data_spliter:
    Compile:
        g++ -o data_spliter data_spliter.cc
    Usage:
        ./data_spliter /path/to/data
    Output:
        three data sets (client_0.txt, client_1.txt, client2.txt)
        Only client_0.txt will include the labels
    Implement:
        (1) read data from the data file and store in a vector<vector<string>> table
        (2) calculate the number of samples and the number of attributes (including label)
        (3) set up three files named "client_0.txt", "client_1.txt" and " client_2.txt"
        (4) Attributes of the data in the table will be splitted to three parts,
            the first part will be transferred to client_2.txt,
            the second part will be transferred to client_1.txt,
            the third part including the labels will be transferred to client_0.txt

About run.sh
    Usage:
        ./run.sh -p <client_id> -n <num_experiments>