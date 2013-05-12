#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=3
n=300
q=0

max_cost=10
for p in 16 8 4 
do
	out_file="../timings/batch_size_grid2_paretosearch_p_$p""_q"$q
	echo "Writing batch size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for b in 4 8 16 32 64 128 256 512 1024 2048 4096
	do
		make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET -DBATCH_SIZE=$b" time_grid_instances2.par > /dev/null
		#taskset -c 0-$((p-1)) 
		./bin/time_grid_instances2.par -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
	done
done

max_cost=1000
for p in 16 8 4 
do
	out_file="../timings/batch_size_grid2_paretosearch_p_$p""_q"$q"_hard"
	echo "Writing batch size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for b in 4 8 16 32 64 128 256 512 1024 2048 4096
	do
		make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET -DBATCH_SIZE=$b" time_grid_instances2.par > /dev/null
		#taskset -c 0-$((p-1))
		./bin/time_grid_instances2.par -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
	done
done

