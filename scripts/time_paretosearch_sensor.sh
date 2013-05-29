#!/bin/bash
cd ../src/
p=8
iter_count=10
n=100000

out_file="../timings/sensor_paretosearch_sequ_btree_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 5 10 15 20 25 30 35 40 45 50
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_btree -v -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_p"$p"_btree_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 5 10 15 20 25 30 35 40 45 50
do
	#taskset -c 0
	./bin/time_sensor_instances_paretosearch_ls_btree.par -p $p -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_sequ_vec_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 5 10 15 20 25 30 35 40 45 50
do
	#taskset -c 0
	#./bin/time_sensor_instances_paretosearch_ls_vec -v -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done


out_file="../timings/sensor_paretosearch_p"$p"_vec_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 5 10 15 20 25 30 35 40 45 50
do
	#taskset -c 0
	#./bin/time_sensor_instances_paretosearch_ls_vec.par -p $p -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done
