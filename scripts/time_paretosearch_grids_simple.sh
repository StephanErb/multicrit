#!/bin/bash
cd ../src/
p=8
iter_count=12
max_costs=10


for q in 0.8 0.4 0 -0.4 -0.8
do
	echo "Sequential - Computing Grid Instance "$q
	taskset -c 0 ./bin/time_grid_instances2_paretosearch_ls_vec -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_vec_q"$q
	taskset -c 0 ./bin/time_grid_instances2_paretosearch_ls_btree -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_btree_q"$q

	echo "Parallel - Computing Grid Instance "$q
	taskset -c 0-$((p-1)) ./bin/time_grid_instances2_paretosearch_ls_vec.par -p $p -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_p"$p"_vec_q"$q
	taskset -c 0-$((p-1)) ./bin/time_grid_instances2_paretosearch_ls_btree.par -p $p -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_p"$p"_btree_q"$q
done
