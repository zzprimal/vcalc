#!/bin/bash
# just quick bash script to create the fuzzer files and sorting them
# assumes that the fuzzer directory was put in the base of the repository
#exit 0

if [ -z $1 ] ; then
    echo "Error: amount of tests not set as a positional argument"
    exit 1
fi

if [ ! -d "./testfiles/fuzzertests" ] ; then
    echo "./testfiles/fuzzertests DNE, creating it"
    mkdir ./testfiles/fuzzertests
fi

for ((i=1;i<=$1;i++));
do
    python3.8 ../dist/fuzzer.py ../dist/config.json ./testfiles/fuzzertests/test$i > /dev/null 2>&1
    # ../dist/venv/bin/python ../dist/fuzzer.py ../dist/config.json ./testfiles/fuzzertests/test$i > /dev/null 2>&1
done
