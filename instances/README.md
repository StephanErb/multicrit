# Problem Instances

## Road networks: time/economic cost

Additional cost file for the DIMACS NY road network (USA-road-NY-DIMACS.zip). DIMACS format. Provided by Enrique Machuca.

* Source and goal nodes from time/distance problem instances on pages 141-144 of the thesis (tables 6.3-6.6)
* Source/goal nodes from time/economic problem instances in table 6.11 on page 152

See _Multiobjective route planning with precalculated heuristics_ and _An Analysis of Some Algorithms and Heuristics for Multiobjective Graph Search_ (thesis) for details.


## Road networks: time/distance

TIGER/Line Maps of Washington D.C (DC), Rhode Island (RI), New Jersey (NJ). Source: Raith & Ehrgott 2009 (provided by Enrique Machuca)

Origins and destinations: *_ODpairs.txt. The network file format:

	sp min #nodes #arcs
	source_node 1
	target_node -1
	arc_from arc_to arc_cost_1 arc_cost_2
	...

Regarding additional arcs: A cycle where each node connects to the next, with both objective function values being 10000, eg

	517 518 10000 10000
