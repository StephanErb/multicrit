#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1000
skew=1
numelements=10000

#################################################################
# BRANCHING PARAMETER (LABEL)
#################################################################
out_file="../timings/btree/insert_nodewidth_label_leaf_sequ_heatmap"
echo "Writing node size heatmap to $out_file"
touch $out_file
rm $out_file # clear
for r in 0 1 3 10 31 100 316 1000 3162 10000
do
	if [ $r -gt 300 ]
	then
		iter_count=100
	fi
	if [ $r -gt 999 ]
	then
		iter_count=10
	fi

	row=()
	sum=0.0
	for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree > /dev/null
		# taskset -c 0 
		current=$(./bin/time_pq_btree -c $iter_count -r $r -s $skew -k $numelements -h)
		row+=($current)
		sum=$(echo $sum + $current | bc)
	done
	echo "$r -> row average $sum"
	for i in "${row[@]}"
	do
		echo "scale=4; $i / $sum" | bc | tr "\n" " " >> $out_file 
		echo -n " " >> $out_file
	done
	echo "" >> $out_file
done

iter_count=4000
out_file="../timings/btree/insert_nodewidth_label_leaf_p_8_heatmap"
echo "Writing node size heatmap to $out_file"
touch $out_file
rm $out_file # clear
for r in 0 1 3 10 31 100 316 1000 3162 10000
do
	if [ $r -gt 300 ]
	then
		iter_count=100
	fi
	if [ $r -gt 999 ]
	then
		iter_count=10
	fi

	row=()
	sum=0.0
	for k in 8 16 32 64 128 256 512 1024 2048 4096 8192 16384
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
		#taskset -c 0-7 
		current=$(./bin/time_pq_btree.par -c $iter_count -r $r -p 8 -s $skew -k $numelements -h)
		row+=($current)
		sum=$(echo $sum + $current | bc)
	done
	echo "$r -> row average $sum"
	for i in "${row[@]}"
	do
		echo "scale=4; $i / $sum" | bc | tr "\n" " " >> $out_file 
		echo -n " " >> $out_file
	done
	echo "" >> $out_file
done


#################################################################
# BRANCHING PARAMETER (LABEL) - detailed
#################################################################
iter_count=1000
out_file="../timings/btree/insert_nodewidth_label_leaf_sequ_heatmap_detailed"
echo "Writing node size heatmap to $out_file"
touch $out_file
rm $out_file # clear
for r in 0 1 3 10 31 100 316 1000 3162 10000
do
	if [ $r -gt 300 ]
	then
		iter_count=100
	fi
	if [ $r -gt 999 ]
	then
		iter_count=10
	fi

	row=()
	sum=0.0
	for k in `seq 64 64 2048`
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree > /dev/null
		# taskset -c 0 
		current=$(./bin/time_pq_btree -c $iter_count -r $r -s $skew -k $numelements -h)
		row+=($current)
		sum=$(echo $sum + $current | bc)
	done
	echo "$r -> row average $sum"
	for i in "${row[@]}"
	do
		echo "scale=4; $i / $sum" | bc | tr "\n" " " >> $out_file 
		echo -n " " >> $out_file
	done
	echo "" >> $out_file
done

iter_count=4000
out_file="../timings/btree/insert_nodewidth_label_leaf_p_8_heatmap_detailed"
echo "Writing node size heatmap to $out_file"
touch $out_file
rm $out_file # clear
for r in 0 1 3 10 31 100 316 1000 3162 10000
do
	if [ $r -gt 300 ]
	then
		iter_count=100
	fi
	if [ $r -gt 999 ]
	then
		iter_count=10
	fi
	
	row=()
	sum=0.0
	for k in `seq 64 64 2048`
	do
		make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DLEAF_PARAMETER_K=$k" time_pq_btree.par > /dev/null
		# taskset -c 0-7 
		current=$(./bin/time_pq_btree.par -c $iter_count -r $r -p 8 -s $skew -k $numelements -h)
		row+=($current)
		sum=$(echo $sum + $current | bc)
	done
	echo "$r -> row average $sum"
	for i in "${row[@]}"
	do
		echo "scale=4; $i / $sum" | bc | tr "\n" " " >> $out_file 
		echo -n " " >> $out_file
	done
	echo "" >> $out_file
done
