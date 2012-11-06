


# Early Sequential Results

Some smoke tests to investigate suitable implementation approaches

All Grid Experiments: 200x200
Correlation factor:
	Simple: p=0.4
	Difficult: p=-0.4

## Array (scanning)
Exponential Graph: 
  Target node label count: 1024
  0.00617 [s]

Raith & Ehrgott Grid (hard): 
  Target node label count: 261
  9.49883 [s]

Machuca Grid Correlated (simple): 
  Target node label count: 210
  5.03485 [s]

Machuca Grid Correlated (difficult): 
  Target node label count: 880
  39.0188 [s]

## Array (scanning with binary search shortcut)
Exponential Graph: 
0.006154 # time in [s]

Raith & Ehrgott Grid (hard): 
8.83218 # time in [s]

Machuca Grid Correlated (p=0.4): 
4.98369 # time in [s]

Machuca Grid Correlated (p=-0.4): 
36.7179 # time in [s]


## Array + Heap LabelSet Implementation
Exponential Graph: 
  Target node label count: 1024
  0.001272 [s]

Raith & Ehrgott Grid (hard): 
  Target node label count: 261
  14.449 [s]

Machuca Grid Correlated (simple): 
  Target node label count: 210
  7.46809 [s]

Machuca Grid Correlated (difficult): 
  Target node label count: 880
  43.9459 [s]


## Separate Temp + Perm Label Arrays (scanning)

### During dominance checks: First look into temp then into perm label set
Exponential Graph: 
  Target node label count: 1024
  0.007783 [s]

Raith & Ehrgott Grid (hard): 
  Target node label count: 261
  8.57528 [s]

Machuca Grid Correlated (simple): 
  Target node label count: 210
  5.23283 [s]

Machuca Grid Correlated (difficult): 
  Target node label count: 880
  26.4623 [s]

### During dominance checks: First look into perm then into temp label set
Exponential Graph: 
  Target node label count: 1024
  0.005061 [s]

Raith & Ehrgott Grid (hard): 
  Target node label count: 261
  8.75456 [s]

Machuca Grid Correlated (simple): 
  Target node label count: 210
  5.23036 [s]

Machuca Grid Correlated (difficult): 
  Target node label count: 880
  28.6798 [s]
