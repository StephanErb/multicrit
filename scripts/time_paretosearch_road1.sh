#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=30

for p in 8
do
	out_file="../timings/road1_paretosearch_p$p"
	echo "Writing to parallel computation to $out_file"
	taskset -c 0-$((p-1)) ./bin/time_road_instances1_paretosearch.par -c $iter_count -d ../instances/ -g DC -n 1  -p $p > $out_file
	taskset -c 0-$((p-1)) ./bin/time_road_instances1_paretosearch.par -c $iter_count -d ../instances/ -g RI -n 10 -p $p >> $out_file
	taskset -c 0-$((p-1)) ./bin/time_road_instances1_paretosearch.par -c $iter_count -d ../instances/ -g NJ -n 19 -p $p >> $out_file
done

out_file="../timings/road1_paretosearch_sequ"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_road_instances1_paretosearch -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
taskset -c 0 ./bin/time_road_instances1_paretosearch -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
taskset -c 0 ./bin/time_road_instances1_paretosearch -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file