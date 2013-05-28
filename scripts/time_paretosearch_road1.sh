#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=10

for p in 8
do
	out_file="../timings/road1_paretosearch_p"$p"_vec"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_vec.par -c $iter_count -d ../instances/ -g DC -n 1  -p $p > $out_file
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_vec.par -c $iter_count -d ../instances/ -g RI -n 10 -p $p >> $out_file
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_vec.par -c $iter_count -d ../instances/ -g NJ -n 19 -p $p >> $out_file

	out_file="../timings/road1_paretosearch_p"$p"_btree"
	echo "Writing to parallel computation to $out_file"
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_btree.par -c $iter_count -d ../instances/ -g DC -n 1  -p $p > $out_file
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_btree.par -c $iter_count -d ../instances/ -g RI -n 10 -p $p >> $out_file
	#taskset -c 0-$((p-1))
	./bin/time_road_instances1_paretosearch_ls_btree.par -c $iter_count -d ../instances/ -g NJ -n 19 -p $p >> $out_file
done

out_file="../timings/road1_paretosearch_sequ_vec"
echo "Writing to sequential computation to $out_file"
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_vec -v -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_vec -v -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_vec -v -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file

out_file="../timings/road1_paretosearch_sequ_btree"
echo "Writing to sequential computation to $out_file"
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_btree -v -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_btree -v -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
#taskset -c 0
./bin/time_road_instances1_paretosearch_ls_btree -v -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file
