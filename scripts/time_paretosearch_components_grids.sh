#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=3
max_cost=1000

for q in 0.8 0
do
	for n in 400
	do
		out_file="../timings/paretosearch_components_p_n"$n"_m"$max_cost"_q"$q
		echo "Writing speedup computation to $out_file"
		touch $out_file
		rm $out_file # clear
		for p in 1 2 4 6 8 10 12 14 16 20 24 28 32 
		do
			# taskset -c 0-$((p-1))
			./bin/time_grid_instances2_paretosearch_ls_btree_with_subtime.par -s -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
		done

		out_file="../timings/paretosearch_components_sequ_n"$n"_m"$max_cost"_q"$q
		echo "Writing speedup computation to $out_file"
		touch $out_file
		rm $out_file # clear
		./bin/time_grid_instances2_paretosearch_ls_btree_with_subtime -s -n $n -c $iter_count -m $max_cost -q $q >> $out_file
	done
done
