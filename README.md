# Engineering Parallel Biobjective Shortest Path Search

## Code Overview

`BiCritShortestPathAlgorithm.hpp` is the shortest path algorithm interface to be used by test and benchmarking clients. The actual algorithm is set and configured in `options.hpp`.

1. `SeqLabelSetting.hpp`: Sequential state-of the art label setting algorithm. It stores labels in label sets implemented in `SeqLabelSet.hpp`.
2. `ParetoSearch_sequential.hpp`: Implementation of the PaPaSearch algorithm of Sanders and Mandow [2013], i.e., a sequential label setting algorithm that is based on the novel idea to process all non-domianted labels within a single iteration. Depends on `ParetoQueue_sequential.hpp`.
3. `ParetoSearch_parallel.hpp`: Parallel Implementation of the PaPaSearch algorithm of Sanders and Mandow [2013]. Depends on `ParetoQueue_parallel.hpp`.
4. `datastructures\BTree.hpp`: In-memory B-tree with parallel and sequential bulk updates (insertions and deletions). 


## Build 

Within a bash shell:

    `source ../lib/tbb/bin/tbbvars.sh intel64`

Modify `src/options.hpp` to configure the algorithm / data structures. Parallel versions of the different algorithm and timing programs end in `.par`, e.g., `./bin/time_road_instances1.par`.
  
    make configure      # only needed once
    make all
    make all DEBUG=yes

    make my_program     # to build a specific executable
    make my_program.par # to build the parallel version of a specific executable


## Running benchmarks

Within `scripts/` run `build_binaries.sh` to configure & generate all binaries used for benchmarking. To run benchmarks, execute the corresponding scripts within the `scripts/` folder. Benchmarking scripts write their timing values to  `timings/`. To produce new graphs, run the makefiles in the different subdirectories of the `gnuplot/` folder.

The benchmarking scripts execute the different benchmarks `src/time_*.cpp`:

* Road1: Simple (time/distance) maps of [Raith, Ehrgott 2009]. To run, extract the DC, RI and NJ tar files in `instances/`
* Road2: Hard (time/economic cost) map of [Machuca 2012]. Extract the NY tar file in `instances/`.
* Grid1: Random grid graphs of [Raith, Ehrgott 2009]. Costs in range [1, 10]
* Grid2: Random grid graphs with tunable difficulties. Important options: `-q X` configures the correlation of the edge weights (e.g., 0.8, 0.4, 0, -0,4, -0.8) and `-m X` sets the upper bound of the cost range [0, X]. We refer to `-m 10` as simple and to `-m 10` as hard.

Common options the shortest path benchmarks:

    -c number of repetitions
    -v enable statistics
    -p PE count

Common options of the BTree / Pareto Queue benchmarks  (e.g., `time_pq_btree.cpp`):

    -c number of repetitions
    -k number of elements to insert
    -r ratio where n stands for the pre-existing elements within the treee for n = k * r. So, r=0 is a bulk construction.
    -p PE count


## Known Issues

* __BTree Implementation Bug__: Subtrees of an inner-node are rebalanced by reconstruction if they either underflow or overflow. If there is an underflow which prevents us from recreating a new subtree with a sufficient number of elements, we also include the elements of a neighboring subtree that does not need to be rebalanced otherwise. However, in our implementation, we can only include subtrees to the _right_, so we might fail to create a properly balanced subtree if the _last_ subtree of a node underflows without any directly preceding subtrees to be rebuild as well. This bug can lead to slightly imbalanced trees, but we have not noticed any performance or correctness problems. However, please note that the tree might throw assertion errors if run in debug mode. 

* __BTree Code Duplication__: The `BTree_base` class is duplicated. One copy is used as the basis of label sets the other one used by pareto queue. We should probably make the slight implementation differences configurable via template parameters (e.g., to only have the COMPUTE_PARETO_MIN macro dependency if needed).

* __Outdated code__: Certain experimental implementations and options turned out not to be competitive (e.g., specific label sets in `SeqLabelSet.hpp`). They have been abandoned long ago and might no longer work or even compile. 

* __Failing tests__: Tests have not been adapted to work with all different implementation options. For example, the tests are know to fail for the `SplittedNaiveLabelSet`.