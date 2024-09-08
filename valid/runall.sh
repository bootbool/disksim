#!/bin/bash

function rundisksim()
{
    count=10
    while [[ $count -le 90 ]];
        do
        count=$((count+5))
        ratio=0.$count
        echo $ratio
        disksim-comp/disksim$ratio  Seagate-Cheetah15k5.parvr${read}loc${local}  out.seagater${read}loc${local}ratio$ratio  ascii  0 1
        mv out.seagater${read}loc${local}ratio$ratio r${read}loc${local}/
    done
    disksim-comp/disksim1.00  Seagate-Cheetah15k5.parvr${read}loc${local}  out.seagater${read}loc${local}ratio1.00  ascii  0 1
    mv out.seagater${read}loc${local}ratio1.00 r${read}loc${local}/
}

read=0.1
local=0.1
for r in 9 5 1
do
    read=0.${r}
    for l in 8 5 1
    do
        local=0.${l}
        echo read $read   local $local
        mkdir r${read}loc${local}
        rundisksim
    done
done
