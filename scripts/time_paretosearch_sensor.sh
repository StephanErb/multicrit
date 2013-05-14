#!/bin/bash
cd ../src/
p=8
iter_count=1
n=1000


out_file="../timings/sensor_paretosearch_sequ_btree_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 10 20 30
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_btree -v -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_p"$p"_btree_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 10 20 30
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_btree.par -v -p $p -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_sequ_vec_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 10 20 30
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_vec -v -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_p"$p"_vec_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 10 20 30
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_vec.par -v -p $p -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done
