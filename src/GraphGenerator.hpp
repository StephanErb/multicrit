#ifndef GRAPHGENERATOR_H_
#define GRAPHGENERATOR_H_

//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"


#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <iostream>
#include <map>

template<typename graph_slot>
class GraphGenerator {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Weight;

	// Mersenne Twister random number genrator
	boost::random::mt19937 gen;

	Weight randomWeight(int lastInRange) {
		boost::random::uniform_int_distribution<> dist(1, lastInRange);
    	return Weight(dist(gen), dist(gen));
	}

public:

	typedef graph_slot Graph;

	/**
	 * Generate grid graphs as used by [Raith, Ehrgott 2009]
	 */
	Graph generateRandomGridGraph(int height, int width) {
		const int maxCostRange = 10;
		typedef std::pair<int, int> Pos;
		std::map<Pos, NodeID> nodes;

		Graph graph;

		graph.addNode();
		const NodeID START = NodeID(0);

		graph.addNode();
		const NodeID END = NodeID(1);

		// Great grid of nodes
		int node_count = 2;
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				graph.addNode();
				nodes[Pos(i,j)] = NodeID(node_count++);
			}	
		}

		// Link START node to the left column
		for (int j=0; j<height; ++j) {		
			graph.addEdge(START, Edge(nodes[Pos(0,j)], randomWeight(maxCostRange))); 
		}

		// Link right column to the END node
		for (int j=0; j<height; ++j) {		
			graph.addEdge(nodes[Pos(width-1,j)], Edge(END, randomWeight(maxCostRange))); 
		}

		// Link adjacent nodes
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				NodeID current = nodes[Pos(i, j)];
				
				if (i+1 < width)  graph.addEdge(current, Edge(nodes[Pos(i+1,j)], randomWeight(maxCostRange))); 
				if (i-1 > 0)      graph.addEdge(current, Edge(nodes[Pos(i-1,j)], randomWeight(maxCostRange))); 
				if (j+1 < height) graph.addEdge(current, Edge(nodes[Pos(i,j+1)], randomWeight(maxCostRange))); 
				if (j-1 < 0)      graph.addEdge(current, Edge(nodes[Pos(i,j-1)], randomWeight(maxCostRange))); 
			}	
		}
		graph.finalize();
		return graph;
	}

};

#endif