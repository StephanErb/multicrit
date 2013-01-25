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
Modify `src/options.hpp` to configure the algorithm / data structures. To produce new graphs run the makefile in `gnuplot/`.

### Road1 (time/distance, simple) of [Raith, Ehrgott 2009]
Extract the DC, RI and NJ tar files in `instances/`. Then run:

    ./bin/time_road_instances1 -d ../instances/ -g NJ -n 19 -c 12

    -d directory with instances
    -g graph/city name
    -n first instance number
    -c number of repetitions
    -v enable statistics

To update the timing values under `timings/` pipe all road timings into one file.

### Road2 (time/economic cost, very hard) of [Machuca 2012]
Extract the NY tar file in `instances/`. Then run:

    ./bin/time_road_instances2 -d ../instances/ -g NY -c 1

    -d directory with instances
    -g graph/city name
    -c number of repetitions
    -v enable statistics

### Grid1 (random, medium) of [Raith, Ehrgott 2009]
Random grid graphs with costs in range [1, 10]:
  
    ./bin/time_grid_instances1 -c 12

    -c number of repetitions. Each one gets a new graph.

### Grid2 (random, various difficulties) of [Machuca 2012]
Random grid graphs with costs in range [1, 10]:
  
    ./bin/time_grid_instances2 -c 12 -p -0.8

    -c number of repetitions. Each one gets a new graph.
    -p correlation of the weights (e.g., 0.8, 0.4, 0, -0,4, -0.8)

### BTree & Parallel BTree
Benchmarks are configured within & run via `./time_btrees.sh` and `./time_btree_parameter.sh`. This will automatically update the timing values under `./timings/btree`
General options of the underlying `time_btree.cpp` und `time_parallel_btree.cpp`:

    -c number of repetitions
    -k number of elements to insert
    -r ratio where n stands for the pre-existing elements within the treee for n = k * r. So, r=0 is a bulk construction.
    -p number of threads to use