# Engineering Parallel Biobjective Shortest Path Search

## Code Overview

1. State-of the art sequential label setting algorithm: `LabelSettingAlgorithm.hpp` which stores in label sets as implemented in `LabelSet.hpp`.

## Build
  
    make configure
    make all
    make all DEBUG=yes

## Running benchmarks
Modify `src/options.hpp` to configure the algorithm / data structures.

### Road (time/distance, simple)
Extract the DC, RI and NJ tar files in `instances/`. Then run:

    ./bin/time_road_instances -d ../instances/ -g NJ -n 19

    -d directory with instances
    -g graph/city name
    -n first instance number
    -c number of repetitions

To update the timing values under `timings/` pipe all road timings into one file.

### Grid (random, simple)
Random grid graphs with costs in range [1, 10]:
  
    ./bin/time_grid_instances -c 12

    -c number of repetitions. Each one gets a new graph.

