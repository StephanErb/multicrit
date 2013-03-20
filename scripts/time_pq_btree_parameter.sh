#!/bin/bash
cd ../src/

# Difficult road instance ops: Size of 1 Million. Insert of size about 10000. So ratio is 10

# Test configuration. Change these
iter_count=1000
skew=1
ratio=10 # r = n/k

#################################################################
# BRANCHING PARAMETER (INT)
#################################################################
out_file="../timings/btree/insert_nodewidth_int_inner_p_1"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree.par
	echo -n "$b " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_inner_p_8"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree.par
	echo -n "$b " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_inner_sequ"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b" time_pq_btree
	echo -n "$b " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 25000 >> $out_file
done

#################################################################
# BRANCHING PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_inner_p_1"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par
	echo -n "$b " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_inner_p_8"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree.par
	echo -n "$b " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_inner_sequ"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for b in 8 12 16 20 24 28 32 48 64 128
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b" time_pq_btree
	echo -n "$b " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 25000 >> $out_file
done


#################################################################
# LEAF PARAMETER (INT)
#################################################################
out_file="../timings/btree/insert_nodewidth_int_leaf_p_1"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree.par
	echo -n "$k " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_leaf_p_8"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree.par
	echo -n "$k " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_int_leaf_sequ"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DLEAF_PARAMETER_K=$k" time_pq_btree
	echo -n "$k " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 25000 >> $out_file
done


#################################################################
# LEAF PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_leaf_p_1"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -LEAF_PARAMETER_K=$k" time_pq_btree.par
	echo -n "$k " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 1 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_leaf_p_8"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -LEAF_PARAMETER_K=$k" time_pq_btree.par
	echo -n "$k " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 25000 >> $out_file
done

out_file="../timings/btree/insert_nodewidth_label_leaf_sequ"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for k in 32 64 128 256 512 1024 2048 4096 8192 16384
do
	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -LEAF_PARAMETER_K=$k" time_pq_btree
	echo -n "$k " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 25000 >> $out_file
done