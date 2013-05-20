#!/bin/bash
cd ../src/

# Bulk Construction
iter_count=10
ratio=0
skew=1
for p in 1 2 4 8 16
do
	out_file="../timings/btree/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1))
	./bin/time_pq_btree_label_large.par -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/btree/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_label_large -c $iter_count -r $ratio -s $skew > $out_file
