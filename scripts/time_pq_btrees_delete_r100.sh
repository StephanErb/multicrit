#!/bin/bash
cd ../src/

ratio=100
iter_count=2


##################
# Plain delete
##################

# Bulk deletion
skew=1
for p in 1 2 4 8 16
do
	out_file="../timings/btree/delete_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -c $iter_count -r $ratio -s $skew  > $out_file


# Skewed Bulk deletion
skew=0.01
for p in 1 2 4 8 16
do
	out_file="../timings/btree/delete_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -c $iter_count -r $ratio -s $skew  > $out_file


##################
# Delete + Combined insert
##################

# Bulk deletion
skew=1
for p in 8
do
	out_file="../timings/btree/delete_combined_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -y -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_combined_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -y -c $iter_count -r $ratio -s $skew  > $out_file


# Skewed Bulk deletion
skew=0.01
for p in 8
do
	out_file="../timings/btree/delete_combined_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -y -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_combined_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -y -c $iter_count -r $ratio -s $skew  > $out_file

##################
# Delete + separated insert
##################

# Bulk deletion
skew=1
for p in 8
do
	out_file="../timings/btree/delete_separated_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -x -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_separated_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -x -c $iter_count -r $ratio -s $skew  > $out_file


# Skewed Bulk deletion
skew=0.01
for p in 8
do
	out_file="../timings/btree/delete_separated_p_$p""_r$ratio""_s$skew"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1)) 
	./bin/time_pq_btree_delete_label_large.par -x -c $iter_count -p $p -r $ratio -s $skew  > $out_file
done

out_file="../timings/btree/delete_separated_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
#taskset -c 0 
./bin/time_pq_btree_delete_label_large -x -c $iter_count -r $ratio -s $skew  > $out_file
