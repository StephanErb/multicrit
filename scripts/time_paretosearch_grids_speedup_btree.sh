#!/bin/bash
cd ../src/
iter_count=1
max_costs=1000
q=0

for p in 16 4 2 1 # HACK: 8 computed by the other script
do
	echo "Parallel - Computing Grid Instance "$q" "$p
	taskset -c 0-$((p-1)) ./bin/time_grid_instances2_paretosearch_ls_btree.par -v -p $p -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_p"$p"_btree_q"$q"_hard"
done

# HACK: sequ computed by other script
#echo "Sequential - Computing Grid Instance "$q
#taskset -c 0 ./bin/time_grid_instances2_paretosearch_ls_btree -v -c $iter_count -q $q -m $max_costs > "../timings/grid2_paretosearch_sequ_btree_q"$q"_hard"