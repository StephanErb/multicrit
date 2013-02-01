#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1
skew=1
# also, see ratio below to configure original tree size: r = n/k

make -B time_pq_mcstl_set
make -B time_pq_set

# Bulk Construction
ratio=0
for p in 1 2 4 8
do
	out_file="../timings/set/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file


# Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
for p in 1 2 4 8
do
	out_file="../timings/set/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file


# Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
for p in 1 2 4 8
do
	out_file="../timings/set/insert_p_$p""_r$ratio"
	echo "Writing to parallel computation to $out_file"
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file