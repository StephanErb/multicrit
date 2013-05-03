#!/bin/bash
cd ../src/

make clean

#########################################################
# Pareto search
#########################################################
echo "Pareto Search..."

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_paretosearch_ls_vec
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances1.par > /dev/null
mv ./bin/time_grid_instances1.par ./bin/time_grid_instances1_paretosearch_ls_vec.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_paretosearch_ls_vec
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_grid_instances2.par > /dev/null
mv ./bin/time_grid_instances2.par ./bin/time_grid_instances2_paretosearch_ls_vec.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_paretosearch_ls_vec
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances1.par > /dev/null
mv ./bin/time_road_instances1.par ./bin/time_road_instances1_paretosearch_ls_vec.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2 > /dev/null
mv ./bin/time_road_instances2 ./bin/time_road_instances2_paretosearch_ls_vec
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_road_instances2.par > /dev/null
mv ./bin/time_road_instances2.par ./bin/time_road_instances2_paretosearch_ls_vec.par 


make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_grid_instances1 > /dev/null
mv ./bin/time_grid_instances1 ./bin/time_grid_instances1_paretosearch_ls_btree
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_grid_instances1.par > /dev/null
mv ./bin/time_grid_instances1.par ./bin/time_grid_instances1_paretosearch_ls_btree.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_grid_instances2 > /dev/null
mv ./bin/time_grid_instances2 ./bin/time_grid_instances2_paretosearch_ls_btree
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_grid_instances2.par > /dev/null
mv ./bin/time_grid_instances2.par ./bin/time_grid_instances2_paretosearch_ls_btree.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_road_instances1 > /dev/null
mv ./bin/time_road_instances1 ./bin/time_road_instances1_paretosearch_ls_btree
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_road_instances1.par > /dev/null
mv ./bin/time_road_instances1.par ./bin/time_road_instances1_paretosearch_ls_btree.par

make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_road_instances2 > /dev/null
mv ./bin/time_road_instances2 ./bin/time_road_instances2_paretosearch_ls_btree
make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch -DBTREE_PARETO_LABELSET" time_road_instances2.par > /dev/null
mv ./bin/time_road_instances2.par ./bin/time_road_instances2_paretosearch_ls_btree.par 


#make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_sensor_instances > /dev/null
#mv ./bin/time_sensor_instances ./bin/time_sensor_instances_paretosearch
#make -B CPPFLAGS="-DLABEL_SETTING_ALGORITHM=ParetoSearch" time_sensor_instances.par > /dev/null
#mv ./bin/time_sensor_instances.par ./bin/time_sensor_instances_paretosearch.par


#########################################################
# Classic algorithm
#########################################################
echo "Classic label setting..."

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
echo "Btree algorithms..."

large_leaf_k=660
small_leaf_k=64
b=16

make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_int_large.par
make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_int_large

make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$small_leaf_k" time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_int_small.par
make -B CPPFLAGS="-DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$small_leaf_k" time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_int_small

make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_label_large.par
make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_label_large

make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$small_leaf_k" time_pq_btree.par > /dev/null
mv ./bin/time_pq_btree.par ./bin/time_pq_btree_label_small.par
make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$small_leaf_k" time_pq_btree > /dev/null
mv ./bin/time_pq_btree ./bin/time_pq_btree_label_small

make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree_delete.par > /dev/null
mv ./bin/time_pq_btree_delete.par ./bin/time_pq_btree_delete_label_large.par
make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBRANCHING_PARAMETER_B=$b -DLEAF_PARAMETER_K=$large_leaf_k" time_pq_btree_delete > /dev/null
mv ./bin/time_pq_btree_delete ./bin/time_pq_btree_delete_label_large


#########################################################
# Vector Pareto Queue
#########################################################
echo "Vector algorithms..."

make -B time_pq_vector > /dev/null
mv ./bin/time_pq_vector ./bin/time_pq_vector_int_scan

make -B CPPFLAGS="-DBINARY_VECTOR_PQ" time_pq_vector > /dev/null
mv ./bin/time_pq_vector ./bin/time_pq_vector_int_binary

make -B CPPFLAGS="-DUSE_GRAPH_LABEL" time_pq_vector > /dev/null
mv ./bin/time_pq_vector ./bin/time_pq_vector_label_scan

make -B CPPFLAGS="-DUSE_GRAPH_LABEL -DBINARY_VECTOR_PQ" time_pq_vector > /dev/null
mv ./bin/time_pq_vector ./bin/time_pq_vector_label_binary
