#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2.par
for p in 1 2 4 8
do
	out_file="../timings/road2_ParallelParetoSearch_ParallelBTreeParetoQueue_p_$p"
	echo "Writing to parallel computation to $out_file"
	./bin/time_road_instances2.par -d ../instances/ -g NY -c $iter_count -p $p > $out_file
done

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2
out_file="../timings/road2_SequentialParetoSearch_VectorParetoQueue"
echo "Writing to sequential computation to $out_file"
./bin/time_road_instances2 -d ../instances/ -g NY -c $iter_count > $out_file

# The original, sequential algorithm
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm" time_road_instances2
out_file="../timings/road2_SequentialSharedHeapLabelSettingAlgorithm_VectorSet"
echo "Writing to sequential computation to $out_file"
./bin/time_road_instances2 -d ../instances/ -g NY -c $iter_count > $out_file
