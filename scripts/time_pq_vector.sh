#!/bin/bash
cd ../src/
mkdir -p ../timings/vec/

# Bulk Construction
iter_count=10
ratio=0
skew=1
out_file="../timings/vec/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_vector_label_scan -c $iter_count -r $ratio -s $skew > $out_file


# For all insertions use
iter_count=1

# Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
skew=1
out_file="../timings/vec/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_vector_label_scan -c $iter_count -r $ratio -s $skew > $out_file

# Skewed Bulk Insertion
ratio=10 # tree is 10 times larger than the elements we try to insert
skew=0.01
out_file="../timings/vec/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_vector_label_scan -c $iter_count -r $ratio -s $skew > $out_file

# Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=1
out_file="../timings/vec/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_vector_label_scan -c $iter_count -r $ratio -s $skew > $out_file

# Skewed Bulk Insertion
ratio=100 # tree is 100 times larger than the elements we try to insert
skew=0.01
out_file="../timings/vec/insert_sequ_r$ratio""_s$skew"
echo "Writing to sequential computation to $out_file"
taskset -c 0 ./bin/time_pq_vector_label_scan -c $iter_count -r $ratio -s $skew > $out_file
