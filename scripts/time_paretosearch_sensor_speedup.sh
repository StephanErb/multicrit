#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=3
n=100000

for d in 10 30 50 
do
	out_file="../timings/sensor_paretosearch_p_d"$d
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for p in 32 28 24 20 16 14 12 10 8 6 4 2 1 
	do
		# taskset -c 0-$((p-1))
		./bin/time_sensor_instances_paretosearch_ls_btree.par -c $iter_count -p $p -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
	done

	out_file="../timings/sensor_paretosearch_sequ_d"$d
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	./bin/time_sensor_instances_paretosearch_ls_btree -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file


	out_file="../timings/sensor_lset_lex_d"$d
	echo "Writing speedup computation to $out_file"
	touch $out_file
	rm $out_file # clear
	./bin/time_sensor_instances_lset_lex -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done