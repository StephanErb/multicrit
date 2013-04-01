#!/bin/bash
cd ../src/

# Difficult road instance ops: Size of 1 Million. Insert of size about 10000. So ratio is 10

# Test configuration. Change these
iter_count=1000
skew=1
ratio=100 # r = n/k

#################################################################
# BRANCHING PARAMETER (INT)
#################################################################
out_file="../timings/btree/insert_nodewidth_int_inner_p_1_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
	taskset -c 0 ./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_inner_p_8_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
	taskset -c 0-7 ./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_inner_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done

#################################################################
# BRANCHING PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_inner_p_1_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
	taskset -c 0 ./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_inner_p_8_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par > /dev/null
	taskset -c 0-7 ./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_inner_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128 
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done


#################################################################
# LEAF PARAMETER (INT)
#################################################################
out_file="../timings/btree/insert_nodewidth_int_leaf_p_1_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
	taskset -c 0 ./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_leaf_p_8_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
	taskset -c 0-7 ./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_leaf_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done


#################################################################
# LEAF PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_leaf_p_1_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
	taskset -c 0 ./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_leaf_p_8_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
	taskset -c 0-7 ./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_leaf_sequ_r$ratio"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree > /dev/null
	taskset -c 0 ./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done
