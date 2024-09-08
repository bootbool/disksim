#!/bin/bash

function rundisksim()
{
    count=10
    while [[ $count -lt 90 ]];
        do
        count=$((count+10))
        ratio=0.$count
        outfile=out-${file}-ratio$ratio
        echo $ratio
        ./disksim Seagate-Cheetah15k5.parv  $outfile  ascii  $file 0 $ratio power.cfg
        mv $outfile power/
    done

    ratio=1.00
    outfile=out-${file}-ratio$ratio
    ./disksim Seagate-Cheetah15k5.parv  $outfile  ascii  $file 0 $ratio power.cfg
    mv $outfile power/
}

for file in `ls -S -L -r  *-trace-[1234567]`
do
    echo $file
    rundisksim
done
