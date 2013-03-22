#!/bin/bash
cd ../src/
iter_count=12

# Grid 1
echo "Computing Grid 1 Instances"
./bin/time_grid_instances1_lset_lex -c $iter_count > '../timings/classic/grid1_lset_lex'
./bin/time_grid_instances1_lset_max -c $iter_count > '../timings/classic/grid1_lset_max'
./bin/time_grid_instances1_lset_sum -c $iter_count > '../timings/classic/grid1_lset_sum'

# Road 1
echo "Computing Road 1 Instances with LEX"
out_file='../timings/classic/road1_lset_lex'
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
./bin/time_road_instances1_lset_lex -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file

echo "Computing Road 1 Instances with SUM"
out_file='../timings/classic/road1_lset_sum'
./bin/time_road_instances1_lset_sum -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
./bin/time_road_instances1_lset_sum -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
./bin/time_road_instances1_lset_sum -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file

echo "Computing Road 1 Instances with MAX"
out_file='../timings/classic/road1_lset_max'
./bin/time_road_instances1_lset_max -c $iter_count -d ../instances/ -g DC -n 1  > $out_file
./bin/time_road_instances1_lset_max -c $iter_count -d ../instances/ -g RI -n 10 >> $out_file
./bin/time_road_instances1_lset_max -c $iter_count -d ../instances/ -g NJ -n 19 >> $out_file
