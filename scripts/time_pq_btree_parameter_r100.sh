#!/bin/bash
cd ../src/

# Difficult road instance ops: Size of 1 Million. Insert of size about 10000. So ratio is 10

# Test configuration. Change these
iter_count=1000
skew=1
ratio=100 # r = n/k
numelements=10000

fixed_k=64
fixed_b=16

#################################################################
# BRANCHING PARAMETER (INT)
#################################################################

for p in 1 2 4 8
do
	out_file="../timings/btree/insert_nodewidth_int_inner_p_$p""_r$ratio"
#	echo "Writing node size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for b in 8 12 16 20 24 28 32 48 64 128
	do
		make -B CPPFLAGS="-DLEAF_PARAMETER_K=$fixed_k -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
#		taskset -c 0-$((p-1)) ./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew -k $numelements >> $out_file
	done
done

out_file="../timings/btree/insert_nodewidth_int_inner_sequ_r$ratio"
#echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$fixed_k -DBRANCHING_PARAMETER_B=$b" time_pq_btree > /dev/null
#	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k $numelements >> $out_file
done

#################################################################
# BRANCHING PARAMETER (LABEL)
#################################################################

for p in 1 2 4 8
do
	out_file="../timings/btree/insert_nodewidth_label_inner_p_$p""_r$ratio"
	echo "Writing node size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for b in 8 12 16 20 24 28 32 48 64 128
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$fixed_k -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
		taskset -c 0-$((p-1)) ./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew -k $numelements >> $out_file
	done
done

out_file="../timings/btree/insert_nodewidth_label_inner_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128 
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$fixed_k -DBRANCHING_PARAMETER_B=$b" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k $numelements >> $out_file
done


#################################################################
# LEAF PARAMETER (INT)
#################################################################

for p in 1 2 4 8
do
	out_file="../timings/btree/insert_nodewidth_int_leaf_p_$p""_r$ratio"
#	echo "Writing node size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
	do
		make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k -DBRANCHING_PARAMETER_B=$fixed_b" time_pq_btree.par > /dev/null
#		taskset -c 0-$((p-1)) ./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew -k $numelements >> $out_file
	done
done

out_file="../timings/btree/insert_nodewidth_int_leaf_sequ_r$ratio"
#echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k -DBRANCHING_PARAMETER_B=$fixed_b" time_pq_btree > /dev/null
#	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k $numelements >> $out_file
done


#################################################################
# LEAF PARAMETER (LABEL)
#################################################################

for p in 1 2 4 8
do
	out_file="../timings/btree/insert_nodewidth_label_leaf_p_$p""_r$ratio"
	echo "Writing node size computation to $out_file"
	touch $out_file
	rm $out_file # clear
	for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k -DBRANCHING_PARAMETER_B=$fixed_b" time_pq_btree.par > /dev/null
		taskset -c 0-$((p-1)) ./bin/time_pq_btree.par -c $iter_count -p $p -r $ratio -s $skew -k $numelements >> $out_file
	done
done

out_file="../timings/btree/insert_nodewidth_label_leaf_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k -DBRANCHING_PARAMETER_B=$fixed_b" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k $numelements >> $out_file
done
