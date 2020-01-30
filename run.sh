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
    ${shell_path}/build/CollaborativeML $id 3 2 1 0 0 0 /home/wuyuncheng/Documents/projects/CollaborativeML/data/networks/Parties.txt bank_marketing_data
done

if [ $id -eq 0 ]
then
    cat $record_file | awk '{sum += $1} END {print "Average accuracy = ", sum/$num_exp}'
fi
