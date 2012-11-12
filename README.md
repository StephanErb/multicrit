# Biobjective Label Setting Algorithms

## Build
  
    make configure
    make all
    make all DEBUG=yes

## Running benchmarks
Modify `src/options.hpp` to configure the algorithm / data structures.

### Road (time/distance, simple)
Extract the DC, RI and NJ tar files in `instances/`. Then run:

    ./bin/time_road_instances -g ../instances/RI2 -i ../instances/RI_ODpairs.txt -l RI -c 12

    -g Graph
    -i list of start & goal nodes
    -l label prefix
    -c number of repetitions

To update the timing values under `timings/` pipe all road timings into one file and updated the instance counter so that they become unique.

### Grid (random, simple)
Random grid graphs with costs in range [1, 10]:
  
    ./bin/time_grid_instances -c 12

    -c number of repetitions. Each one gets a new graph.
