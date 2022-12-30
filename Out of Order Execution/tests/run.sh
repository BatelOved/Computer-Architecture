#!/bin/bash

# num=1
# var=$( cat ../examples/example${num}_command )
# echo $var
# ./cacheSim "../examples/example${num}_trace" ${var} > 1.txt
# gdb --args ./cacheSim "../examples/example${num}_trace" ${var}

BLACK=`tput setaf 0`
RED=`tput setaf 1`
GREEN=`tput setaf 2`
YELLOW=`tput setaf 3`
BLUE=`tput setaf 4`
MAGENTA=`tput setaf 5`
CYAN=`tput setaf 6`
WHITE=`tput setaf 7`

    echo "${CYAN}This Year Segel Tests: "
for num in {1..2}
do
    var0=$( echo ./examples/opcode1.dat )
    var1=$( echo ./examples/example${num}.in )
    var2=$( cat ./examples/example${num}_cmd )
    ./dflow_calc ${var0} ${var1} ${var2} > ./outputs/my_this_year${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc ${var0} ${var1} ${var2} > ./valgrind_tests/this_year_leak${num}.txt 2>&1
    diff ./outputs/my_this_year${num}.txt ./outputs/this_year${num}.txt
    if cmp ./outputs/my_this_year${num}.txt ./outputs/this_year${num}.txt
        then
    echo "${GREEN}test ${num} passed";
        else
        echo "${RED}test ${num} failed";
    fi
done
echo
echo

    echo "${CYAN}This Year Segel EXTENDED Tests: "
for num in {1..2}
do
    var0=$( echo ./examples/opcode1.dat )
    var1=$( echo ./examples/example${num}.in )
    var2=$( cat ./examples/example${num}_ext )
    ./dflow_calc ${var0} ${var1} ${var2} > ./outputs/my_this_year_ext${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc ${var0} ${var1} ${var2}  > ./valgrind_tests/this_year_ext_leak${num}.txt 2>&1
    diff ./outputs/my_this_year_ext${num}.txt ./outputs/this_year_ext${num}.txt
    if cmp ./outputs/my_this_year_ext${num}.txt ./outputs/this_year_ext${num}.txt
       then
    echo "${GREEN}test ${num} passed";
      else
    echo "${RED}test ${num} failed";
    fi
done

echo
echo
    echo "${CYAN}N&R random Tests: "
for num in {0..99}
do
    var0=$( echo ./inputs/random/opcode${num} )
    var1=$( echo ./inputs/random/test_example${num} )
    var2=$(  cat ./inputs/random/inputscmd)
    #echo $((1+$RANDOM%10)) >> "${var0}"
    ./dflow_calc  ${var0} ${var1} ${var2} > ./outputs/random/my_out${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc  ${var0} ${var1} ${var2} > ./valgrind_tests/NR_leak_random${num}.txt 2>&1
    diff ./outputs/random/my_out${num}.txt ./outputs/random/expected${num}.txt
    if cmp ./outputs/random/my_out${num}.txt ./outputs/random/expected${num}.txt
    then
    echo "${GREEN}test ${num} passed";
    else
        echo "${RED}test ${num} failed";
    fi
done



echo
echo
    echo "${CYAN}NimRosman Tests A lot of Dependencies: "
for num in {0..99}
do
    var0=$( echo ./inputs/one/opcode_one${num} )
    var1=$( echo ./inputs/one/test_example_one${num} )
    var2=$(  cat ./inputs/one/inputscmd_one)
    #echo $((1+$RANDOM%10)) >> "${var0}"
    ./dflow_calc  ${var0} ${var1} ${var2} > ./outputs/one/my_out${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc  ${var0} ${var1} ${var2} > ./valgrind_tests/NR_leak_one${num}.txt 2>&1
    diff ./outputs/one/my_out${num}.txt ./outputs/one/expected${num}.txt
    if cmp ./outputs/one/my_out${num}.txt ./outputs/one/expected${num}.txt
    then
    echo "${GREEN}test ${num} passed";
    else
        echo "${RED}test ${num} failed";
    fi

done

echo
echo
    echo "${CYAN}NimRosman Test No dependencies: "
for num in {0..0}
do
    var0=$( echo ./inputs/two/opcode_two${num} )
    var1=$( echo ./inputs/two/test_example_two${num} )
    var2=$(  cat ./inputs/two/inputscmd_two)
    #echo $((1+$RANDOM%10)) >> "${var0}"
    ./dflow_calc  ${var0} ${var1} ${var2} > ./outputs/two/my_out${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc  ${var0} ${var1} ${var2} > ./valgrind_tests/NR_leak_two${num}.txt 2>&1
    diff ./outputs/two/my_out${num}.txt ./outputs/two/expected${num}.txt
    if cmp ./outputs/two/my_out${num}.txt ./outputs/two/expected${num}.txt
    then
    echo "${GREEN}test ${num} passed";
    else
        echo "${RED}test ${num} failed";
    fi

done



echo
echo
    echo "${CYAN}NimRosman Test Laces: "
for num in {0..4}
do
    var0=$( echo ./inputs/three/opcode_three${num} )
    var1=$( echo ./inputs/three/test_example_three${num} )
    var2=$(  cat ./inputs/three/inputscmd_three)
    #echo $((1+$RANDOM%10)) >> "${var0}"
    ./dflow_calc  ${var0} ${var1} ${var2} > ./outputs/three/my_out${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc  ${var0} ${var1} ${var2} > ./valgrind_tests/NR_leak_three${num}.txt 2>&1
    diff ./outputs/three/my_out${num}.txt ./outputs/three/expected${num}.txt
    if cmp ./outputs/three/my_out${num}.txt ./outputs/three/expected${num}.txt
    then
    echo "${GREEN}test ${num} passed";
    else
        echo "${RED}test ${num} failed";
    fi

done

echo
echo
    echo "${CYAN}N&R Long random Tests: "
for num in {0..29}
do
    var0=$( echo ./inputs/four/opcode${num} )
    var1=$( echo ./inputs/four/test_example${num} )
    var2=$(  cat ./inputs/four/inputscmd)
    #echo $((1+$RANDOM%10)) >> "${var0}"
    ./dflow_calc  ${var0} ${var1} ${var2} > ./outputs/four/my_out${num}.txt
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc  ${var0} ${var1} ${var2} > ./valgrind_tests/NR_leak_four${num}.txt 2>&1
    diff ./outputs/four/my_out${num}.txt ./outputs/four/expected${num}.txt
    if cmp ./outputs/four/my_out${num}.txt ./outputs/four/expected${num}.txt
    then
    echo "${GREEN}test ${num} passed";
    else
        echo "${RED}test ${num} failed";
    fi
done
