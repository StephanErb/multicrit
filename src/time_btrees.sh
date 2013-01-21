#!/bin/bash

# Test configuration. Change these
iter_count=10
skew=1
# also, see ratio below to configure original tree size: r = n/k

make -B time_parallel_btree.par time_btree



# Bulk Construction
ratio=0
for p in 1 2 3 4 
do
	out_file="../timings/btree/constr_p_"$p
	echo "Writing to parallel computation to $out_file"
	./bin/time_parallel_btree.par -c $iter_count -p $p -r $ratio -s $skew > $out_file
done
out_file="../timings/btree/constr_sequ"
echo "Writing to sequential computation to $out_file"
./bin/time_btree -c $iter_count -r $ratio -s $skew > $out_file


# Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
for p in 1 2 3 4 
do
	out_file="../timings/btree/insert_p_"$p
	echo "Writing to parallel computation to $out_file"
	./bin/time_parallel_btree.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done
out_file="../timings/btree/insert_sequ"
echo "Writing to sequential computation to $out_file"
./bin/time_btree -c $iter_count -r $ratio -s $skew  > $out_file


