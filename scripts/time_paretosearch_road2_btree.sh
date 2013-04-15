#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1

for p in 8
do
	out_file="../timings/road2_paretosearch_p"$p"_btree"
	echo "Writing to parallel computation to $out_file"
	taskset -c 0-$((p-1)) ./bin/time_road_instances2_paretosearch_ls_btree.par -d ../instances/ -g NY -c $iter_count -p $p > $out_file
done

out_file="../timings/road2_paretosearch_sequ_btree"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_road_instances2_paretosearch_ls_btree -d ../instances/ -g NY -c $iter_count > $out_file
