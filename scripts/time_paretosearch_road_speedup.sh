#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1
s=1

out_file="../timings/road2_paretosearch_p_NY"$s
echo "Writing speedup computation to $out_file"
touch $out_file
rm $out_file # clear
for p in 64 32 28 24 20 16 12 10 8 6 4 2 1 
do
	# taskset -c 0-$((p-1))
	./bin/time_road_instances2_paretosearch_ls_btree.par -s $s -c $iter_count -p $p -g NY -d ../instances/ >> $out_file
done


out_file="../timings/road2_paretosearch_sequ_NY"$s
echo "Writing speedup computation to $out_file"
touch $out_file
rm $out_file # clear
./bin/time_road_instances2_paretosearch_ls_btree -s $s -c $iter_count -g NY -d ../instances/ >> $out_file


out_file="../timings/road2_lset_lex_NY"$s
echo "Writing speedup computation to $out_file"
touch $out_file
rm $out_file # clear
./bin/time_road_instances2_lset_lex -s $s -c $iter_count -g NY -d ../instances/ >> $out_file
