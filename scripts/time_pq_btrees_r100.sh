#!/bin/bash
cd ../src/

# Bulk Construction
iter_count=1
ratio=0
skew=1

# Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=1
for p in 1 2 4 8 16
do
	out_file="../timings/btree/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	taskset -c 0-$((p-1)) ./bin/time_pq_btree_label.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_btree_label -c $iter_count -r $ratio -s $skew  > $out_file


# Skewed Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=0.01
for p in 1 2 4 8 16
do
	out_file="../timings/btree/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	taskset -c 0-$((p-1)) ./bin/time_pq_btree_label.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_btree_label -c $iter_count -r $ratio -s $skew  > $out_file
