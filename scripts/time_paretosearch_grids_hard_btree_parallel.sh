#!/bin/bash
cd ../src/
iter_count=5
max_costs=1000

for p in 16 8 4 2 1 
do
	for q in 0 0.8 -0.8
	do
		out_file="../timings/grid2_paretosearch_p"$p"_btree_q"$q"_hard"
		echo "Writing to "$out_file
		#taskset -c 0-$((p-1))
		./bin/time_grid_instances2_paretosearch_ls_btree.par -p $p -c $iter_count -q $q -m $max_costs > $out_file
	done
done