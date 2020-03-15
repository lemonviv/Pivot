# Pivot
Privacy preserving vertical federated learning on tree-based models

# Dependencies
+ Pivot-SPDZ
    + This is a fork of MP-SPDZ repository (https://github.com/data61/MP-SPDZ). 
    + We revise some codes and configurations in this repository. The Pivot program
    calls Pivot-SPDZ as a library.  
    + Git clone Pivot-SPDZ and install it.
+ Libhcs (https://github.com/tiehuis/libhcs)
    + Pivot uses libhcs for threshold homomorphic encryption computations. 
    + Git clone this repository and install it.
    + We fix a threshold decryption bug in libhcs. Replace xx.cpp in the 
    libhcs project and re-install this library.
+ Libscapi (https://github.com/cryptobiu/libscapi)
    + Pivot uses libscapi for network communications among clients. 
    + Git clone this repository and install it.
+ Python
    + We implemented the non-private baselines and generated the synthetic 
     datasets using sklearn.
    + Install the necessary dependencies of python (see tools/README.md).

# Running example
+ Make sure the required environment is setup correctly.
+ Build Pivot-SPDZ
    + cd ${PIVOT_SPDZ_HOME}, make sure that "MY_CFLAGS = -DINSECURE" is in the CONFIG.mine file;
    + cd ${PIVOT_SPDZ_HOME}, make mpir to generate required mpir lib for using together with Pivot;
    + cd ${PIVOT_SPDZ_HOME}, setup the desired parameters and run 
    bash fast-make.sh to generate pre-requisite programs and parameters;
    + ./compile.py ${PIVOT_SPDZ_HOME}/Programs/Source/vfl_decision_tree.mpc
+ Build Pivot
    + revise ${SPDZ_HOME} in CMakeLists.txt to ${PIVOT_SPDZ_HONE}
    + revise directory paths and parameters in ${PIVOT_HOME}/src/include/common.h
    + cd ${PIVOT_HOME}/build
    + bash build.sh (resulting in Pivot executable program)
+ Run with 3 clients
    + cd ${PIVOT_SPDZ_HOME}, run 3 programs in separate terminals
        + ./semi-party.x -F -N 3 -I -p 0 vfl_decision_tree
        + ./semi-party.x -F -N 3 -I -p 1 vfl_decision_tree
        + ./semi-party.x -F -N 3 -I -p 2 vfl_decision_tree
    + cd ${PIVOT_HOME}, run 3 programs in separate terminals
        + ./Pivot 0 3 2 0 0 0 1 ${PIVOT_HOME}/data/networks/Parties.txt bank_marketing_data 16 5
        + ./Pivot 1 3 2 0 0 0 1 ${PIVOT_HOME}/data/networks/Parties.txt bank_marketing_data 16 5
        + ./Pivot 2 3 2 0 0 0 1 ${PIVOT_HOME}/data/networks/Parties.txt bank_marketing_data 16 5
            + the 1st parameter is client id 
            + the 2nd is the total client number
            + the 3rd is the number of classes 
            + the 4th is algorithm type (0 for decision tree, 1 for random forest, 2 for GBDT)
            + the 5th is the tree type (0 for classification, 1 for regression),
            + the 6th is the protocol type (0 for basic, 1 for enhanced)
            + the 7th is the optimization type (currently set 1 by default)
            + the 8th is the file of ip and port of the clients
            + the 9th is the data folder under ${PIVOT_HOME}/data/
            + the 10th is the maximum number of splits for any feature
            + the 11th is the maximum tree depth
            + the 12th is the number of trees for ensemble algorithms.
    + then the program logs can be found in ${PIVOT_HOME}/log folder.
    