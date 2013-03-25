#!/bin/bash
cd ../src/

iter_count=1

out_file="../timings/road2_lset_lex"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_road_instances2_lset_lex -d ../instances/ -g NY -c $iter_count > $out_file
