#!/bin/bash

#test automation script
INPUT_DIR="tests/tests2"
OUT_BIN="bp_main"


echo -e 'building... '
make clean
make
chmod 777 $OUT_BIN


number_of_tests=`ls -l $INPUT_DIR | grep in* | wc -l`
echo -e 'number of tests to run: ' ${number_of_tests}

number_of_failed=0
for filename in ${INPUT_DIR}/test*.trc; do
    test_num=`echo $filename | cut -d'.' -f1`
    dos2unix ${filename}
    ./${OUT_BIN} ${filename} > ${test_num}Yours.out
    diff ${test_num}.out ${test_num}Yours.out
    if [[ $? -ne 0 ]]; then
	    number_of_failed=$(($number_of_failed+1))
	    echo -e "     Failed"
    else 
    	echo -e "     Passed!"
    fi
done

echo Failed $number_of_failed

make clean