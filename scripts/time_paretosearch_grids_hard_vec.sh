#!/bin/bash
cd ../src/
p=8
iter_count=10
max_costs=1000

for q in 0
do
	echo "Sequential - Computing Grid Instance "$q
	#taskset -c 0 
	./bin/time_grid_instances2_paretosearch_ls_vec -v -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_vec_q"$q"_hard"

	echo "Parallel - Computing Grid Instance "$q
	#taskset -c 0-$((p-1))
	./bin/time_grid_instances2_paretosearch_ls_vec.par -p $p -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_p"$p"_vec_q"$q"_hard"
done

