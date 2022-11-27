#!/bin/bash


number_of_failed=0
for filename in ./test*.in; do
    test_num=`echo $filename | cut -d'.' -f1`
    
    ./bp_main $filename > ${test_num}Yours.out
    diff -q ${test_num}.out ${test_num}Yours.out
    if [[ $? -ne 0 ]]; then
	number_of_failed=$(($number_of_failed+1))
	echo "*************************************"
    fi
done

echo Failed $number_of_failed
