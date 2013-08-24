#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG // uncomment to enable debug print
#define TBB_USE_DEBUG 1
#define TBB_USE_ASSERT 1
#define TBB_USE_THREADING_TOOLS 1

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE shortestpath_abelsetting_tests
#include <boost/test/auto_unit_test.hpp>

#include <iostream>
#include "../BiCritShortestPathAlgorithm.hpp"
#include "../GraphGenerator.hpp"

void assertTrue(bool cond, std::string msg) {
	BOOST_REQUIRE_MESSAGE(cond, msg);
}

template<class LabelSettingAlgorithm>
bool contains(const LabelSettingAlgorithm& algo, const NodeID node, const Label& label) {
	return std::find(algo.begin(node), algo.end(node), label) != algo.end(node);
}

void createGridSimple(Graph& graph) {
	graph.addNode();
	const NodeID START = NodeID(0);

	graph.addNode();
	const NodeID END = NodeID(1);

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(2), Edge::edge_data(1,2)));
	graph.addEdge(NodeID(2), Edge(END, Edge::edge_data(1,1)));

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(3), Edge::edge_data(2,1)));
	graph.addEdge(NodeID(3), Edge(END, Edge::edge_data(1,1)));

	graph.addNode(); // dominated path from start to end
	graph.addEdge(START, Edge(NodeID(4), Edge::edge_data(1,1)));
	graph.addEdge(NodeID(4), Edge(END, Edge::edge_data(4,4)));

	graph.finalize();
}

template<class LabelSettingAlgorithm>
void testGridSimple(LabelSettingAlgorithm&& algo) {
	algo.run(NodeID(0));

	assertTrue(algo.size(NodeID(1)) == 2, "Should not contain dominated labels");
	assertTrue(contains(algo, NodeID(1), Label(2,3)), "");
	assertTrue(contains(algo, NodeID(1), Label(3,2)), "");
}

template<class LabelSettingAlgorithm>
void testGridExponential(LabelSettingAlgorithm&& algo, const Graph& graph) {
	algo.run(NodeID(0));

	assertTrue(algo.size(NodeID(0)) == 1, "Start node should have no labels");
	assertTrue(algo.size(NodeID(1)) == 1, "Second node should have one labels");

	unsigned int label_count = 2;
	for (unsigned int i=2; i<graph.numberOfNodes(); i=i+2) {
		assertTrue(algo.size(NodeID(i)) == label_count, "Expect exponential num of labels");
		label_count = 2 * label_count;
	}
}
 
BOOST_AUTO_TEST_CASE(testParetoSearch_Simple) {
	Graph graph;
	createGridSimple(graph);
	#ifdef PARALLEL_BUILD
		testGridSimple(ParetoSearch<Graph>(graph, my_default_thread_count));
	#else 
		testGridSimple(ParetoSearch<Graph>(graph));
	#endif
}
BOOST_AUTO_TEST_CASE(testParetoSearch_Exponential) {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 20);
	#ifdef PARALLEL_BUILD
		testGridExponential(ParetoSearch<Graph>(graph, my_default_thread_count), graph);
	#else 
		testGridExponential(ParetoSearch<Graph>(graph), graph);
	#endif
}

BOOST_AUTO_TEST_CASE(testSharedHeapLabelSettingAlgorithm_Simple) {
	Graph graph;
	createGridSimple(graph);
	testGridSimple(SharedHeapLabelSettingAlgorithm<Graph>(graph));
}
BOOST_AUTO_TEST_CASE(testSharedHeapLabelSettingAlgorithm_Exponential) {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 20);
	testGridExponential(SharedHeapLabelSettingAlgorithm<Graph>(graph), graph);
}

BOOST_AUTO_TEST_CASE(testNodeHeapLabelSettingAlgorithm_Simple) {
	Graph graph;
	createGridSimple(graph);
	testGridSimple(NodeHeapLabelSettingAlgorithm<Graph>(graph));
}
BOOST_AUTO_TEST_CASE(testNodeHeapLabelSettingAlgorithm_Exponential) {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 20);
	testGridExponential(NodeHeapLabelSettingAlgorithm<Graph>(graph), graph);
}