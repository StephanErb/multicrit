#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1
n=300
q=0
max_cost=1000

out_file="../timings/grid2_paretosearch_p_n"$n"_m"$m"_q"$q
echo "Writing batch size computation to $out_file"
touch $out_file
rm $out_file # clear
for p in 32 24 20 16 12 8 4 2 1 
do
	taskset -c 0-$((p-1)) ./bin/time_grid_instances2_paretosearch_ls_btree.par -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
done


out_file="../timings/grid2_paretosearch_sequ_n"$n"_m"$m"_q"$q
echo "Writing batch size computation to $out_file"
touch $out_file
rm $out_file # clear
taskset -c 0 ./bin/time_grid_instances2_paretosearch_ls_btree -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file


out_file="../timings/grid2_lset_lex_n"$n"_m"$m"_q"$q
echo "Writing batch size computation to $out_file"
touch $out_file
rm $out_file # clear
taskset -c 0 ./bin/time_grid_instances2_lset_lex -n $n -c $iter_count -p $p -m $max_cost -q $q >> $out_file
