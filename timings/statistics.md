


# SharedHeapLabelSettingAlgorithm_VectorSet_Lex

__Exponential Graph (root degree&depth=32):__

    Created Labels: 6291360
      initially dominated: 0% (=0)
      initially non-dominated: 100% (=6291360)
        finally dominated: 0% (=0)
    LabelSet sizes: 
      avg (final): 5957
      max (final): 65536
      max (temp): 65536
    Target node label count: 0
    4.86318 # time in [s]

__Raith & Ehrgott Grid (200x200):__

    Created Labels: 21839882
      initially dominated: 71.2254% (=15555545)
      initially non-dominated: 28.7746% (=6284337)
        finally dominated: 12.8985% (=810588)
    LabelSet sizes: 
      avg (final): 136
      max (final): 368
      max (temp): 368
    Target node label count: 284
    8.29521 # time in [s]

__Machuca Grid Correlated (p=0.8, 200x200):__

    Created Labels: 1357631
      initially dominated: 70.9773% (=963610)
      initially non-dominated: 29.0227% (=394021)
        finally dominated: 13.3962% (=52784)
    LabelSet sizes: 
      avg (final): 8
      max (final): 27
      max (temp): 27
    Target node label count: 23
    0.250895 # time in [s]
    
__Machuca Grid Correlated (p=-0.8, 200x200):__

    Created Labels: 112048842
      initially dominated: 68.768% (=77053703)
      initially non-dominated: 31.232% (=34995139)
        finally dominated: 19.5955% (=6857468)
    LabelSet sizes: 
      avg (final): 703
      max (final): 1583
      max (temp): 1583
    Target node label count: 1582
    61.4934 # time in [s]




# SequentialParetoSearch_std::vectorParetoQueue

__Exponential Graph (root degree&depth=32):__

    Iterations: 33
    Created Labels: 6291360
      initially dominated: 0% (=0)
        via candidate shortcut: -nan% (=0)
      initially non-dominated: 100% (=6291360)
        finally dominated: 0% (=0)
    LabelSet sizes: 
      avg: 5957
      max: 65536
    ParetoQueue sizes: 
      avg: 254196
      max: 2097152
    Pareto Optimal Elements: 
      avg: 190647
      max: 2097152
    Identical Target Nodes per Iteration: 
      avg: 4095
      max: 32768
    Target node label count: 0
    5.93423 # time in [s]

__Raith & Ehrgott Grid (200x200):__

    Iterations: 2228
    Created Labels: 21839882
      initially dominated: 72.4779% (=15829089)
        via candidate shortcut: 46.6628% (=7386294)
      initially non-dominated: 27.5221% (=6010793)
        finally dominated: 8.93466% (=537044)
    LabelSet sizes: 
      avg: 136
      max: 368
    ParetoQueue sizes: 
      avg: 24609
      max: 58144
    Pareto Optimal Elements: 
      avg: 2456
      max: 6059
    Identical Target Nodes per Iteration: 
      avg: 4
      max: 592
    Target node label count: 284
    10.0692 # time in [s]

__Machuca Grid Correlated (p=0.8, 200x200):__

    Iterations: 2270
    Created Labels: 1357631
      initially dominated: 70.4468% (=956407)
        via candidate shortcut: 48.7005% (=465775)
      initially non-dominated: 29.5532% (=401224)
        finally dominated: 14.951% (=59987)
    LabelSet sizes: 
      avg: 8
      max: 27
    ParetoQueue sizes: 
      avg: 1358
      max: 3810
    Pareto Optimal Elements: 
      avg: 150
      max: 475
    Identical Target Nodes per Iteration: 
      avg: 2
      max: 21
    Target node label count: 23
    0.376274 # time in [s]

__Machuca Grid Correlated (p=-0.8, 200x200):__

    Iterations: 4166
    Created Labels: 112048842
      initially dominated: 73.8799% (=82781568)
        via candidate shortcut: 68.7152% (=56883491)
      initially non-dominated: 26.1201% (=29267274)
        finally dominated: 3.85961% (=1129603)
    LabelSet sizes: 
      avg: 703
      max: 1583
    ParetoQueue sizes: 
      avg: 70785
      max: 152941
    Pareto Optimal Elements: 
      avg: 6754
      max: 15492
    Identical Target Nodes per Iteration: 
      avg: 11
      max: 381
    Target node label count: 1582
    45.5604 # time in [s]





  
#########################################################################
Unordered and probably outdated Misc timings. Probably on i10pc112

#########################################################################

## ParallelParetoSearch_BTreeParetoQueue (Implementation as of: Implement simple parallel ParetoSearch) -> Writing to threadlocal. THen copying all these ans sorting in final sort. Merging pareto min to delete via inplace merge.

# Exponential Graph (root degree&depth=32):
Iterations: 33
Created Labels: 5657958
  initially dominated: 0% (=0)
    via candidate shortcut: -nan% (=0)
  initially non-dominated: 100% (=5657958)
    finally dominated: 3.26032e+14% (=18446744073708918214)
LabelSet sizes: 
  avg: 5957
  max: 65536
ParetoQueue sizes: 
  avg: 254196
  max: 2097152
Pareto Optimal Elements: 
  avg: 190647
  max: 2097152
Identical Target Nodes per Iteration: 
  avg: 4120
  max: 32768
Target node label count: 0
2.87989 # time in [s]

# Raith & Ehrgott Grid (200x200):
Iterations: 2228
Created Labels: 20803769
  initially dominated: 72.0137% (=14981565)
    via candidate shortcut: 48.5513% (=7273747)
  initially non-dominated: 27.9863% (=5822204)
    finally dominated: 5.98493% (=348455)
LabelSet sizes: 
  avg: 136
  max: 368
ParetoQueue sizes: 
  avg: 24609
  max: 58144
Pareto Optimal Elements: 
  avg: 2456
  max: 6059
Identical Target Nodes per Iteration: 
  avg: 4
  max: 592
Target node label count: 284
9.96358 # time in [s]

# Machuca Grid Correlated (p=0.8, 200x200):
Iterations: 2270
Created Labels: 1290126
  initially dominated: 70.15% (=905023)
    via candidate shortcut: 50.5985% (=457928)
  initially non-dominated: 29.85% (=385103)
    finally dominated: 11.3907% (=43866)
LabelSet sizes: 
  avg: 8
  max: 27
ParetoQueue sizes: 
  avg: 1358
  max: 3810
Pareto Optimal Elements: 
  avg: 150
  max: 475
Identical Target Nodes per Iteration: 
  avg: 2
  max: 21
Target node label count: 23
0.442694 # time in [s]

# Machuca Grid Correlated (p=-0.8, 200x200):
Iterations: 4166
Created Labels: 106392871
  initially dominated: 73.569% (=78272199)
    via candidate shortcut: 71.1441% (=55686071)
  initially non-dominated: 26.431% (=28120672)
    finally dominated: 6.55985e+13% (=18446744073709534617)
LabelSet sizes: 
  avg: 703
  max: 1583
ParetoQueue sizes: 
  avg: 70785
  max: 152941
Pareto Optimal Elements: 
  avg: 6754
  max: 15492
Identical Target Nodes per Iteration: 
  avg: 11
  max: 381
Target node label count: 1582
33.4407 # time in [s]

# Implementation: Implement simple parallel ParetoSearch -> (sequential) Bucketsort with writing to threadlocal. THen copying all these ans sorting in final sort. Merging pareto min to delete via inplace merge.
Iterations: 41683
Created Labels: 894626510
  initially dominated: 60.3072% (=539524096)
    via candidate shortcut: 40.0074% (=215849559)
  initially non-dominated: 39.6928% (=355102414)
    finally dominated: 0.761665% (=2704692)
LabelSet sizes: 
  avg: 1333
  max: 10218
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Pareto Optimal Elements: 
  avg: 8454
  max: 14185
Identical Target Nodes per Iteration: 
  avg: 2
  max: 119
1 NY1 278.427 1089 3677.67 3677  # time in [s], target node label count, memory [mb], peak memory [mb] 

Iterations: 40836
Created Labels: 923367485
  initially dominated: 60.4169% (=557870129)
    via candidate shortcut: 36.6289% (=204341634)
  initially non-dominated: 39.5831% (=365497356)
    finally dominated: 4.36783% (=15964294)
LabelSet sizes: 
  avg: 1322
  max: 9664
ParetoQueue sizes: 
  avg: 662178
  max: 1455938
Pareto Optimal Elements: 
  avg: 8559
  max: 17301
Identical Target Nodes per Iteration: 
  avg: 2
  max: 179
2 NY2 344.905 1469 3767.42 3767  # time in [s], target node label count, memory [mb], peak memory [mb]


## ParallelParetoSearch_BTreeParetoQueue: Compute candidates in parallel -> Sorting via buckets per thread which are then pulled form the task responsible for this node
# Exponential Graph (root degree&depth=32):
 Statistics disabled at compile time. See options file.
Target node label count: 0
3.60492 # time in [s]

# Raith & Ehrgott Grid (200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 284
10.0307 # time in [s]

# Machuca Grid Correlated (p=0.8, 200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 23
0.55477 # time in [s]

# Machuca Grid Correlated (p=-0.8, 200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 1582
32.8334 # time in [s]

# ParallelParetoSearch_BTreeParetoQueue: Compute candidates in parallel -> Sorting via buckets per thread which are then pulled form the task responsible for this node
1 NY1 277.201 1089 3835.63 3835  # time in [s], target node label count, memory [mb], peak memory [mb] 
2 NY2 346.142 1469 3939.32 3939  # time in [s], target node label count, memory [mb], peak memory [mb] 







#########################################################################
Initialie Baum Impl erstmalig im ungetunten ParetoSearch Algo eingesetzt

(Anfang des Januar) --> Zeigt uns was für einen Weg wir gegangen sind
#########################################################################
# SequentialParetoSearch_BTreeParetoQueue
# Exponential Graph (root degree&depth=32):
Iterations: 33
Created Labels: 6291360
  initially dominated: 0% (=0)
    via candidate shortcut: -nan% (=0)
  initially non-dominated: 100% (=6291360)
    finally dominated: 0% (=0)
LabelSet sizes: 
  avg: 5957
  max: 65536
ParetoQueue sizes: 
  avg: 254196
  max: 2097152
Pareto Optimal Elements: 
  avg: 190647
  max: 2097152
Identical Target Nodes per Iteration: 
  avg: 4095
  max: 32768
Target node label count: 0
7.46374 # time in [s]

# Raith & Ehrgott Grid (200x200):
Iterations: 2228
Created Labels: 21839882
  initially dominated: 72.4779% (=15829089)
    via candidate shortcut: 46.6628% (=7386294)
  initially non-dominated: 27.5221% (=6010793)
    finally dominated: 8.93466% (=537044)
LabelSet sizes: 
  avg: 136
  max: 368
ParetoQueue sizes: 
  avg: 24609
  max: 58144
Pareto Optimal Elements: 
  avg: 2456
  max: 6059
Identical Target Nodes per Iteration: 
  avg: 4
  max: 592
Target node label count: 284
11.4939 # time in [s]

# Machuca Grid Correlated (p=0.8, 200x200):
Iterations: 2270
Created Labels: 1357631
  initially dominated: 70.4468% (=956407)
    via candidate shortcut: 48.7005% (=465775)
  initially non-dominated: 29.5532% (=401224)
    finally dominated: 14.951% (=59987)
LabelSet sizes: 
  avg: 8
  max: 27
ParetoQueue sizes: 
  avg: 1358
  max: 3810
Pareto Optimal Elements: 
  avg: 150
  max: 475
Identical Target Nodes per Iteration: 
  avg: 2
  max: 21
Target node label count: 23
0.422501 # time in [s]

# Machuca Grid Correlated (p=-0.8, 200x200):
Iterations: 4166
Created Labels: 112048842
  initially dominated: 73.8799% (=82781568)
    via candidate shortcut: 68.7152% (=56883491)
  initially non-dominated: 26.1201% (=29267274)
    finally dominated: 3.85961% (=1129603)
LabelSet sizes: 
  avg: 703
  max: 1583
ParetoQueue sizes: 
  avg: 70785
  max: 152941
Pareto Optimal Elements: 
  avg: 6754
  max: 15492
Identical Target Nodes per Iteration: 
  avg: 11
  max: 381
Target node label count: 1582
50.584 # time in [s]






################################################################
Di 19. Feb 01:35:49 CET 2013

Optimized Sequential Pareto Search (Inplace merge & bucket sort)
################################################################
## SequentialParetoSearch_VectorParetoQueue 
# Exponential Graph (root degree&depth=32):
 Statistics disabled at compile time. See options file.
Target node label count: 0
3.05904 210.281 # time in [s], mem in [mb]

# Raith & Ehrgott Grid (200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 284
7.90701 73.9531 # time in [s], mem in [mb]

# Machuca Grid Correlated (p=0.8, 200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 23
0.269046 6.64453 # time in [s], mem in [mb]

# Machuca Grid Correlated (p=-0.8, 200x200):
 Statistics disabled at compile time. See options file.
Target node label count: 1582
31.2311 381.043 # time in [s], mem in [mb]

## SequentialParetoSearch_BTreeParetoQueue  
# Exponential Graph (root degree&depth=32):
Sequential BTree: 
  inner slots size [2, 32]
  leaf slots size [15, 63]
 Statistics disabled at compile time. See options file.
Target node label count: 0
3.33039 224.801 # time in [s], mem in [mb]

# Raith & Ehrgott Grid (200x200):
Sequential BTree: 
  inner slots size [2, 32]
  leaf slots size [15, 63]
 Statistics disabled at compile time. See options file.
Target node label count: 284
8.5249 43.4219 # time in [s], mem in [mb]

# Machuca Grid Correlated (p=0.8, 200x200):
Sequential BTree: 
  inner slots size [2, 32]
  leaf slots size [15, 63]
 Statistics disabled at compile time. See options file.
Target node label count: 23
0.314143 6.54688 # time in [s], mem in [mb]

# Machuca Grid Correlated (p=-0.8, 200x200):
Sequential BTree: 
  inner slots size [2, 32]
  leaf slots size [15, 63]
 Statistics disabled at compile time. See options file.
Target node label count: 1582
34.5741 379.496 # time in [s], mem in [mb]






################################################################
Di 19. Feb 01:35:49 CET 2013

Meeting on BTree results & limited scalability

Results gathered on i10pc112
Commit: https://algo2.iti.kit.edu/svn/erb@341 d8aefcb4-f1e2-11e1-aef8-dd4ad02cd871
        95da86e65b790ee18fa9f53e69aa27c90f7a2216
################################################################

## ParallelParetoSearch_ParallelBTreeParetoQueue

# PE 4:
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  55.1715 Find & Group Pareto Min   # efficiency 0.82
  112.259 Update Labelsets          # efficiency 0.74
  17.2771 Sort Updates              # efficiency 0.70
  74.6565 Update PQ                 # efficiency 0.89
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 259.364 1089 4113.5 4113 4  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 3: relative speedup 2.57 efficiency 0.856
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  70.4725 Find & Group Pareto Min   # efficiency 0.84
  134.115 Update Labelsets          # efficiency 0.82
  20.9722 Sort Updates              # efficiency 0.77
  95.4457 Update PQ                 # efficiency 0.93
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 321.006 1089 4035.27 4035 3  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 2: relative speedup 1.85 efficiency 0.928
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  97.196 Find & Group Pareto Min    # efficiency 0.92
  181.967 Update Labelsets          # efficiency 0.91
  27.9455 Sort Updates              # efficiency 0.87
  137.752 Update PQ                 # efficiency 0.97
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 444.861 1089 3977.21 3977 2  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 1
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  179.193 Find & Group Pareto Min
  331.192 Update Labelsets 
  48.5896 Sort Updates
  266.02 Update PQ 
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 824.995 1089 3906.33 3906 1  # time in [s], target node label count, memory [mb], peak memory [mb], p

## SequentialParetoSearch_VectorParetoQueue
Iterations: 41683
Created Labels: 978246494
  initially dominated: 60.2916% (=589800871)
    via candidate shortcut: 37.9922% (=224078125)
  initially non-dominated: 39.7084% (=388445623)
    finally dominated: 9.28004% (=36047901)
LabelSet sizes: 
  avg: 1333
  max: 10218
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Pareto Optimal Elements: 
  avg: 8454
  max: 14185
Subcomponent Timings:
  57.0922 Find Pareto Min
  46.6239 Group Pareto Min
  321.016 Update Labelsets 
  39.2974 Sort Updates
  104.639 Update PQ 
1 NY1 568.668 1089 3884.86 3885 0  # time in [s], target node label count, memory [mb], peak memory [mb], 

#SequentialSharedHeapLabelSettingAlgorithm_VectorSet_Sum
1 NY1 1495.47 1089 6696.43 6696 0  # time in [s], target node label count, memory [mb], peak memory [mb], p



## ParallelParetoSearch_ParallelBTreeParetoQueue
## with Sub-Sub Component Timing

# PE 4
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  60.5254 Find & Group Pareto Min
      5.7838 Recursive TBB Tasks
      22.2471 Find Pareto Min 
      1.88078 Copy Updates 
      29.0373 Group Pareto Min  
      1.57636 Copy Affected Nodes  
  125.85 Update Labelsets 
      16.9898 Collect Candidate Labels 
      14.3863 Sort Candidate Labels 
      94.828 Update Labelsets 
      2.00101 Copy Updates 
  17.2557 Sort Updates
  74.7888 Update PQ 
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 278.42 1089 4127.46 4127 4  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 3
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  76.0922 Find & Group Pareto Min
      7.47241 Recursive TBB Tasks
      28.3385 Find Pareto Min 
      2.39405 Copy Updates 
      35.8626 Group Pareto Min  
      2.02465 Copy Affected Nodes  
  154.756 Update Labelsets 
      19.0404 Collect Candidate Labels 
      15.8416 Sort Candidate Labels 
      117.938 Update Labelsets 
      1.89586 Copy Updates 
  20.9591 Sort Updates
  95.6328 Update PQ 
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 347.44 1089 4095.49 4095 3  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 2
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  108.429 Find & Group Pareto Min
      9.49632 Recursive TBB Tasks
      42.6241 Find Pareto Min 
      3.56142 Copy Updates 
      49.8712 Group Pareto Min  
      2.87632 Copy Affected Nodes  
  214.767 Update Labelsets 
      27.5844 Collect Candidate Labels 
      28.7761 Sort Candidate Labels 
      166.284 Update Labelsets 
      1.44281 Copy Updates 
  27.9409 Sort Updates
  138.084 Update PQ 
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 489.221 1089 3976.53 3976 2  # time in [s], target node label count, memory [mb], peak memory [mb], p

# PE 1
Iterations: 41683
ParetoQueue sizes: 
  avg: 680209
  max: 1475541
Subcomponent Timings:
  200.367 Find & Group Pareto Min
      21.1344 Recursive TBB Tasks
      80.5679 Find Pareto Min 
      4.61795 Copy Updates 
      89.9865 Group Pareto Min  
      4.05987 Copy Affected Nodes  
  397.121 Update Labelsets 
      41.0496 Collect Candidate Labels 
      51.5879 Sort Candidate Labels 
      302.587 Update Labelsets 
      1.13934 Copy Updates 
  48.5227 Sort Updates
  268.089 Update PQ 
Parallel BTree (k=85, b=6):
  inner slots size [1, 24]. Bytes: 968
  leaf slots size [21, 85]. Bytes: 1024
1 NY1 914.099 1089 3907.61 3907 1  # time in [s], target node label count, memory [mb], peak memory [mb], p