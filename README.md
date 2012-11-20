# Engineering Parallel Biobjective Shortest Path Search

## Code Overview

`BiCritShortestPathAlgorithm` is the shortest path algorithm interface to be used by clients. The actual algorithm is set and configured in `options.hpp`.

1. `SeqLabelSetting.hpp`: Sequential state-of the art label setting algorithm. It stores labels in label sets implemented in `SeqLabelSet.hpp`.
2. `ParetoSearch.hpp`: Sequential label setting algorithm that is based on the novel idea to process all globally, currently non-domianted labels within a single iteration.

## Build
  
    make configure
    make all
    make all DEBUG=yes

## Running benchmarks
Modify `src/options.hpp` to configure the algorithm / data structures.

### Road (time/distance, simple)
Extract the DC, RI and NJ tar files in `instances/`. Then run:

    ./bin/time_road_instances -d ../instances/ -g NJ -n 19 -c 12

    -d directory with instances
    -g graph/city name
    -n first instance number
    -c number of repetitions

To update the timing values under `timings/` pipe all road timings into one file.

### Grid (random, simple)
Random grid graphs with costs in range [1, 10]:
  
    ./bin/time_grid_instances -c 12

    -c number of repetitions. Each one gets a new graph.

