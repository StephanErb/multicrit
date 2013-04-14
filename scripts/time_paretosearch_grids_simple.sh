#!/bin/bash
cd ../src/
p=8
iter_count=12
max_costs=10


for q in 0.8 0.4 0 -0.4 -0.8
do
	echo "Sequential - Computing Grid Instance "$q
	taskset -c 0 ./bin/time_grid_instances2_paretosearch -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_q"$q

	echo "Parallel - Computing Grid Instance "$q
	taskset -c 0-$((p-1)) ./bin/time_grid_instances2_paretosearch.par -p $p -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_p"$p"_q"$q
done
