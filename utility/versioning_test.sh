#!/bin/bash

`touch file_01`

for i in {1..$1}
do
    `echo "Version $i" >> file_01`
done
