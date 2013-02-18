#!/bin/bash
cd ../src/

# Test configuration. Change these
iter_count=1

# The original, sequential algorithm
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm" time_road_instances2
out_file="../timings/road2_SequentialSharedHeapLabelSettingAlgorithm_VectorSet"
echo "Writing to sequential computation to $out_file"
./bin/time_road_instances2 -d ../instances/ -g NY -c $iter_count > $out_file
