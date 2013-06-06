#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=10
max_cost=1000
p=8

for n in 100 200 300
do
	out_file="../timings/grid2_paretosearch_correlation_p"$p"_n"$n"_m"$max_cost
	echo "Writing correlation computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for q in 1 0.8 0.6 0.4 0.2 0 -0.2 -0.4 -0.6 -0.8 -1
	do
		./bin/time_grid_instances2_paretosearch_ls_btree.par -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
	done

	out_file="../timings/grid2_paretosearch_correlation_sequ_n"$n"_m"$max_cost
	echo "Writing correlation computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for q in 1 0.8 0.6 0.4 0.2 0 -0.2 -0.4 -0.6 -0.8 -1
	do
		./bin/time_grid_instances2_paretosearch_ls_btree -n $n -c $iter_count -m $max_cost -q $q >> $out_file
	done

	out_file="../timings/grid2_lset_lex_correlation_n"$n"_m"$max_cost
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for q in 1 0.8 0.6 0.4 0.2 0 -0.2 -0.4 -0.6 -0.8 -1
	do
		./bin/time_grid_instances2_lset_lex -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
	done
done