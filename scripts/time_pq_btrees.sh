#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=10
skew=1
# also, see ratio below to configure original tree size: r = n/k


# Bulk Construction
ratio=0
make -B time_pq_btree.par
for p in 1 2 4 8
do
	out_file="../timings/btree/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

make -B time_pq_btree
out_file="../timings/btree/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_btree -c $iter_count -r $ratio -s $skew > $out_file


# Bulk Insertion
make -B time_pq_btree.par
ratio=10 # tree is 10 times larger than the elements we try to insert
for p in 1 2 4 8
do
	out_file="../timings/btree/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

make -B time_pq_btree
out_file="../timings/btree/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_btree -c $iter_count -r $ratio -s $skew  > $out_file


# Bulk Insertion
make -B time_pq_btree.par
ratio=100 # tree is 100 times larger than the elements we try to insert
for p in 1 2 4 8
do
	out_file="../timings/btree/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

make -B time_pq_btree
out_file="../timings/btree/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_btree -c $iter_count -r $ratio -s $skew  > $out_file
