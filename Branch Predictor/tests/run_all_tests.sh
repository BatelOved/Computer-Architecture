#!/bin/bash

#test automation script
INPUT_DIR="tests2"
MY_OUT_DIR_NAME="${INPUT_DIR}_my_out"
OUT_BIN="../../bp_main"


echo -e 'building... '
make clean
make
chmod 777 $OUT_BIN

#remove the directory if it exists, to create a new empty one for the new run.
if [[  -e $MY_OUT_DIR_NAME ]]; then
	rm -rf $mkdir $MY_OUT_DIR_NAME
fi
mkdir $MY_OUT_DIR_NAME

number_of_tests=`ls -l $INPUT_DIR | grep in* | wc -l`
echo -e 'number of tests to run: ' ${number_of_tests}

number_of_failed=0
for filename in tests/test*.trc; do
    test_num=`echo $filename | cut -d'.' -f1`
    dos2unix ${filename}
    ./bp_main $filename > $MY_OUT_DIR_NAME/${test_num}Yours.out
    diff ${test_num}.out $MY_OUT_DIR_NAME/${test_num}Yours.out
    if [[ $? -ne 0 ]]; then
	    number_of_failed=$(($number_of_failed+1))
	    echo -e "     Failed"
    else 
		  echo -e "     Passed!"
	  fi
done

echo Failed $number_of_failed
echo -e 'cleaning directory'
#rm -rf $MY_OUT_DIR_NAME
