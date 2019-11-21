### Guidelines
To communicate with MP-SPDZ programs, the following steps should be done.
1. Go to MP-SPDZ directory, bash fast-make.sh to generate pre-requisite programs and parameters;
2. Go to MP-SPDZ directory, make mpir to generate required mpir lib for using together with CollaborativeML;
3. Go to MP-SPDZ directory, ./compile.py Programs/Source/vfl_logistic_func.mpc, to compile the mpc program;
4. Go to the spdz_util.cpp file, change the filename in initialise_fields function to the file path in ${SPDZ_HOME}/Player-Data/3-128-128/Params-Data;
5. build CollaborativeML;
6. Go to MP-SPDZ directory, open three terminals, execute the following commands, respectively:
    (1) ./semi-party.x -N 3 -I -p 0 vfl_logistic_func
    (2) ./semi-party.x -N 3 -I -p 1 vfl_logistic_func
    (3) ./semi-party.x -N 3 -I -p 2 vfl_logistic_func
7. Open three terminals in CollaborativeML/build, execute the following commands, respectively:
    (1) ./CollaborativeML 0
    (2) ./CollaborativeML 1
    (3) ./CollaborativeML 2