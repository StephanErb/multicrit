#!/bin/bash
cd ../src/

iter_count=100

make -B CPPFLAGS="-DUSE_GRAPH_LABEL" time_pq_mcstl_set  > /dev/null
make -B CPPFLAGS="-DUSE_GRAPH_LABEL" time_pq_set  > /dev/null

ratio=0
skew=1
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file


# Bulk Insertion
ratio=0.1 # tree is smaller than the insertion
skew=1
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file

# Skewed Bulk Insertion
ratio=0.1 # tree is smaller than the insertion
skew=0.01
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file



iter_count=10

# Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
skew=1
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file

# Skewed Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
skew=0.01
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file




iter_count=2


# Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=1
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file

# Skewed Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=0.01
for p in 8 1
do
	out_file="../timings/set/insert_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_mcstl_set -c $iter_count -p $p -r $ratio -s $skew > $out_file
done

out_file="../timings/set/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
./bin/time_pq_set -c $iter_count -r $ratio -s $skew > $out_file
