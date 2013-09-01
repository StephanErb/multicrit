#ifndef GRAPH_H_
#define GRAPH_H_

#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef typename Graph::NodeID NodeID;
typedef typename Graph::EdgeID EdgeID;

#endif