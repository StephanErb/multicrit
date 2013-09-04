#ifndef GRAPH_H_
#define GRAPH_H_

#include "datastructures/graph/KGraph.hpp"
#include "datastructures/graph/Edge.hpp"
#include "datastructures/graph/GraphTypes.hpp"
#include "datastructures/graph/GraphMacros.h"

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef typename Graph::NodeID NodeID;
typedef typename Graph::EdgeID EdgeID;

#endif