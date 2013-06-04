#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=10
max_cost=10


for q in 0 -0.8
do
	for n in 100 200 300 400 500 600
	do
		out_file="../timings/grid2_paretosearch_p_n"$n"_m"$max_cost"_q"$q
		echo "Writing speedup computation to $out_file"
		touch $out_file
		rm $out_file # clear
		for p in 32 28 24 20 16 14 12 10 8 6 4 2 1  
		do
			# taskset -c 0-$((p-1))
			./bin/time_grid_instances2_paretosearch_ls_btree.par -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
		done

		out_file="../timings/grid2_paretosearch_sequ_n"$n"_m"$max_cost"_q"$q
		echo "Writing speedup computation to $out_file"
		touch $out_file
		rm $out_file # clear
		./bin/time_grid_instances2_paretosearch_ls_btree -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file


		out_file="../timings/grid2_lset_lex_n"$n"_m"$max_cost"_q"$q
		echo "Writing speedup computation to $out_file"
		touch $out_file
		rm $out_file # clear
		./bin/time_grid_instances2_lset_lex -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
	done
done