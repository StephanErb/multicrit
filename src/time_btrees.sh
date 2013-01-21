#!/bin/bash

# Test configuration. Change these
iter_count=10
skew=1
ratio=0.1

make time_parallel_btree.par time_btree

# Constants
BULK_CONSTRUCTION=1
BULK_INSERTION=2


# Bulk Construction
for p in 1 2 3 4 
do
	out_file="../timings/btree/constr_p_"$p
	echo "Writing to parallel computation to $out_file"
	./bin/time_parallel_btree.par -c $iter_count -p $p -t $BULK_CONSTRUCTION > $out_file
done

out_file="../timings/btree/constr_sequ"
echo "Writing to sequential computation to $out_file"
./bin/time_btree -c $iter_count -t $BULK_CONSTRUCTION > $out_file


# Bulk Insertion
for p in 1 2 3 4 
do
	out_file="../timings/btree/insert_p_"$p
	echo "Writing to parallel computation to $out_file"
	./bin/time_parallel_btree.par -c $iter_count -p $p -r $ratio -s $skew -t $BULK_INSERTION > $out_file
done

out_file="../timings/btree/insert_sequ"
echo "Writing to sequential computation to $out_file"
./bin/time_btree -c $iter_count -r $ratio -s $skew -t $BULK_INSERTION > $out_file

