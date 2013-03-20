#!/bin/bash
cd ../src/

# Difficult road instance ops: Size of 1 Million. Insert of size about 10000. So ratio is 100

# Test configuration. Change these
iter_count=30
skew=1
ratio=10 # r = n/k

default_inner_node_width=16
default_leaf_node_width=128

# Best Paralle Node Size
out_file="../timings/btree/insert_p_nodewidth_int_inner"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for num_cachelines in 4 8 12 14 16 18 20 24 28 32
do
	make -B CPPFLAGS="-DINNER_NODE_WIDTH=$num_cachelines -DLEAF_NODE_WIDTH=$default_leaf_node_width" time_pq_btree.par
	echo -n "$num_cachelines " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_p_nodewidth_int_leaf"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for num_cachelines in 4 8 16 32 64 96 128 192 256 512 1024
do
	make -B CPPFLAGS="-DINNER_NODE_WIDTH=$default_inner_node_width -DLEAF_NODE_WIDTH=$num_cachelines" time_pq_btree.par
	echo -n "$num_cachelines " >> $out_file
	./bin/time_pq_btree.par -c $iter_count -p 8 -r $ratio -s $skew -k 10000 >> $out_file
done

# Best Sequential Node Size
out_file="../timings/btree/insert_sequ_nodewidth_int_inner"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for num_cachelines in 4 8 12 14 16 18 20 24 28 32
do
	make -B CPPFLAGS="-DINNER_NODE_WIDTH=$num_cachelines -DLEAF_NODE_WIDTH=$default_leaf_node_width" time_pq_btree
	echo -n "$num_cachelines " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done

out_file="../timings/btree/insert_sequ_nodewidth_int_leaf"
echo "Writing node size computation to $out_file"
touch $out_file
rm $out_file # clear
for num_cachelines in 4 8 16 32 64 96 128 192 256 512 1024
do
	make -B CPPFLAGS="-DINNER_NODE_WIDTH=$default_inner_node_width -DLEAF_NODE_WIDTH=$num_cachelines" time_pq_btree
	echo -n "$num_cachelines " >> $out_file
	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
done


## Now for Labels (x,y,node) instead of just plain ints

# Best Paralle Node Size
#out_file="../timings/btree/insert_p_nodewidth_label"
#echo "Writing node size computation to $out_file"
#touch $out_file
#rm $out_file # clear
#for num_cachelines in 1 2 3 4 5 6 7 8 9 10 11 12 14 16 18 20 32 64 128 256 512 1024
#do
#	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DINNER_NODE_WIDTH=$num_cachelines -DLEAF_NODE_WIDTH=$num_cachelines" time_pq_btree.par
#	echo -n "$num_cachelines " >> $out_file
#	./bin/time_pq_btree.par -c $iter_count -p 4 -r $ratio -s $skew -k 10000 >> $out_file
#done

# Best Sequential Node Size
#out_file="../timings/btree/insert_sequ_nodewidth_label"
#echo "Writing node size computation to $out_file"
#touch $out_file
#rm $out_file # clear
#for num_cachelines in 1 2 3 4 5 6 7 8 9 10 11 12 14 16 18 20 32 64 128 256 512 1024
#do
#	make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DINNER_NODE_WIDTH=$num_cachelines -DLEAF_NODE_WIDTH=$num_cachelines" time_pq_btree
#	echo -n "$num_cachelines " >> $out_file
#	./bin/time_pq_btree -c $iter_count -r $ratio -s $skew -k 10000 >> $out_file
#done