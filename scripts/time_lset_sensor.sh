#!/bin/bash
cd ../src/

iter_count=10
n=100000

out_file="../timings/sensor_lset_lex_n"$n
echo $out_file
touch $out_file
rm $out_file
for d in 5 10 15 20 25 30 35 40 45 50
do
	#taskset -c 0
	./bin/time_sensor_instances_lset_lex -v -c $iter_count -d ../instances/sensor/ -g "n"$n"_d"$d >> $out_file
done
