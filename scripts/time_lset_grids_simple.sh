#!/bin/bash
cd ../src/
iter_count=12
max_costs=10

for q in 0.8 0 -0.8
do
	echo "Computing Grid Instance "$q
	taskset -c 0 ./bin/time_grid_instances2_lset_lex -c $iter_count -q $q -m $max_costs > "../timings/grid2_lset_lex_q"$q
done
