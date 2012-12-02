



#SharedHeapLabelSettingAlgorithm_VectorSet_Lex
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
    
\newpage


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



## PreSort Optimization

# Exponential Graph (root degree&depth=32):

Iterations: 33
Created Labels: 6291360
  initially dominated: 0% (=0)
    via candidate shortcut: -nan% (=0)
  initially non-dominated: 100% (=6291360)
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
7.38313 # time in [s]

# Raith & Ehrgott Grid (200x200):

Iterations: 2228
Created Labels: 21839882
  initially dominated: 72.4779% (=15829089)
    via candidate shortcut: 9.9703% (=1578207)
  initially non-dominated: 27.5221% (=6010793)
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
12.2231 # time in [s]

# Machuca Grid Correlated (p=0.8, 200x200):

Iterations: 2270
Created Labels: 1357631
  initially dominated: 70.4468% (=956407)
    via candidate shortcut: 12.3631% (=118242)
  initially non-dominated: 29.5532% (=401224)
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
0.414576 # time in [s]

# Machuca Grid Correlated (p=-0.8, 200x200):

Iterations: 4166
Created Labels: 112048842
  initially dominated: 73.8799% (=82781568)
    via candidate shortcut: 14.6534% (=12130315)
  initially non-dominated: 26.1201% (=29267274)
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
56.9015 # time in [s]



# SequentialParetoSearch_VectorParetoQueue
# Map: ../instances/USA-road-t.NY.gr ../instances/USA-road-m.NY.gr
# Nodes 264346 Edges 733846
# Nodes 264347 Edges 730100

Iterations: 41683
Created Labels: 978246494
  initially dominated: 60.2916% (=589800871)
    via candidate shortcut: 4.02458% (=23736982)
  initially non-dominated: 39.7084% (=388445623)
ParetoQueue sizes: 
  avg: 61976
  max: 1475541
Pareto Optimal Elements: 
  avg: 8454
  max: 14185
Identical Target Nodes per Iteration: 
  avg: 2
  max: 119
1 NY1 792.945 1089 # time in [s], target node label count