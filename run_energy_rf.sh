#!/bin/bash

shell_path=$(cd "$(dirname "$0")";pwd)
record_file="result.log"
num_exp=10

while getopts "p:n:" opt
do
    case $opt in
	    p)
	        id=${OPTARG}
	        ;;
	    n)
	        num_exp=${OPTARG}
	        ;;
	    *)
	        echo "Usage: ./run.sh -p <client_id> -n <num_experiments>"
            exit 1;;	
    esac
done

for i in {0..9}
do
    sleep $id	
    ${shell_path}/build/CollaborativeML $id 3 2 1 1 0 0 /home/wuyuncheng/Documents/projects/CollaborativeML/data/networks/Parties.txt appliances_energy_prediction_data
    sleep $id
done

if [ $id -eq 0 ]
then
    cat $record_file | awk '{sum += $1} END {print "Average accuracy = ", sum/$num_exp}'
fi
