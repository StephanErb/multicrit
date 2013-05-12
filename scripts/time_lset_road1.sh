#!/bin/bash
cd ../src/

iter_count=10

out_file="../timings/road1_lset_lex"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
#taskset -c 0 
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
#taskset -c 0 
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file
