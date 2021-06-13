# Pivot
This is the implementation of our paper: 
"[Privacy preserving vertical federated learning for tree-based models](http://www.vldb.org/pvldb/vol13/p2090-wu.pdf)",
which is published in PVLDB 2020. This paper proposes a private and efficient solution for tree-based models,
including **decision tree (DT)**, **random forest (RF)**, and **gradient boosting decision tree (GBDT)**, 
under the **vertical federated learning (VFL)** setting. The solution is based on a hybrid of threshold partially
homomorphic encryption (TPHE) and secure multiparty computation (MPC) techniques.

## Dependencies
+ [Pivot-SPDZ](https://github.com/lemonviv/Pivot-SPDZ)
    + This is a fork of [MP-SPDZ](https://github.com/data61/MP-SPDZ/tree/2c3606ccb2658cea10826d670298e04b1385415a) repository. We 
    have revised some codes and configurations in this repository. The Pivot program
    calls Pivot-SPDZ as a library. 
    + Clone Pivot-SPDZ and follow the guide in [MP-SPDZ](https://github.com/data61/MP-SPDZ) to install it.
+ [libhcs](https://github.com/lemonviv/libhcs) 
    + This is a fork of [libhcs](https://github.com/tiehuis/libhcs). We have 
    fixed a threshold decryption bug in the original repo. Pivot uses libhcs for threshold homomorphic encryption computations. 
    + Clone this repository and follow the guide in [libhcs](https://github.com/tiehuis/libhcs) to install it.
+ [libscapi](https://github.com/cryptobiu/libscapi/tree/b77816a8ad09181be319316f4023f628ab7ffb88)
    + Pivot uses libscapi for network communications among clients. 
    + Clone this repository and follow the guide in [libscapi](https://github.com/cryptobiu/libscapi) to install it.
+ Python
    + We implemented the non-private baselines and generated the synthetic 
     datasets using sklearn.
    + Install the necessary dependencies of python (see tools/README.md).
+ Protobuf
    + We used protobuf version 3.14.0 for the messages communicated among clients. 

## Executions

### Configuration
 * In Pivot, update the following if needed:
    + `data/networks/Parties.txt`: defining the participating parties' ip addresses and ports
    + `src/include/common.h`: 
        + `DEFAULT_PARAM_DATA_FILE`: the SPDZ related party file (in the Pivot-SPDZ folder)
        + `SPDZ_PORT_NUM_DT`: the port for connecting to SPDZ decision tree MPC program
        + `SPDZ_PORT_NUM_DT_ENHANCED`: the port for connecting to SPDZ decision tree prediction of the enhanced protocol
    + other algorithm-related default parameters in `src/include/common.h`: e.g., the number of parties
    + revise ${SPDZ_HOME} in CMakeLists.txt to ${PIVOT_SPDZ_HONE}
 * In Pivot-SPDZ, update the following if needed:
    + `Programs/Source/vfl_decision_tree.mpc`
        + `PORT_NUM`: same as `SPDZ_PORT_NUM_DT`
        + `MAX_NUM_CLIENTS`: the maximum number of clients could handle, default is set to 3
        + `MAX_CLASSES_NUM`: the maximum number of classes for classification (by default is 2 for regression)
        + other algorithm-related parameters
    + `Programs/Source/vfl_dt_enhanced_prediction.mpc`
        + `PORT_NUM`: same as `SPDZ_PORT_NUM_DT_ENHANCED`
        + `MAX_NUM_CLIENTS`: the maximum number of clients could handle, default is set to 3
        + `MAX_TREE_DEPTH`: the maximum depth of the evaluated tree, must be the same as in Pivot
        + `TESTING_NUM`: the number of samples in the testing stage, must be the exact at the moment
    + `fast-make.sh`: modify Setup.x and setup-online.sh (default is 3 clients and the security parameter is 128 bits)

### Build programs
 * Build Pivot-SPDZ
    + cd ${PIVOT_SPDZ_HOME}, make sure that `MY_CFLAGS = -DINSECURE` is in the CONFIG.mine file (for running fake online protocol)
    + cd ${PIVOT_SPDZ_HOME}, `make mpir` to generate required mpir lib;
    + cd ${PIVOT_SPDZ_HOME}, run `bash fast-make.sh` to generate pre-requisite programs and parameters;
    + compile the MPC programs
    <pre><code>
    ./compile.py ${PIVOT_SPDZ_HOME}/Programs/Source/vfl_decision_tree.mpc
    ./compile.py ${PIVOT_SPDZ_HOME}/Programs/Source/vfl_dt_enhanced_prediction.mpc
    </code></pre>
 * Build Pivot
    + build the program as follows:
     ```
        mkdir build 
        cmake -Bbuild -H.
        cd build/
        make
     ```

### Basic protocol
 * To run the Pivot training, for example, the DT algorithm with 3 clients, execute:
    + cd ${PIVOT_SPDZ_HOME}, run 3 MPC programs in separate terminals
        <pre><code>
        ./semi-party.x -F -N 3 -I -p 0 vfl_decision_tree
        ./semi-party.x -F -N 3 -I -p 1 vfl_decision_tree
        ./semi-party.x -F -N 3 -I -p 2 vfl_decision_tree
        </code></pre>
    + cd ${PIVOT_HOME}, run 3 programs in separate terminals for DT model
        <pre><code>
        ./Pivot --client-id 0 --client-num 3 --class-num 2 --algorithm-type 0 
                --tree-type 0 --solution-type 0 --optimization-type 1 
                --network-file ${PIVOT_HOME}/data/networks/Parties.txt 
                --data-file ${PIVOT_HOME}/data/bank_marketing_data/client_0.txt 
                --logger-file ${PIVOT_HOME}/log/release_test/bank_marketing_data 
                --max-bins 16 --max-depth 3 --num-trees 1 
        ./Pivot --client-id 1 --client-num 3 --class-num 2 --algorithm-type 0
                --tree-type 0 --solution-type 0 --optimization-type 1 
                --network-file ${PIVOT_HOME}/data/networks/Parties.txt 
                --data-file ${PIVOT_HOME}/data/bank_marketing_data/client_1.txt 
                --logger-file ${PIVOT_HOME}/log/release_test/bank_marketing_data
                 --max-bins 16 --max-depth 3 --num-trees 1
        ./Pivot --client-id 2 --client-num 3 --class-num 2 --algorithm-type 0
                --tree-type 0 --solution-type 0 --optimization-type 1 
                --network-file ${PIVOT_HOME}/data/networks/Parties.txt 
                --data-file ${PIVOT_HOME}/data/bank_marketing_data/client_2.txt 
                --logger-file ${PIVOT_HOME}/log/release_test/bank_marketing_data 
                --max-bins 16 --max-depth 3 --num-trees 1
        </code></pre>
    + the description of parameters in the Pivot program is as follows: 
        + `client-id denotes the id of the client, the super client's id is 0 by default` 
        + `client-num denotes the total number of participating clients`
        + `class-num denotes the the number of classes in the classification, regression set to 1 by default`
        + `algorithm-type: 0 for decision tree, 1 for random forest, 2 for GBDT`
        + `tree-type: 0 for classification, 1 for regression`
        + `solution-type: 0 for basic protocol, 1 for enhanced protocol`
        + `optimization-type: currently set to 1`
        + `network-file: the path that describes the party information, including ip and port`
        + `data-file: the dataset used for this client`
        + `logger-file: the log file path`
        + `max-bins: the maximum number of splits for any feature`
        + `max-depth: the maximum tree depth during training`
        + `num-trees: the number of trees for ensemble algorithms (for DT set to 1 by default)`
 * To run RF and GBDT model, modify the corresponding parameter for invoking Pivot 

### Enhanced protocol
 * To run the enhanced protocol, besides of modifying the corresponding parameter for invoking Pivot,
 need to run another MPC program `vfl_dt_enhanced_prediction` for the model prediction stage.
    + cd ${PIVOT_SPDZ_HOME}, run another 3 MPC programs in separate terminals
        <pre><code>
        ./semi-party.x -F -N 3 -I -p 0 -pn 6000 vfl_dt_enhanced_prediction
        ./semi-party.x -F -N 3 -I -p 1 -pn 6000 vfl_dt_enhanced_prediction
        ./semi-party.x -F -N 3 -I -p 2 -pn 6000 vfl_dt_enhanced_prediction
        </code></pre>
    + the above `-pn` parameter is the port for `vfl_dt_enhanced_prediction` connections (if not specified, 
    default is 5000, as used for `vfl_decision_tree`)