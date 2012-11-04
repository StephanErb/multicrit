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
#include <cmath>

template<typename graph_slot>
class GraphGenerator {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Weight;
	typedef typename Weight::weight_type WeightType;

	// Mersenne Twister random number genrator
	boost::random::mt19937 gen;

	Weight randomWeight(WeightType max_cost) {
		boost::random::uniform_int_distribution<> dist(1, max_cost);
    	return Weight(dist(gen), dist(gen));
	}

	Weight correlatedRandomWeight(WeightType max_cost, double p) {
		boost::random::uniform_int_distribution<> dist(1, max_cost);

		double first_weight = dist(gen);
		double second_weight = dist(gen);

		if (p < 0) {
			p = p * -1;
			second_weight = 1 + max_cost - (p * first_weight + (1-p) * second_weight);
		} else {
			second_weight = (p * first_weight + (1-p) * second_weight);
		}
    	return Weight(first_weight, second_weight);
	}

public:

	typedef graph_slot Graph;

	/**
	 * Generate grid graphs as used by [Raith, Ehrgott 2009]
	 */
	void generateRandomGridGraph(Graph& graph, int height, int width) {
		const WeightType max_cost = 10;
		NodeID nodes[width][height];

		graph.addNode();
		const NodeID START = NodeID(0);

		graph.addNode();
		const NodeID END = NodeID(1);

		// Create grid of nodes
		int node_count = 2;
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				graph.addNode();
				nodes[i][j] = NodeID(node_count++);
			}	
		}
		// Link START node to the left column
		for (int j=0; j<height; ++j) {		
			graph.addEdge(START, Edge(nodes[0][j], randomWeight(max_cost))); 
		}
		// Link right column to the END node
		for (int j=0; j<height; ++j) {		
			graph.addEdge(nodes[width-1][j], Edge(END, randomWeight(max_cost))); 
		}
		// Link adjacent nodes
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				NodeID current = nodes[i][j];
				
				if (i+1 < width)  graph.addEdge(current, Edge(nodes[i+1][j], randomWeight(max_cost))); 
				if (i-1 >= 0)     graph.addEdge(current, Edge(nodes[i-1][j], randomWeight(max_cost))); 
				if (j+1 < height) graph.addEdge(current, Edge(nodes[i][j+1], randomWeight(max_cost))); 
				if (j-1 >= 0)     graph.addEdge(current, Edge(nodes[i][j-1], randomWeight(max_cost))); 
			}	
		}
		graph.finalize();
		//printGraph(graph);
	}

	/** See thesis "An Analysis of Some Algorithms and Heuristics for Multiobjective Graph Search":
	  *
	  * Grid graph (without external start and and node) where the correlation between both costs
	  * is configured by a mutliplier -1 <= p <=1

	  * Interesting p values from easy to difficult: 0.8, 0.4, 0, -0.4, -0.8
	  */
	void generateRandomGridGraphWithCostCorrleation(Graph& graph, int height, int width, double p) {
		const WeightType max_cost = 10;
		NodeID nodes[width][height];

		// Create grid of nodes
		int node_count = 0;
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				graph.addNode();
				nodes[i][j] = NodeID(node_count++);
			}	
		}
		// Link adjacent nodes
		for (int i=0; i<width; ++i) {
			for (int j=0; j<height; ++j) {
				NodeID current = nodes[i][j];
				
				if (i+1 < width)  graph.addEdge(current, Edge(nodes[i+1][j], correlatedRandomWeight(max_cost, p))); 
				if (i-1 >= 0)     graph.addEdge(current, Edge(nodes[i-1][j], correlatedRandomWeight(max_cost, p))); 
				if (j+1 < height) graph.addEdge(current, Edge(nodes[i][j+1], correlatedRandomWeight(max_cost, p))); 
				if (j-1 >= 0)     graph.addEdge(current, Edge(nodes[i][j-1], correlatedRandomWeight(max_cost, p))); 
			}	
		}
		graph.finalize();
		//printGraph(graph);
	}

	/**
	 * Simple graph with exponential number of labels. Idea taken from the paper 
	 * 		"Pareto Shortest Paths is Often Feasible in Practice" (see Fig 1.)
	 */
	void generateExponentialGraph(Graph& graph, int node_count) {
		graph.addNode();
		const NodeID START = NodeID(0);
		NodeID previous_top = START;

		int i = 0;
		int first_weight = 2;
		int second_weight = 1;

		while (i < node_count) {
			graph.addNode();
			NodeID bottom = NodeID(++i);
			graph.addNode();
			NodeID top = NodeID(++i);

			graph.addEdge(previous_top, Edge(bottom, Weight(first_weight, second_weight)));
			graph.addEdge(previous_top, Edge(top, Weight(first_weight, 2*first_weight)));
			graph.addEdge(bottom, Edge(top, Weight(first_weight, second_weight)));

			first_weight = 2 * first_weight;
			second_weight = 2 * second_weight;
			previous_top = top;
		}
		graph.finalize();
		//printGraph(graph);
	}

	void printGraph(Graph& graph) {
		FORALL_NODES(graph, node) {
			FORALL_EDGES(graph, node, eid) {
				const Edge& edge = graph.getEdge(eid);
				std::cout << node << " ---(" << edge.first_weight << ", " << edge.second_weight << ")--> " << edge.target << std::endl;
			}
		}
	}

};

#endif