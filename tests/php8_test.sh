#!/bin/bash

cd $(dirname $0)
cd ..

echo -ne "\e[2m"
./configure && make || { echo -e "\e[0;1mCompile failure" && exit 1; }
echo -ne "\e[0m"

cd tests

../dia2code/dia2code -t php8 php8_test.dia

for phpFile in *.php
do
    diff $phpFile php8_expectedOutput/$phpFile \
    || echo -e "\e[31;2mOutput file ${phpFile} differs from expected output.\e[0m"
done
