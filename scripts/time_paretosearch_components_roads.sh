#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=3

for roadinstance in 2 3 
do
	out_file="../timings/paretosearch_components_p_NY"$roadinstance
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for p in 1 2 4 6 8 10 12 14 16 20 24 28 32 
	do
		# taskset -c 0-$((p-1))
		./bin/time_road_instances2_paretosearch_ls_btree_with_subtime.par -s -r $roadinstance -c $iter_count -p $p -g NY -d ../instances/ >> $out_file
	done

	out_file="../timings/paretosearch_components_sequ_NY"$roadinstance
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	./bin/time_road_instances2_paretosearch_ls_btree_with_subtime -s -r $roadinstance -c $iter_count -g NY -d ../instances/ >> $out_file
done
