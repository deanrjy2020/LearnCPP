#!/bin/bash

# run single test and append result
run_single() {
    TEST_NAME=$1

    SOURCE="tests/${TEST_NAME}.cpp"

    RESULT=$(${G_EXE} ${TEST_NAME})
    EXIT_CODE=$?
    echo "${RESULT}"

    if [ $EXIT_CODE -ne 0 ]; then
        echo "Error, program failed with return code: $EXIT_CODE"
        exit 1
    fi

    # Remove old result block
    sed -i '/\/\*===== Output =====/,/\*\//d' "${SOURCE}"

    # Append new result block
    {
        echo "/*===== Output ====="
        echo "$RESULT"
        echo -e "\n*/"
    } >> "${SOURCE}"

    # remove the FXXX CRLF, and only use LF, also the .gitattributes is needed.
    sed -i 's/\r$//' "${SOURCE}"

    #echo "Result appended to ${SOURCE}"
}

do_run() {
    echo "Runing log:"
    if [ "${G_RUN_TEST}" == "all" ]; then
        for T in ${G_TESTS_LIST}; do
            run_single ${T}
        done
    else
        run_single ${G_RUN_TEST}
    fi
}
#=====================================================
# global variables

# run all the tests by default
G_RUN_TEST=all
if [ "$#" == "1" ]; then
    G_RUN_TEST="${1}"
fi
#echo $G_RUN_TEST

G_TESTS_LIST=$(cat "tests_list.src")
#echo "G_TESTS_LIST = $G_TESTS_LIST"

G_EXE="out/program.exe"

#=====================================================
# do the jobs

# run all tests or single test based on the user input.
do_run
