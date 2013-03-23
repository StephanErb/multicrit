#!/bin/bash
cd ../src/

make clean

#########################################################
# Pareto search
#########################################################
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_paretosearch
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances1.par > /dev/null
mv ./bin/time_grid_instances1.par ./bin/time_grid_instances1_paretosearch.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_paretosearch
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances2.par > /dev/null
mv ./bin/time_grid_instances2.par ./bin/time_grid_instances2_paretosearch.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_paretosearch
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances1.par > /dev/null
mv ./bin/time_road_instances1.par ./bin/time_road_instances1_paretosearch.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2 > /dev/null
mv ./bin/time_road_instances2 ./bin/time_road_instances2_paretosearch
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2.par > /dev/null
mv ./bin/time_road_instances2.par ./bin/time_road_instances2_paretosearch.par 

#make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_sensor_instances > /dev/null
#mv ./bin/time_sensor_instances ./bin/time_sensor_instances_paretosearch
#make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_sensor_instances.par > /dev/null
#mv ./bin/time_sensor_instances.par ./bin/time_sensor_instances_paretosearch.par


#########################################################
# Classic algorithm
#########################################################

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_LEX" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_lset_lex
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_SUM" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_lset_sum
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_MAX" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_lset_max

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_LEX" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_lset_lex
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_SUM" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_lset_sum
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_MAX" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_lset_max

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_LEX" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_lset_lex
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_SUM" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_lset_sum
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_MAX" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_lset_max

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_LEX" time_road_instances2 > /dev/null
mv ./bin/time_road_instances2 ./bin/time_road_instances2_lset_lex

#make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=SharedHeapLabelSettingAlgorithm -DPRIORITY_LEX" time_sensor_instances > /dev/null
#mv ./bin/time_sensor_instances ./bin/time_sensor_instances_lset_lex


#########################################################
# BTree algorithm
#########################################################

make -B time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_int.par

make -B time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_int

make -B CPPFLAGS="-DUSE_GRAPH_LABEL" time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_label.par

make -B CPPFLAGS="-DUSE_GRAPH_LABEL" time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_label
