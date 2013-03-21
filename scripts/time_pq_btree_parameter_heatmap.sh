#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=100
skew=1
numelements=10000

#################################################################
# BRANCHING PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_leaf_p_1_heatmap"
echo "Writing node size heatmap to $out_file"
touch $out_file
rm $out_file # clear
for r in 0 10 100 1000 10000
do
	for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree.par
		./bin/time_pq_btree.par -c $iter_count -p 1 -r $r -s $skew -k $numelements -h >> $out_file
		echo -n " " >> $out_file
	done
	echo "" >> $out_file
done