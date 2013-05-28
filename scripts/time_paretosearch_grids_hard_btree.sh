#!/bin/bash
cd ../src/
iter_count=5
max_costs=1000

for q in 0 0.8 -0.8
do
	echo "Sequential - Computing Grid Instance "$q
	#taskset -c 0
	./bin/time_grid_instances2_paretosearch_ls_btree -v -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_btree_q"$q"_hard"
done

